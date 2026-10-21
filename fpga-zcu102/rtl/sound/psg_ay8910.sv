`timescale 1ns / 1ps

// AY-3-8910 (PSG) Synthesizer Core with PWM D/A Output
module psg_ay8910 (
    input  wire        clk,
    input  wire        clk_en,      // 3.579545 / 2 or 1.789772 MHz enable pulse
    input  wire        reset_n,
    input  wire        wr_en,
    input  wire [3:0]  addr,
    input  wire [7:0]  din,
    output wire [7:0]  dout,
    output wire        audio_pwm_l,
    output wire        audio_pwm_r
);

    reg [7:0] regs [0:15];
    integer k;

    // Registers write/read
    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            for (k = 0; k < 16; k = k + 1) regs[k] <= 8'h00;
        end else if (wr_en) begin
            regs[addr] <= din;
        end
    end

    assign dout = regs[addr];

    // Frequency generators for Ch A, B, C
    wire [11:0] tone_period_a = {regs[1][3:0], regs[0]};
    wire [11:0] tone_period_b = {regs[3][3:0], regs[2]};
    wire [11:0] tone_period_c = {regs[5][3:0], regs[4]};

    reg [11:0] cnt_a, cnt_b, cnt_c;
    reg out_a, out_b, out_c;

    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            cnt_a <= 0; out_a <= 0;
            cnt_b <= 0; out_b <= 0;
            cnt_c <= 0; out_c <= 0;
        end else if (clk_en) begin
            // Channel A
            if (tone_period_a == 0 || cnt_a >= tone_period_a) begin
                cnt_a <= 0;
                out_a <= ~out_a;
            end else begin
                cnt_a <= cnt_a + 1'b1;
            end

            // Channel B
            if (tone_period_b == 0 || cnt_b >= tone_period_b) begin
                cnt_b <= 0;
                out_b <= ~out_b;
            end else begin
                cnt_b <= cnt_b + 1'b1;
            end

            // Channel C
            if (tone_period_c == 0 || cnt_c >= tone_period_c) begin
                cnt_c <= 0;
                out_c <= ~out_c;
            end else begin
                cnt_c <= cnt_c + 1'b1;
            end
        end
    end

    // Mixer
    wire enable_tone_a = ~regs[7][0];
    wire enable_tone_b = ~regs[7][1];
    wire enable_tone_c = ~regs[7][2];

    wire sig_a = enable_tone_a ? out_a : 1'b1;
    wire sig_b = enable_tone_b ? out_b : 1'b1;
    wire sig_c = enable_tone_c ? out_c : 1'b1;

    // Amplitude calculation (4-bit volumes)
    wire [3:0] vol_a = regs[8][3:0];
    wire [3:0] vol_b = regs[9][3:0];
    wire [3:0] vol_c = regs[10][3:0];

    wire [7:0] dac_a = sig_a ? {vol_a, 4'h0} : 8'h00;
    wire [7:0] dac_b = sig_b ? {vol_b, 4'h0} : 8'h00;
    wire [7:0] dac_c = sig_c ? {vol_c, 4'h0} : 8'h00;

    // Summing channels
    wire [9:0] dac_mix = dac_a + dac_b + dac_c;

    // Delta-Sigma PWM Converter (8-bit resolution)
    reg [8:0] pwm_acc_l, pwm_acc_r;
    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            pwm_acc_l <= 0;
            pwm_acc_r <= 0;
        end else begin
            pwm_acc_l <= pwm_acc_l[7:0] + dac_mix[9:2];
            pwm_acc_r <= pwm_acc_r[7:0] + dac_mix[9:2];
        end
    end

    assign audio_pwm_l = pwm_acc_l[8];
    assign audio_pwm_r = pwm_acc_r[8];

endmodule
