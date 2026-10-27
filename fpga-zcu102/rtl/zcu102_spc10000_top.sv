`timescale 1ns / 1ps

// ============================================================================
// Top-Level Module: zcu102_spc10000_top
// Target: AMD Xilinx ZCU102 Evaluation Board (XCZU9EG-2FFVB1156E)
// Integrates PS (system_wrapper) and SPC-10000 PL System
// ============================================================================

module zcu102_spc10000_top (
    // External I/O (DIP switches SW12 & GPIO LEDs)
    input  wire [3:0]  sw_mode,
    output wire [7:0]  led,

    // PMOD Audio PWM Output
    output wire        audio_l_pwm,
    output wire        audio_r_pwm
);

    // ------------------------------------------------------------------------
    // Clock and Reset from PS
    // ------------------------------------------------------------------------
    wire pl_clk100;
    wire pl_resetn;

    // ------------------------------------------------------------------------
    // BRAM Interface between PS and PL Dual-Port RAM
    // ------------------------------------------------------------------------
    wire [16:0] bram_porta_addr;
    wire        bram_porta_clk;
    wire [31:0] bram_porta_din;
    wire [31:0] bram_porta_dout;
    wire        bram_porta_en;
    wire        bram_porta_rst;
    wire [3:0]  bram_porta_we;

    // ------------------------------------------------------------------------
    // AXI4-Lite Register Interface between PS and PL Registers
    // ------------------------------------------------------------------------
    wire [31:0] m_axi_regs_araddr;
    wire [1:0]  m_axi_regs_arburst;
    wire [3:0]  m_axi_regs_arcache;
    wire [7:0]  m_axi_regs_arlen;
    wire [0:0]  m_axi_regs_arlock;
    wire [2:0]  m_axi_regs_arprot;
    wire [3:0]  m_axi_regs_arqos;
    wire        m_axi_regs_arready;
    wire [2:0]  m_axi_regs_arsize;
    wire [15:0] m_axi_regs_aruser;
    wire        m_axi_regs_arvalid;

    wire [31:0] m_axi_regs_awaddr;
    wire [1:0]  m_axi_regs_awburst;
    wire [3:0]  m_axi_regs_awcache;
    wire [7:0]  m_axi_regs_awlen;
    wire [0:0]  m_axi_regs_awlock;
    wire [2:0]  m_axi_regs_awprot;
    wire [3:0]  m_axi_regs_awqos;
    wire        m_axi_regs_awready;
    wire [2:0]  m_axi_regs_awsize;
    wire [15:0] m_axi_regs_awuser;
    wire        m_axi_regs_awvalid;

    wire        m_axi_regs_bready;
    wire [1:0]  m_axi_regs_bresp;
    wire        m_axi_regs_bvalid;

    wire [31:0] m_axi_regs_rdata;
    wire        m_axi_regs_rlast;
    wire        m_axi_regs_rready;
    wire [1:0]  m_axi_regs_rresp;
    wire        m_axi_regs_rvalid;

    wire [31:0] m_axi_regs_wdata;
    wire        m_axi_regs_wlast;
    wire        m_axi_regs_wready;
    wire [3:0]  m_axi_regs_wstrb;
    wire        m_axi_regs_wvalid;

    // Single beat transfer for AXI4-Lite slave
    assign m_axi_regs_rlast = 1'b1;

    // ------------------------------------------------------------------------
    // Processing System (Zynq UltraScale+ MPSoC BD Wrapper)
    // ------------------------------------------------------------------------
    system_wrapper u_system (
        .BRAM_PORTA_addr(bram_porta_addr),
        .BRAM_PORTA_clk(bram_porta_clk),
        .BRAM_PORTA_din(bram_porta_din),
        .BRAM_PORTA_dout(bram_porta_dout),
        .BRAM_PORTA_en(bram_porta_en),
        .BRAM_PORTA_rst(bram_porta_rst),
        .BRAM_PORTA_we(bram_porta_we),

        .M_AXI_REGS_araddr(m_axi_regs_araddr),
        .M_AXI_REGS_arburst(m_axi_regs_arburst),
        .M_AXI_REGS_arcache(m_axi_regs_arcache),
        .M_AXI_REGS_arlen(m_axi_regs_arlen),
        .M_AXI_REGS_arlock(m_axi_regs_arlock),
        .M_AXI_REGS_arprot(m_axi_regs_arprot),
        .M_AXI_REGS_arqos(m_axi_regs_arqos),
        .M_AXI_REGS_arready(m_axi_regs_arready),
        .M_AXI_REGS_arsize(m_axi_regs_arsize),
        .M_AXI_REGS_aruser(m_axi_regs_aruser),
        .M_AXI_REGS_arvalid(m_axi_regs_arvalid),

        .M_AXI_REGS_awaddr(m_axi_regs_awaddr),
        .M_AXI_REGS_awburst(m_axi_regs_awburst),
        .M_AXI_REGS_awcache(m_axi_regs_awcache),
        .M_AXI_REGS_awlen(m_axi_regs_awlen),
        .M_AXI_REGS_awlock(m_axi_regs_awlock),
        .M_AXI_REGS_awprot(m_axi_regs_awprot),
        .M_AXI_REGS_awqos(m_axi_regs_awqos),
        .M_AXI_REGS_awready(m_axi_regs_awready),
        .M_AXI_REGS_awsize(m_axi_regs_awsize),
        .M_AXI_REGS_awuser(m_axi_regs_awuser),
        .M_AXI_REGS_awvalid(m_axi_regs_awvalid),

        .M_AXI_REGS_bready(m_axi_regs_bready),
        .M_AXI_REGS_bresp(m_axi_regs_bresp),
        .M_AXI_REGS_bvalid(m_axi_regs_bvalid),

        .M_AXI_REGS_rdata(m_axi_regs_rdata),
        .M_AXI_REGS_rlast(m_axi_regs_rlast),
        .M_AXI_REGS_rready(m_axi_regs_rready),
        .M_AXI_REGS_rresp(m_axi_regs_rresp),
        .M_AXI_REGS_rvalid(m_axi_regs_rvalid),

        .M_AXI_REGS_wdata(m_axi_regs_wdata),
        .M_AXI_REGS_wlast(m_axi_regs_wlast),
        .M_AXI_REGS_wready(m_axi_regs_wready),
        .M_AXI_REGS_wstrb(m_axi_regs_wstrb),
        .M_AXI_REGS_wvalid(m_axi_regs_wvalid),

        .pl_clk100(pl_clk100),
        .pl_resetn(pl_resetn)
    );

    // ------------------------------------------------------------------------
    // Video Stream Interface (Stub / Loopback)
    // ------------------------------------------------------------------------
    wire [23:0] video_tdata;
    wire        video_tvalid;
    wire        video_tuser;
    wire        video_tlast;

    // ------------------------------------------------------------------------
    // SPC-10000 Core Logic
    // ------------------------------------------------------------------------
    top_spc10000 u_spc10000 (
        .clk_100m(pl_clk100),
        .reset_n(pl_resetn),

        .sw_mode(sw_mode),
        .led(led),
        .audio_l_pwm(audio_l_pwm),
        .audio_r_pwm(audio_r_pwm),

        // AXI4-Lite Registers
        .s_axi_awaddr(m_axi_regs_awaddr[7:0]),
        .s_axi_awprot(m_axi_regs_awprot),
        .s_axi_awvalid(m_axi_regs_awvalid),
        .s_axi_awready(m_axi_regs_awready),
        .s_axi_wdata(m_axi_regs_wdata),
        .s_axi_wstrb(m_axi_regs_wstrb),
        .s_axi_wvalid(m_axi_regs_wvalid),
        .s_axi_wready(m_axi_regs_wready),
        .s_axi_bresp(m_axi_regs_bresp),
        .s_axi_bvalid(m_axi_regs_bvalid),
        .s_axi_bready(m_axi_regs_bready),
        .s_axi_araddr(m_axi_regs_araddr[7:0]),
        .s_axi_arprot(m_axi_regs_arprot),
        .s_axi_arvalid(m_axi_regs_arvalid),
        .s_axi_arready(m_axi_regs_arready),
        .s_axi_rdata(m_axi_regs_rdata),
        .s_axi_rresp(m_axi_regs_rresp),
        .s_axi_rvalid(m_axi_regs_rvalid),
        .s_axi_rready(m_axi_regs_rready),

        // BRAM Memory Interface
        .s_bram_addr({7'b0000000, bram_porta_addr}),
        .s_bram_wdata(bram_porta_din),
        .s_bram_rdata(bram_porta_dout),
        .s_bram_we(bram_porta_we),
        .s_bram_en(bram_porta_en),

        // Video Stream
        .m_axis_video_tdata(video_tdata),
        .m_axis_video_tvalid(video_tvalid),
        .m_axis_video_tready(1'b1), // Always sink frames
        .m_axis_video_tuser(video_tuser),
        .m_axis_video_tlast(video_tlast)
    );

endmodule

