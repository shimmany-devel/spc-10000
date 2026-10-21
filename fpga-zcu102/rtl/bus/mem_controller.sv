`timescale 1ns / 1ps

// Dual-Port Memory System:
// Port A: Z8000 / Z80 CPU Access
// Port B: PS AXI Bus Master (for DMA, ROM flashing, and debugger access)
module mem_controller (
    input  wire        clk,
    input  wire        reset_n,

    // CPU Port (Port A)
    input  wire [23:0] cpu_addr,
    input  wire [15:0] cpu_din,
    output reg  [15:0] cpu_dout,
    input  wire        cpu_req,
    input  wire        cpu_wr,
    input  wire [1:0]  cpu_byte_en,
    output wire        cpu_ack,

    // VDG Port (Dedicated Video Scan)
    input  wire [12:0] vdg_vram_addr,
    output wire [7:0]  vdg_vram_data,

    // AXI / PS Port (Port B)
    input  wire [23:0] ps_bram_addr,
    input  wire [31:0] ps_bram_wdata,
    output reg  [31:0] ps_bram_rdata,
    input  wire [3:0]  ps_bram_we,
    input  wire        ps_bram_en
);

    // 64KB Main RAM (BRAM) - accessible by both Z80 and Z8000
    (* ram_style = "block" *) reg [7:0] main_ram [0:65535];

    // 8KB VRAM (BRAM)
    (* ram_style = "block" *) reg [7:0] vram [0:8191];

    // ACK for CPU
    reg cpu_ack_reg;
    assign cpu_ack = cpu_ack_reg;

    // CPU Port A Read/Write
    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            cpu_ack_reg <= 1'b0;
            cpu_dout    <= 16'h0000;
        end else begin
            cpu_ack_reg <= cpu_req;
            if (cpu_req) begin
                if (cpu_addr < 24'h010000) begin
                    // Main RAM region
                    if (cpu_wr) begin
                        if (cpu_byte_en[1]) main_ram[cpu_addr[15:0]]     <= cpu_din[15:8];
                        if (cpu_byte_en[0]) main_ram[cpu_addr[15:0] + 1] <= cpu_din[7:0];
                    end else begin
                        cpu_dout <= {main_ram[cpu_addr[15:0]], main_ram[cpu_addr[15:0] + 1]};
                    end
                end else if (cpu_addr >= 24'h020000 && cpu_addr < 24'h022000) begin
                    // Direct VRAM mapping
                    if (cpu_wr) begin
                        if (cpu_byte_en[0]) vram[cpu_addr[12:0]] <= cpu_din[7:0];
                    end else begin
                        cpu_dout <= {8'h00, vram[cpu_addr[12:0]]};
                    end
                end
            end
        end
    end

    // VDG dedicated read port
    assign vdg_vram_data = vram[vdg_vram_addr];

    // PS Port B
    always @(posedge clk) begin
        if (ps_bram_en) begin
            if (ps_bram_addr < 24'h010000) begin
                if (ps_bram_we[0]) main_ram[ps_bram_addr + 0] <= ps_bram_wdata[7:0];
                if (ps_bram_we[1]) main_ram[ps_bram_addr + 1] <= ps_bram_wdata[15:8];
                if (ps_bram_we[2]) main_ram[ps_bram_addr + 2] <= ps_bram_wdata[23:16];
                if (ps_bram_we[3]) main_ram[ps_bram_addr + 3] <= ps_bram_wdata[31:24];
                ps_bram_rdata <= {main_ram[ps_bram_addr + 3], main_ram[ps_bram_addr + 2],
                                  main_ram[ps_bram_addr + 1], main_ram[ps_bram_addr + 0]};
            end else if (ps_bram_addr >= 24'h020000 && ps_bram_addr < 24'h022000) begin
                wire [12:0] v_offset = ps_bram_addr[12:0];
                if (ps_bram_we[0]) vram[v_offset] <= ps_bram_wdata[7:0];
                ps_bram_rdata <= {24'h0, vram[v_offset]};
            end
        end
    end

endmodule
