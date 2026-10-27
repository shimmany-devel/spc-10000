`timescale 1ns / 1ps

// ============================================================================
// Module: mem_controller
// Target: AMD Xilinx UltraScale+ True Dual-Port Block RAM
// Port A: CPU (16-bit word / 8-bit byte-enable access, 64KB Main RAM + 8KB VRAM)
// Port B: PS AXI BRAM Controller (32-bit dword access) + Dedicated VDG Scan
// ============================================================================

module mem_controller (
    input  wire        clk,
    input  wire        reset_n,

    // CPU Port (Port A)
    input  wire [23:0] cpu_addr,
    input  wire [15:0] cpu_din,
    output wire [15:0] cpu_dout,
    input  wire        cpu_req,
    input  wire        cpu_wr,
    input  wire [1:0]  cpu_byte_en,
    output wire        cpu_ack,

    // VDG Port (Dedicated Video Scan Read)
    input  wire [12:0] vdg_vram_addr,
    output reg  [7:0]  vdg_vram_data,

    // AXI / PS Port (Port B)
    input  wire [23:0] ps_bram_addr,
    input  wire [31:0] ps_bram_wdata,
    output wire [31:0] ps_bram_rdata,
    input  wire [3:0]  ps_bram_we,
    input  wire        ps_bram_en
);

    // ========================================================================
    // 1. 64KB Main RAM (4 Banks of 16K x 8-bit for clean TDP-BRAM inference)
    // Bank 0: byte addr % 4 == 0 (lower byte of even word)
    // Bank 1: byte addr % 4 == 1 (upper byte of even word)
    // Bank 2: byte addr % 4 == 2 (lower byte of odd word)
    // Bank 3: byte addr % 4 == 3 (upper byte of odd word)
    // ========================================================================
    (* ram_style = "block" *) reg [7:0] ram_b0 [0:16383];
    (* ram_style = "block" *) reg [7:0] ram_b1 [0:16383];
    (* ram_style = "block" *) reg [7:0] ram_b2 [0:16383];
    (* ram_style = "block" *) reg [7:0] ram_b3 [0:16383];

    reg [7:0] b0_cpu_dout, b1_cpu_dout, b2_cpu_dout, b3_cpu_dout;
    reg [7:0] b0_ps_dout,  b1_ps_dout,  b2_ps_dout,  b3_ps_dout;

    wire [13:0] cpu_word_idx = cpu_addr[15:2];
    wire        cpu_word_sel = cpu_addr[1]; // 0: even word (b1,b0), 1: odd word (b3,b2)
    wire        cpu_main_ram_en = cpu_req && (cpu_addr < 24'h010000);

    wire [13:0] ps_word_idx = ps_bram_addr[15:2];
    wire        ps_main_ram_en = ps_bram_en && (ps_bram_addr < 24'h010000);

    // CPU Port (Port A) for Main RAM
    always @(posedge clk) begin
        if (cpu_main_ram_en) begin
            if (cpu_wr) begin
                if (!cpu_word_sel) begin
                    if (cpu_byte_en[1]) ram_b1[cpu_word_idx] <= cpu_din[15:8];
                    if (cpu_byte_en[0]) ram_b0[cpu_word_idx] <= cpu_din[7:0];
                end else begin
                    if (cpu_byte_en[1]) ram_b3[cpu_word_idx] <= cpu_din[15:8];
                    if (cpu_byte_en[0]) ram_b2[cpu_word_idx] <= cpu_din[7:0];
                end
            end
            b0_cpu_dout <= ram_b0[cpu_word_idx];
            b1_cpu_dout <= ram_b1[cpu_word_idx];
            b2_cpu_dout <= ram_b2[cpu_word_idx];
            b3_cpu_dout <= ram_b3[cpu_word_idx];
        end
    end

    // PS Port (Port B) for Main RAM
    always @(posedge clk) begin
        if (ps_main_ram_en) begin
            if (ps_bram_we[0]) ram_b0[ps_word_idx] <= ps_bram_wdata[7:0];
            if (ps_bram_we[1]) ram_b1[ps_word_idx] <= ps_bram_wdata[15:8];
            if (ps_bram_we[2]) ram_b2[ps_word_idx] <= ps_bram_wdata[23:16];
            if (ps_bram_we[3]) ram_b3[ps_word_idx] <= ps_bram_wdata[31:24];
            b0_ps_dout <= ram_b0[ps_word_idx];
            b1_ps_dout <= ram_b1[ps_word_idx];
            b2_ps_dout <= ram_b2[ps_word_idx];
            b3_ps_dout <= ram_b3[ps_word_idx];
        end
    end

    // Registered word selector to align with BRAM 1-cycle read latency
    reg cpu_word_sel_reg;
    reg is_vram_access_reg;
    reg ps_vram_access_reg;

    always @(posedge clk) begin
        cpu_word_sel_reg   <= cpu_word_sel;
        is_vram_access_reg <= (cpu_addr >= 24'h020000 && cpu_addr < 24'h022000);
        ps_vram_access_reg <= (ps_bram_addr >= 24'h020000 && ps_bram_addr < 24'h022000);
    end

    wire [15:0] main_ram_cpu_out = cpu_word_sel_reg ? {b3_cpu_dout, b2_cpu_dout} : {b1_cpu_dout, b0_cpu_dout};
    wire [31:0] main_ram_ps_out  = {b3_ps_dout, b2_ps_dout, b1_ps_dout, b0_ps_dout};

    // ========================================================================
    // 2. 8KB Video RAM (Dual-Port: Port A = Host CPU/PS, Port B = VDG Scan)
    // ========================================================================
    (* ram_style = "block" *) reg [7:0] vram [0:8191];
    reg [7:0] vram_host_dout;

    wire ps_vram_en  = ps_bram_en && (ps_bram_addr >= 24'h020000 && ps_bram_addr < 24'h022000);
    wire cpu_vram_en = cpu_req && (cpu_addr >= 24'h020000 && cpu_addr < 24'h022000);

    // Host Port (Port A) arbitration (PS has priority if both active)
    wire [12:0] host_vram_addr = ps_vram_en ? ps_bram_addr[12:0] : cpu_addr[12:0];
    wire [7:0]  host_vram_din  = ps_vram_en ? ps_bram_wdata[7:0] : cpu_din[7:0];
    wire        host_vram_we   = ps_vram_en ? ps_bram_we[0] : (cpu_wr && cpu_byte_en[0]);
    wire        host_vram_act  = ps_vram_en || cpu_vram_en;

    always @(posedge clk) begin
        if (host_vram_act) begin
            if (host_vram_we) begin
                vram[host_vram_addr] <= host_vram_din;
            end
            vram_host_dout <= vram[host_vram_addr];
        end
    end

    // VDG Dedicated Scan Port (Port B - Read Only)
    always @(posedge clk) begin
        vdg_vram_data <= vram[vdg_vram_addr];
    end

    // ========================================================================
    // 3. Output Muxing & Handshake
    // ========================================================================
    reg cpu_ack_reg;
    assign cpu_ack = cpu_ack_reg;

    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            cpu_ack_reg <= 1'b0;
        end else begin
            cpu_ack_reg <= cpu_req;
        end
    end

    assign cpu_dout     = is_vram_access_reg ? {8'h00, vram_host_dout} : main_ram_cpu_out;
    assign ps_bram_rdata = ps_vram_access_reg ? {24'h000000, vram_host_dout} : main_ram_ps_out;

endmodule
