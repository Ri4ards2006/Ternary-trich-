// =============================================================================
// DVI / TMDS Serializer and Differential PHY for Gowin GW1N-9C
// Uses hardware OSER10 (10:1 DDR Serializer) and ELVDS_OBUF primitives
// =============================================================================

`timescale 1ns / 1ps

module dvi_transmitter (
    input  wire       clk_p5,       // 126 MHz Serial Clock (5x Pixel Clock)
    input  wire       clk_pix,      // 25.2 MHz Pixel Clock
    input  wire       rst_n,        // Active Low Reset
    input  wire [9:0] tmds_r,       // Red TMDS 10-bit word
    input  wire [9:0] tmds_g,       // Green TMDS 10-bit word
    input  wire [9:0] tmds_b,       // Blue TMDS 10-bit word
    output wire       tmds_clk_p,   // HDMI Clock +
    output wire       tmds_clk_n,   // HDMI Clock -
    output wire [2:0] tmds_d_p,     // HDMI Data [2:0] + (R, G, B)
    output wire [2:0] tmds_d_n      // HDMI Data [2:0] - (R, G, B)
);

    wire ser_clk;
    wire ser_r;
    wire ser_g;
    wire ser_b;

    // TMDS Clock pattern: 5 ones, 5 zeros = 1x clock frequency
    wire [9:0] tmds_clk_word = 10'b1111100000;

    // Serializer for Clock Channel
    OSER10 #(
        .GSREN("false"),
        .LSREN("true")
    ) u_oser_clk (
        .D0(tmds_clk_word[0]),
        .D1(tmds_clk_word[1]),
        .D2(tmds_clk_word[2]),
        .D3(tmds_clk_word[3]),
        .D4(tmds_clk_word[4]),
        .D5(tmds_clk_word[5]),
        .D6(tmds_clk_word[6]),
        .D7(tmds_clk_word[7]),
        .D8(tmds_clk_word[8]),
        .D9(tmds_clk_word[9]),
        .FCLK(clk_p5),
        .PCLK(clk_pix),
        .RESET(!rst_n),
        .Q(ser_clk)
    );

    // Serializer for Data Channel 0 (Blue)
    OSER10 #(
        .GSREN("false"),
        .LSREN("true")
    ) u_oser_b (
        .D0(tmds_b[0]),
        .D1(tmds_b[1]),
        .D2(tmds_b[2]),
        .D3(tmds_b[3]),
        .D4(tmds_b[4]),
        .D5(tmds_b[5]),
        .D6(tmds_b[6]),
        .D7(tmds_b[7]),
        .D8(tmds_b[8]),
        .D9(tmds_b[9]),
        .FCLK(clk_p5),
        .PCLK(clk_pix),
        .RESET(!rst_n),
        .Q(ser_b)
    );

    // Serializer for Data Channel 1 (Green)
    OSER10 #(
        .GSREN("false"),
        .LSREN("true")
    ) u_oser_g (
        .D0(tmds_g[0]),
        .D1(tmds_g[1]),
        .D2(tmds_g[2]),
        .D3(tmds_g[3]),
        .D4(tmds_g[4]),
        .D5(tmds_g[5]),
        .D6(tmds_g[6]),
        .D7(tmds_g[7]),
        .D8(tmds_g[8]),
        .D9(tmds_g[9]),
        .FCLK(clk_p5),
        .PCLK(clk_pix),
        .RESET(!rst_n),
        .Q(ser_g)
    );

    // Serializer for Data Channel 2 (Red)
    OSER10 #(
        .GSREN("false"),
        .LSREN("true")
    ) u_oser_r (
        .D0(tmds_r[0]),
        .D1(tmds_r[1]),
        .D2(tmds_r[2]),
        .D3(tmds_r[3]),
        .D4(tmds_r[4]),
        .D5(tmds_r[5]),
        .D6(tmds_r[6]),
        .D7(tmds_r[7]),
        .D8(tmds_r[8]),
        .D9(tmds_r[9]),
        .FCLK(clk_p5),
        .PCLK(clk_pix),
        .RESET(!rst_n),
        .Q(ser_r)
    );

    // Differential Output Buffers (ELVDS_OBUF)
    ELVDS_OBUF u_obuf_clk (
        .I(ser_clk),
        .O(tmds_clk_p),
        .OB(tmds_clk_n)
    );

    ELVDS_OBUF u_obuf_d0 (
        .I(ser_b),
        .O(tmds_d_p[0]),
        .OB(tmds_d_n[0])
    );

    ELVDS_OBUF u_obuf_d1 (
        .I(ser_g),
        .O(tmds_d_p[1]),
        .OB(tmds_d_n[1])
    );

    ELVDS_OBUF u_obuf_d2 (
        .I(ser_r),
        .O(tmds_d_p[2]),
        .OB(tmds_d_n[2])
    );

endmodule

