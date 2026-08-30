// =============================================================================
// Gowin rPLL & Clock Divider for Tang Nano 9K
// Input:  27.000 MHz (Onboard Crystal)
// Output: 126.000 MHz (5x Serial TMDS Clock)
//         25.200 MHz  (1x Pixel Clock for 640x480 @ 60Hz)
// =============================================================================

`timescale 1ns / 1ps

module gowin_rpll (
    input  wire clk_in,    // 27 MHz onboard
    input  wire rst_n,     // Active low reset
    output wire clk_p5,    // 126 MHz (5x pixel clock)
    output wire clk_pix,   // 25.2 MHz (1x pixel clock)
    output wire lock       // PLL lock flag
);

    wire pll_clkout;
    wire pll_lock;

    // Gowin rPLL Primitive instantiation
    // Fin = 27 MHz
    // Fout = Fin * (FBDIV_SEL + 1) / (IDIV_SEL + 1) = 27 * 14 / 3 = 126 MHz
    // Fvco = Fout * ODIV_SEL = 126 * 4 = 504 MHz (Within 400 - 1200 MHz VCO range)
    rPLL #(
        .FCLKIN("27.0"),
        .DEVICE("GW1N-9C"),
        .DYN_IDIV_SEL("false"),
        .IDIV_SEL(2),              // 3:1 division -> 9 MHz PFD
        .DYN_FBDIV_SEL("false"),
        .FBDIV_SEL(13),            // 14:1 multiplication -> 126 MHz
        .DYN_ODIV_SEL("false"),
        .ODIV_SEL(4),              // 504 MHz VCO
        .PSDA_SEL("0000"),
        .DYN_DA_EN("false"),
        .DUTYDA_SEL("1000"),
        .CLKOUT_FT_DIR(1'b1),
        .CLKOUTP_FT_DIR(1'b1),
        .CLKOUT_DLY_STEP(0),
        .CLKOUTP_DLY_STEP(0),
        .CLKFB_SEL("internal"),
        .CLKOUT_BYPASS("false"),
        .CLKOUTP_BYPASS("false"),
        .CLKOUTD_BYPASS("false"),
        .DYN_SDIV_SEL(2),
        .CLKOUTD_SRC("CLKOUT"),
        .CLKOUTD3_SRC("CLKOUT")
    ) u_rpll (
        .CLKOUT(pll_clkout),
        .LOCK(pll_lock),
        .CLKOUTP(),
        .CLKOUTD(),
        .CLKOUTD3(),
        .CLKIN(clk_in),
        .CLKFB(1'b0),
        .FBDSEL(6'b0),
        .IDSEL(6'b0),
        .ODSEL(6'b0),
        .DUTYDA(4'b0),
        .PSDA(4'b0),
        .FDLY(4'b0),
        .RESET(!rst_n),
        .RESET_P(1'b0)
    );

    assign clk_p5 = pll_clkout;
    assign lock   = pll_lock;

    // Gowin CLKDIV: Divides 126 MHz by 5 to generate synchronous 25.2 MHz pixel clock
    CLKDIV #(
        .DIV_MODE("5"),
        .GSREN("false")
    ) u_clkdiv (
        .HCLKIN(pll_clkout),
        .RESETN(pll_lock && rst_n),
        .CALIB(1'b1),
        .CLKOUT(clk_pix)
    );

endmodule
