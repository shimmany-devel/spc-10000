`timescale 1ns / 1ps

// Converts native video timing into an AXI4-Stream Video master stream
// for Zynq UltraScale+ DisplayPort Subsystem.
module axis_video_out #(
    parameter DATA_WIDTH = 24
)(
    input  wire                    aclk,
    input  wire                    aresetn,

    // Video Generator inputs
    input  wire [7:0]              pixel_r,
    input  wire [7:0]              pixel_g,
    input  wire [7:0]              pixel_b,
    input  wire                    pixel_valid,
    input  wire                    hsync,
    input  wire                    vsync,

    // AXI4-Stream Video Interface
    output wire [DATA_WIDTH-1:0]   m_axis_tdata,
    output wire                    m_axis_tvalid,
    input  wire                    m_axis_tready,
    output wire                    m_axis_tuser,   // SOF (Start of Frame)
    output wire                    m_axis_tlast    // EOL (End of Line)
);

    reg sof_reg;
    reg prev_vsync;
    reg eol_reg;
    reg prev_hsync;

    // Detect Frame Start (rising edge of vsync or transition to active)
    always @(posedge aclk) begin
        if (!aresetn) begin
            prev_vsync <= 1'b0;
            prev_hsync <= 1'b0;
            sof_reg    <= 1'b0;
            eol_reg    <= 1'b0;
        end else begin
            prev_vsync <= vsync;
            prev_hsync <= hsync;

            if (~prev_vsync && vsync) begin
                sof_reg <= 1'b1;
            end else if (pixel_valid && m_axis_tready) begin
                sof_reg <= 1'b0;
            end

            eol_reg <= (prev_hsync && ~hsync);
        end
    end

    assign m_axis_tdata  = {pixel_r, pixel_g, pixel_b};
    assign m_axis_tvalid = pixel_valid;
    assign m_axis_tuser  = sof_reg && pixel_valid;
    assign m_axis_tlast  = eol_reg && pixel_valid;

endmodule
