`timescale 1ns / 1ps

module mc6847_core (
    input  wire        clk,          // Base clock (e.g., 25MHz or 100MHz with clk_en)
    input  wire        clk_en,       // 3.579545MHz approx pulse
    input  wire        reset_n,

    // Register / Mode inputs
    input  wire [7:0]  gmode,        // VDG control register

    // VRAM Interface (Master Read)
    output reg  [12:0] vram_addr,
    input  wire [7:0]  vram_data,

    // Video Output (Internal timing 256x192)
    output reg  [7:0]  pixel_r,
    output reg  [7:0]  pixel_g,
    output reg  [7:0]  pixel_b,
    output reg         pixel_valid,
    output reg         hsync,
    output reg         vsync,
    output reg         field_sync    // /FS interrupt for Z80 (active low)
);

    // Standard MC6847 Timing: 256 active pixels per line, 192 active lines
    // Line total ~ 384 clocks, Frame total ~ 262 lines
    localparam H_ACTIVE = 256;
    localparam H_TOTAL  = 384;
    localparam V_ACTIVE = 192;
    localparam V_TOTAL  = 262;

    reg [8:0] h_cnt;
    reg [8:0] v_cnt;

    // Palette: Green, Yellow, Blue, Red, Buff, Cyan, Magenta, Orange, Black
    localparam COLOR_BLACK   = 24'h000000;
    localparam COLOR_GREEN   = 24'h00E000;
    localparam COLOR_YELLOW  = 24'hE0E000;
    localparam COLOR_BLUE    = 24'h0000E0;
    localparam COLOR_RED     = 24'hE00000;
    localparam COLOR_BUFF    = 24'hE0E0E0;
    localparam COLOR_CYAN    = 24'h00E0E0;
    localparam COLOR_MAGENTA = 24'hE000E0;
    localparam COLOR_ORANGE  = 24'hE08000;

    // Internal CGROM simplified (standard ASCII 5x7 / 8x12 matrix)
    reg [7:0] cgrom [0:1023];
    initial begin
        // default clear
        for (int i = 0; i < 1024; i++) cgrom[i] = 8'h00;
        // Basic font patterns will be synthesized or loaded via ROM block
    end

    // Horizontal & Vertical counters
    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            h_cnt <= 0;
            v_cnt <= 0;
            field_sync <= 1'b1;
            hsync <= 1'b1;
            vsync <= 1'b1;
        end else if (clk_en) begin
            if (h_cnt == H_TOTAL - 1) begin
                h_cnt <= 0;
                if (v_cnt == V_TOTAL - 1) begin
                    v_cnt <= 0;
                end else begin
                    v_cnt <= v_cnt + 1;
                end
            end else begin
                h_cnt <= h_cnt + 1;
            end

            // Sync pulse generation
            hsync <= ~(h_cnt >= 280 && h_cnt < 312);
            vsync <= ~(v_cnt >= 210 && v_cnt < 213);
            field_sync <= ~(v_cnt >= 192 && v_cnt < 200);
        end
    end

    // Addressing VRAM for Text / Graphic mode
    wire [1:0] page = gmode[5:4];
    wire [11:0] page_offset = {page, 9'b0}; // page * 0x200
    wire [4:0] char_col = h_cnt[7:3];       // 0 to 31
    wire [3:0] char_row = v_cnt[7:4];       // 0 to 15 (each 12 lines in hardware map)
    wire [3:0] font_y   = v_cnt % 12;

    reg [7:0] cur_char;
    reg [7:0] cur_attr;
    reg [7:0] font_bits;

    always @(posedge clk) begin
        if (clk_en) begin
            if (h_cnt < H_ACTIVE && v_cnt < V_ACTIVE) begin
                pixel_valid <= 1'b1;
                
                // Pipeline stage for reading char and attr
                vram_addr <= page_offset + (v_cnt / 12) * 32 + char_col;
                cur_char  <= vram_data;
                
                // Font generation
                if (cur_char >= 8'h20 && cur_char < 8'h80) begin
                    font_bits <= cgrom[(cur_char - 8'h20) * 12 + font_y];
                end else begin
                    font_bits <= 8'h00;
                end

                // Invert check
                if (font_bits[7 - (h_cnt % 8)]) begin
                    {pixel_r, pixel_g, pixel_b} <= COLOR_GREEN;
                end else begin
                    {pixel_r, pixel_g, pixel_b} <= COLOR_BLACK;
                end
            end else begin
                pixel_valid <= 1'b0;
                {pixel_r, pixel_g, pixel_b} <= 24'h000000;
            end
        end
    end

endmodule
