`timescale 1ns / 1ps

// AXI4-Lite Slave Controller for SPC-10000 Control & Status Registers
module axi_lite_regs #(
    parameter integer C_S_AXI_DATA_WIDTH = 32,
    parameter integer C_S_AXI_ADDR_WIDTH = 8
)(
    input  wire                                S_AXI_ACLK,
    input  wire                                S_AXI_ARESETN,
    // Write Address Channel
    input  wire [C_S_AXI_ADDR_WIDTH-1:0]       S_AXI_AWADDR,
    input  wire [2:0]                          S_AXI_AWPROT,
    input  wire                                S_AXI_AWVALID,
    output reg                                 S_AXI_AWREADY,
    // Write Data Channel
    input  wire [C_S_AXI_DATA_WIDTH-1:0]       S_AXI_WDATA,
    input  wire [(C_S_AXI_DATA_WIDTH/8)-1:0]   S_AXI_WSTRB,
    input  wire                                S_AXI_WVALID,
    output reg                                 S_AXI_WREADY,
    // Write Response Channel
    output reg  [1:0]                          S_AXI_BRESP,
    output reg                                 S_AXI_BVALID,
    input  wire                                S_AXI_BREADY,
    // Read Address Channel
    input  wire [C_S_AXI_ADDR_WIDTH-1:0]       S_AXI_ARADDR,
    input  wire [2:0]                          S_AXI_ARPROT,
    input  wire                                S_AXI_ARVALID,
    output reg                                 S_AXI_ARREADY,
    // Read Data Channel
    output reg  [C_S_AXI_DATA_WIDTH-1:0]       S_AXI_RDATA,
    output reg  [1:0]                          S_AXI_RRESP,
    output reg                                 S_AXI_RVALID,
    input  wire                                S_AXI_RREADY,

    // Hardware Interface
    output reg                                 reg_cpu_reset,
    output reg                                 reg_mode_select,    // 0: Z80, 1: Z8000
    output reg  [1:0]                          reg_clk_speed,      // 0: 4/10MHz, 1: 20MHz, 2: 50MHz, 3: 100MHz
    output reg  [79:0]                         reg_key_matrix,     // 10 bytes (8 bits each)
    output reg  [7:0]                          reg_sub_mbox_in,    // PS -> Z8000/Z80
    input  wire [7:0]                          reg_sub_mbox_out,   // Z8000/Z80 -> PS
    input  wire [7:0]                          reg_status_in
);

    // Address Decodes
    localparam ADDR_SYS_CTRL    = 8'h00; // [0] Reset, [1] Mode, [3:2] Speed
    localparam ADDR_STATUS      = 8'h04;
    localparam ADDR_KBD_0_3     = 8'h10; // Row 0..3
    localparam ADDR_KBD_4_7     = 8'h14; // Row 4..7
    localparam ADDR_KBD_8_9     = 8'h18; // Row 8..9
    localparam ADDR_MBOX        = 8'h20; // Mailbox IO

    // Internal Registers
    reg [31:0] sys_ctrl_reg;
    reg [31:0] kbd_0_3_reg;
    reg [31:0] kbd_4_7_reg;
    reg [15:0] kbd_8_9_reg;
    reg [7:0]  mbox_reg;

    assign reg_cpu_reset   = sys_ctrl_reg[0];
    assign reg_mode_select = sys_ctrl_reg[1];
    assign reg_clk_speed   = sys_ctrl_reg[3:2];
    assign reg_key_matrix  = {kbd_8_9_reg, kbd_4_7_reg, kbd_0_3_reg};
    assign reg_sub_mbox_in = mbox_reg;

    // Write state machine
    reg aw_en;
    always @(posedge S_AXI_ACLK) begin
        if (!S_AXI_ARESETN) begin
            S_AXI_AWREADY <= 1'b0;
            S_AXI_WREADY  <= 1'b0;
            S_AXI_BVALID  <= 1'b0;
            S_AXI_BRESP   <= 2'b00;
            aw_en         <= 1'b1;
            sys_ctrl_reg  <= 32'h0000_0001; // Default: Reset asserted
            kbd_0_3_reg   <= 32'hFFFF_FFFF; // Default: All keys released (Active-Low)
            kbd_4_7_reg   <= 32'hFFFF_FFFF;
            kbd_8_9_reg   <= 16'hFFFF;
            mbox_reg      <= 8'h00;
        end else begin
            if (~S_AXI_AWREADY && S_AXI_AWVALID && S_AXI_WVALID && aw_en) begin
                S_AXI_AWREADY <= 1'b1;
                S_AXI_WREADY  <= 1'b1;
                aw_en         <= 1'b0;
            end else begin
                S_AXI_AWREADY <= 1'b0;
                S_AXI_WREADY  <= 1'b0;
            end

            if (S_AXI_WREADY && S_AXI_WVALID && S_AXI_AWREADY && S_AXI_AWVALID) begin
                case (S_AXI_AWADDR[7:0])
                    ADDR_SYS_CTRL: sys_ctrl_reg <= S_AXI_WDATA;
                    ADDR_KBD_0_3:  kbd_0_3_reg  <= S_AXI_WDATA;
                    ADDR_KBD_4_7:  kbd_4_7_reg  <= S_AXI_WDATA;
                    ADDR_KBD_8_9:  kbd_8_9_reg  <= S_AXI_WDATA[15:0];
                    ADDR_MBOX:     mbox_reg     <= S_AXI_WDATA[7:0];
                    default: ;
                endcase
            end

            if (~S_AXI_BVALID && S_AXI_AWVALID && S_AXI_WVALID && S_AXI_AWREADY && S_AXI_WREADY) begin
                S_AXI_BVALID <= 1'b1;
                S_AXI_BRESP  <= 2'b00;
            end else if (S_AXI_BREADY && S_AXI_BVALID) begin
                S_AXI_BVALID <= 1'b0;
                aw_en        <= 1'b1;
            end
        end
    end

    // Read state machine
    always @(posedge S_AXI_ACLK) begin
        if (!S_AXI_ARESETN) begin
            S_AXI_ARREADY <= 1'b0;
            S_AXI_RVALID  <= 1'b0;
            S_AXI_RRESP   <= 2'b00;
            S_AXI_RDATA   <= 32'h0;
        end else begin
            if (~S_AXI_ARREADY && S_AXI_ARVALID) begin
                S_AXI_ARREADY <= 1'b1;
            end else begin
                S_AXI_ARREADY <= 1'b0;
            end

            if (S_AXI_ARREADY && S_AXI_ARVALID && ~S_AXI_RVALID) begin
                S_AXI_RVALID <= 1'b1;
                S_AXI_RRESP  <= 2'b00;
                case (S_AXI_ARADDR[7:0])
                    ADDR_SYS_CTRL: S_AXI_RDATA <= sys_ctrl_reg;
                    ADDR_STATUS:   S_AXI_RDATA <= {24'h0, reg_status_in};
                    ADDR_KBD_0_3:  S_AXI_RDATA <= kbd_0_3_reg;
                    ADDR_KBD_4_7:  S_AXI_RDATA <= kbd_4_7_reg;
                    ADDR_KBD_8_9:  S_AXI_RDATA <= {16'h0, kbd_8_9_reg};
                    ADDR_MBOX:     S_AXI_RDATA <= {24'h0, reg_sub_mbox_out};
                    default:       S_AXI_RDATA <= 32'h0;
                endcase
            end else if (S_AXI_RVALID && S_AXI_RREADY) begin
                S_AXI_RVALID <= 1'b0;
            end
        end
    end

endmodule
