`timescale 1ns / 1ps

// Zilog Z8001 16-bit Segmented Microprocessor Core
// Instruction set subset sufficient for bootstrap, BIOS, and core operations
module z8000_core (
    input  wire        clk,
    input  wire        clk_en,
    input  wire        reset_n,

    // Bus Interface
    output reg  [23:0] addr,         // Segment (7-bit) + Offset (16-bit)
    output reg  [15:0] data_out,
    input  wire [15:0] data_in,
    output reg  [1:0]  byte_en,      // High byte, Low byte
    output reg         mreq_n,
    output reg         iorq_n,
    output reg         rd_n,
    output reg         wr_n,
    output reg  [3:0]  status,       // 0001: Mem Read, 0010: Mem Write, 0011: I/O, etc.
    input  wire        wait_n,

    // Interrupts
    input  wire        nvi_n,
    input  wire        vi_n,
    input  wire        nmi_n
);

    // Status definitions
    localparam ST_IFETCH = 4'b0000;
    localparam ST_MEM_RD = 4'b0001;
    localparam ST_MEM_WR = 4'b0010;
    localparam ST_IO_RD  = 4'b0011;
    localparam ST_IO_WR  = 4'b0100;
    localparam ST_HALT   = 4'b1000;

    // Registers: 16 x 16-bit General Registers (R0 - R15)
    reg [15:0] regs [0:15];
    reg [15:0] pc_offset;
    reg [6:0]  pc_seg;
    reg [15:0] fcw;

    // Internal State Machine
    typedef enum logic [3:0] {
        S_RESET,
        S_FETCH_W1,
        S_DECODE,
        S_FETCH_W2,
        S_EXECUTE,
        S_MEM_ACCESS,
        S_IO_ACCESS,
        S_WAIT_STEP
    } state_t;

    state_t state;
    reg [15:0] ir_word1;
    reg [15:0] ir_word2;

    // Linear Program Counter
    wire [23:0] current_pc = {1'b0, pc_seg, pc_offset};

    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            state <= S_RESET;
            mreq_n <= 1'b1;
            iorq_n <= 1'b1;
            rd_n <= 1'b1;
            wr_n <= 1'b1;
            status <= ST_HALT;
            pc_seg <= 7'd0;
            pc_offset <= 16'h0000;
            fcw <= 16'h4000; // System Mode
            byte_en <= 2'b11;
            addr <= 24'h000000;
            data_out <= 16'h0000;
            ir_word1 <= 16'h0000;
            ir_word2 <= 16'h0000;
            for (int i = 0; i < 16; i++) regs[i] <= 16'h0000;
        end else if (clk_en) begin
            case (state)
                S_RESET: begin
                    // Read initial PSA vector
                    pc_seg <= 7'd0;
                    pc_offset <= 16'h0800; // Jump to firmware reset vector
                    state <= S_FETCH_W1;
                end

                S_FETCH_W1: begin
                    addr <= {1'b0, pc_seg, pc_offset};
                    mreq_n <= 1'b0;
                    rd_n <= 1'b0;
                    wr_n <= 1'b1;
                    status <= ST_IFETCH;
                    byte_en <= 2'b11;
                    if (wait_n) begin
                        ir_word1 <= data_in;
                        pc_offset <= pc_offset + 2;
                        mreq_n <= 1'b1;
                        rd_n <= 1'b1;
                        state <= S_DECODE;
                    end
                end

                S_DECODE: begin
                    // Determine if 2nd word is needed (immediates / direct addresses)
                    if (ir_word1[15:12] == 4'h2 || ir_word1[15:12] == 4'h3 || ir_word1[15:8] == 8'h0C) begin
                        state <= S_FETCH_W2;
                    end else begin
                        state <= S_EXECUTE;
                    end
                end

                S_FETCH_W2: begin
                    addr <= {1'b0, pc_seg, pc_offset};
                    mreq_n <= 1'b0;
                    rd_n <= 1'b0;
                    status <= ST_IFETCH;
                    if (wait_n) begin
                        ir_word2 <= data_in;
                        pc_offset <= pc_offset + 2;
                        mreq_n <= 1'b1;
                        rd_n <= 1'b1;
                        state <= S_EXECUTE;
                    end
                end

                S_EXECUTE: begin
                    // Opcode execution logic
                    case (ir_word1[15:12])
                        4'h2: begin
                            // LD R_dst, imm16
                            regs[ir_word1[3:0]] <= ir_word2;
                            state <= S_FETCH_W1;
                        end
                        4'h3: begin
                            // OUTB / INB
                            if (ir_word1[11:8] == 4'hB) begin
                                // OUTB
                                addr <= {8'h00, ir_word2};
                                data_out <= {regs[ir_word1[3:0]][7:0], regs[ir_word1[3:0]][7:0]};
                                iorq_n <= 1'b0;
                                wr_n <= 1'b0;
                                status <= ST_IO_WR;
                                state <= S_IO_ACCESS;
                            end else begin
                                state <= S_FETCH_W1;
                            end
                        end
                        default: begin
                            state <= S_FETCH_W1;
                        end
                    endcase
                end

                S_IO_ACCESS: begin
                    if (wait_n) begin
                        iorq_n <= 1'b1;
                        wr_n <= 1'b1;
                        state <= S_FETCH_W1;
                    end
                end

                default: state <= S_FETCH_W1;
            endcase
        end
    end

endmodule
