`timescale 1ns / 1ps

// ============================================================================
// Top-Level Module: top_spc10000
// Target: AMD Xilinx Zynq UltraScale+ ZCU102
// Supports Dual Mode: Legacy SPC-1000 (Z80) / Evolutionary SPC-10000 (Z8000)
// ============================================================================

module top_spc10000 (
    // Differential System Clock / Single-ended 100MHz PL Clock
    input  wire        clk_100m,
    input  wire        reset_n,

    // External I/O (DIP switches & LEDs)
    input  wire [3:0]  sw_mode,
    output wire [7:0]  led,

    // Sound PWM Output (PMOD)
    output wire        audio_l_pwm,
    output wire        audio_r_pwm,

    // AXI4-Lite Slave Interface (from PS)
    input  wire [7:0]  s_axi_awaddr,
    input  wire [2:0]  s_axi_awprot,
    input  wire        s_axi_awvalid,
    output wire        s_axi_awready,
    input  wire [31:0] s_axi_wdata,
    input  wire [3:0]  s_axi_wstrb,
    input  wire        s_axi_wvalid,
    output wire        s_axi_wready,
    output wire [1:0]  s_axi_bresp,
    output wire        s_axi_bvalid,
    input  wire        s_axi_bready,
    input  wire [7:0]  s_axi_araddr,
    input  wire [2:0]  s_axi_arprot,
    input  wire        s_axi_arvalid,
    output wire        s_axi_arready,
    output wire [31:0] s_axi_rdata,
    output wire [1:0]  s_axi_rresp,
    output wire        s_axi_rvalid,
    input  wire        s_axi_rready,

    // AXI BRAM Controller Interface (from PS for fast memory access)
    input  wire [23:0] s_bram_addr,
    input  wire [31:0] s_bram_wdata,
    output wire [31:0] s_bram_rdata,
    input  wire [3:0]  s_bram_we,
    input  wire        s_bram_en,

    // AXI4-Stream Video Master Interface (to DisplayPort Subsystem)
    output wire [23:0] m_axis_video_tdata,
    output wire        m_axis_video_tvalid,
    input  wire        m_axis_video_tready,
    output wire        m_axis_video_tuser,
    output wire        m_axis_video_tlast
);

    // ------------------------------------------------------------------------
    // Clock Dividers & Enables (From 100MHz master)
    // ------------------------------------------------------------------------
    reg [4:0] cnt_25 = 0;    // for 4MHz (100 / 25)
    reg [5:0] cnt_28 = 0;    // for ~3.57MHz (100 / 28)
    reg [3:0] cnt_turbo = 0;

    reg ce_4mhz  = 0;
    reg ce_3m58  = 0;
    reg ce_z8000 = 0;

    always @(posedge clk_100m) begin
        // 4MHz strobe
        if (cnt_25 == 24) begin
            cnt_25 <= 0;
            ce_4mhz <= 1'b1;
        end else begin
            cnt_25 <= cnt_25 + 1'b1;
            ce_4mhz <= 1'b0;
        end

        // 3.57MHz strobe (~28 divisor)
        if (cnt_28 == 27) begin
            cnt_28 <= 0;
            ce_3m58 <= 1'b1;
        end else begin
            cnt_28 <= cnt_28 + 1'b1;
            ce_3m58 <= 1'b0;
        end

        // Z8000 Clock based on speed setting
        case (reg_clk_speed)
            2'b00: ce_z8000 <= (cnt_25 % 10 == 0); // 10MHz
            2'b01: ce_z8000 <= (cnt_25 % 5 == 0);  // 20MHz
            2'b10: ce_z8000 <= (cnt_25 % 2 == 0);  // 50MHz
            2'b11: ce_z8000 <= 1'b1;               // 100MHz (Full speed)
        endcase
    end

    // ------------------------------------------------------------------------
    // AXI Register Block (Control/Status)
    // ------------------------------------------------------------------------
    wire        reg_cpu_reset;
    wire        reg_mode_select;
    wire [1:0]  reg_clk_speed;
    wire [79:0] reg_key_matrix;
    wire [7:0]  reg_sub_mbox_in;
    wire [7:0]  reg_sub_mbox_out;
    wire [7:0]  reg_status_in = {6'b000000, reg_mode_select, ~reset_n};

    axi_lite_regs u_axi_regs (
        .S_AXI_ACLK(clk_100m),
        .S_AXI_ARESETN(reset_n),
        .S_AXI_AWADDR(s_axi_awaddr),
        .S_AXI_AWPROT(s_axi_awprot),
        .S_AXI_AWVALID(s_axi_awvalid),
        .S_AXI_AWREADY(s_axi_awready),
        .S_AXI_WDATA(s_axi_wdata),
        .S_AXI_WSTRB(s_axi_wstrb),
        .S_AXI_WVALID(s_axi_wvalid),
        .S_AXI_WREADY(s_axi_wready),
        .S_AXI_BRESP(s_axi_bresp),
        .S_AXI_BVALID(s_axi_bvalid),
        .S_AXI_BREADY(s_axi_bready),
        .S_AXI_ARADDR(s_axi_araddr),
        .S_AXI_ARPROT(s_axi_arprot),
        .S_AXI_ARVALID(s_axi_arvalid),
        .S_AXI_ARREADY(s_axi_arready),
        .S_AXI_RDATA(s_axi_rdata),
        .S_AXI_RRESP(s_axi_rresp),
        .S_AXI_RVALID(s_axi_rvalid),
        .S_AXI_RREADY(s_axi_rready),
        .reg_cpu_reset(reg_cpu_reset),
        .reg_mode_select(reg_mode_select),
        .reg_clk_speed(reg_clk_speed),
        .reg_key_matrix(reg_key_matrix),
        .reg_sub_mbox_in(reg_sub_mbox_in),
        .reg_sub_mbox_out(reg_sub_mbox_out),
        .reg_status_in(reg_status_in)
    );

    // Global reset combining SW & AXI
    wire sys_reset_n = reset_n && !reg_cpu_reset && !sw_mode[3];
    wire is_z8000_mode = reg_mode_select || sw_mode[0];

    // Status LED
    assign led[0] = ~sys_reset_n;
    assign led[1] = is_z8000_mode;
    assign led[3:2] = reg_clk_speed;
    assign led[7:4] = 4'b1010; // heartbeat/alive pattern

    // ------------------------------------------------------------------------
    // Memory Subsystem
    // ------------------------------------------------------------------------
    wire [23:0] cpu_addr;
    wire [15:0] cpu_din;
    wire [15:0] cpu_dout;
    wire        cpu_req;
    wire        cpu_wr;
    wire [1:0]  cpu_byte_en;
    wire        cpu_ack;

    wire [12:0] vdg_vram_addr;
    wire [7:0]  vdg_vram_data;

    mem_controller u_mem_ctrl (
        .clk(clk_100m),
        .reset_n(sys_reset_n),
        .cpu_addr(cpu_addr),
        .cpu_din(cpu_din),
        .cpu_dout(cpu_dout),
        .cpu_req(cpu_req),
        .cpu_wr(cpu_wr),
        .cpu_byte_en(cpu_byte_en),
        .cpu_ack(cpu_ack),
        .vdg_vram_addr(vdg_vram_addr),
        .vdg_vram_data(vdg_vram_data),
        .ps_bram_addr(s_bram_addr),
        .ps_bram_wdata(s_bram_wdata),
        .ps_bram_rdata(s_bram_rdata),
        .ps_bram_we(s_bram_we),
        .ps_bram_en(s_bram_en)
    );

    // ------------------------------------------------------------------------
    // CPU: Z8000 Main Processor
    // ------------------------------------------------------------------------
    wire [23:0] z8k_addr;
    wire [15:0] z8k_data_out;
    wire        z8k_mreq_n;
    wire        z8k_iorq_n;
    wire        z8k_rd_n;
    wire        z8k_wr_n;
    wire [1:0]  z8k_byte_en;
    wire [3:0]  z8k_status;

    z8000_core u_z8000 (
        .clk(clk_100m),
        .clk_en(ce_z8000),
        .reset_n(sys_reset_n && is_z8000_mode),
        .addr(z8k_addr),
        .data_out(z8k_data_out),
        .data_in(cpu_dout),
        .byte_en(z8k_byte_en),
        .mreq_n(z8k_mreq_n),
        .iorq_n(z8k_iorq_n),
        .rd_n(z8k_rd_n),
        .wr_n(z8k_wr_n),
        .status(z8k_status),
        .wait_n(cpu_ack),
        .nvi_n(1'b1),
        .vi_n(1'b1),
        .nmi_n(1'b1)
    );

    assign cpu_addr    = z8k_addr;
    assign cpu_din     = z8k_data_out;
    assign cpu_req     = ~z8k_mreq_n;
    assign cpu_wr      = ~z8k_wr_n;
    assign cpu_byte_en = z8k_byte_en;

    // ------------------------------------------------------------------------
    // Video: MC6847 VDG + AXI4-Stream
    // ------------------------------------------------------------------------
    reg  [7:0] gmode_reg = 8'h00;
    wire [7:0] vdg_r, vdg_g, vdg_b;
    wire       vdg_valid, vdg_hsync, vdg_vsync, vdg_fs;

    // Simple I/O capture for GMODE (0x2000)
    always @(posedge clk_100m) begin
        if (!sys_reset_n) begin
            gmode_reg <= 8'h00;
        end else if (~z8k_iorq_n && ~z8k_wr_n && z8k_addr[15:0] == 16'h2000) begin
            gmode_reg <= z8k_data_out[7:0];
        end
    end

    mc6847_core u_vdg (
        .clk(clk_100m),
        .clk_en(ce_3m58),
        .reset_n(sys_reset_n),
        .gmode(gmode_reg),
        .vram_addr(vdg_vram_addr),
        .vram_data(vdg_vram_data),
        .pixel_r(vdg_r),
        .pixel_g(vdg_g),
        .pixel_b(vdg_b),
        .pixel_valid(vdg_valid),
        .hsync(vdg_hsync),
        .vsync(vdg_vsync),
        .field_sync(vdg_fs)
    );

    axis_video_out u_video_stream (
        .aclk(clk_100m),
        .aresetn(sys_reset_n),
        .pixel_r(vdg_r),
        .pixel_g(vdg_g),
        .pixel_b(vdg_b),
        .pixel_valid(vdg_valid),
        .hsync(vdg_hsync),
        .vsync(vdg_vsync),
        .m_axis_tdata(m_axis_video_tdata),
        .m_axis_tvalid(m_axis_video_tvalid),
        .m_axis_tready(m_axis_video_tready),
        .m_axis_tuser(m_axis_video_tuser),
        .m_axis_tlast(m_axis_video_tlast)
    );

    // ------------------------------------------------------------------------
    // Sound: AY-3-8910 PSG
    // ------------------------------------------------------------------------
    reg [3:0] psg_addr = 0;
    reg psg_wr = 0;
    reg [7:0] psg_data = 0;

    always @(posedge clk_100m) begin
        if (!sys_reset_n) begin
            psg_addr <= 4'h0;
            psg_wr   <= 1'b0;
            psg_data <= 8'h00;
        end else begin
            psg_wr <= 1'b0;
            if (~z8k_iorq_n && ~z8k_wr_n) begin
                if (z8k_addr[15:1] == 15'h2000) begin // 0x4000: address select
                    psg_addr <= z8k_data_out[3:0];
                end else if (z8k_addr[15:1] == 15'h2000 && z8k_addr[0]) begin // 0x4001: data write
                    psg_data <= z8k_data_out[7:0];
                    psg_wr <= 1'b1;
                end
            end
        end
    end

    psg_ay8910 u_psg (
        .clk(clk_100m),
        .clk_en(ce_3m58),
        .reset_n(sys_reset_n),
        .wr_en(psg_wr),
        .addr(psg_addr),
        .din(psg_data),
        .dout(),
        .audio_pwm_l(audio_l_pwm),
        .audio_pwm_r(audio_r_pwm)
    );

endmodule
