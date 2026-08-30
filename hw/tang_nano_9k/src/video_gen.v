// =============================================================================
// Hardware Video Generator & Ternary UI Renderer (640x480 @ 60 Hz)
// Retro-Scientific Dark Theme with Hardware Geometric Glyphs
// =============================================================================

`timescale 1ns / 1ps

module video_gen (
    input  wire        clk_pix,       // 25.2 MHz Pixel Clock
    input  wire        rst_n,         // Active low reset
    input  wire [1:0]  mode,          // 0: 9-Trit Reg, 1: ALU Full-Adder, 2: Truth Table
    input  wire [17:0] reg_trits,     // 9 Trits (2 bits each)
    input  wire [1:0]  alu_a,         // Operand A
    input  wire [1:0]  alu_b,         // Operand B
    input  wire [1:0]  alu_cin,       // Carry In
    input  wire [1:0]  alu_res,       // ALU Result / Sum
    input  wire [1:0]  alu_cout,      // Carry Out
    input  wire [2:0]  alu_op,        // Active Operation
    input  wire [7:0]  last_uart_rx,  // Last received UART byte for telemetry
    input  wire [23:0] frame_tick,    // Heartbeat counter

    output reg         hsync,
    output reg         vsync,
    output reg         de,
    output reg  [7:0]  pixel_r,
    output reg  [7:0]  pixel_g,
    output reg  [7:0]  pixel_b
);

    // -------------------------------------------------------------------------
    // VGA 640x480 @ 60 Hz Standard Timings
    // -------------------------------------------------------------------------
    localparam H_ACTIVE = 640;
    localparam H_FRONT  = 16;
    localparam H_SYNC   = 96;
    localparam H_BACK   = 48;
    localparam H_TOTAL  = 800;

    localparam V_ACTIVE = 480;
    localparam V_FRONT  = 10;
    localparam V_SYNC   = 2;
    localparam V_BACK   = 33;
    localparam V_TOTAL  = 525;

    reg [9:0] h_cnt;
    reg [9:0] v_cnt;

    wire active_area = (h_cnt < H_ACTIVE) && (v_cnt < V_ACTIVE);

    always @(posedge clk_pix or negedge rst_n) begin
        if (!rst_n) begin
            h_cnt <= 10'd0;
            v_cnt <= 10'd0;
            hsync <= 1'b1;
            vsync <= 1'b1;
            de    <= 1'b0;
        end else begin
            // Horizontal counter
            if (h_cnt == H_TOTAL - 1) begin
                h_cnt <= 10'd0;
                // Vertical counter
                if (v_cnt == V_TOTAL - 1)
                    v_cnt <= 10'd0;
                else
                    v_cnt <= v_cnt + 1'b1;
            end else begin
                h_cnt <= h_cnt + 1'b1;
            end

            // Standard Negative-polarity sync pulses
            hsync <= !((h_cnt >= (H_ACTIVE + H_FRONT)) && (h_cnt < (H_ACTIVE + H_FRONT + H_SYNC)));
            vsync <= !((v_cnt >= (V_ACTIVE + V_FRONT)) && (v_cnt < (V_ACTIVE + V_FRONT + V_SYNC)));
            de    <= active_area;
        end
    end

    // -------------------------------------------------------------------------
    // Color Palette Definitions (RGB888)
    // -------------------------------------------------------------------------
    localparam [23:0] COLOR_BG         = 24'h0C101A; // Dark Obsidian Navy
    localparam [23:0] COLOR_PANEL_BG   = 24'h141A28; // Panel Background
    localparam [23:0] COLOR_BORDER     = 24'h00DCD2; // Neon Cyan
    localparam [23:0] COLOR_BORDER_DIM = 24'h24344E; // Dark Slate Blue
    localparam [23:0] COLOR_TEXT_WHITE = 24'hFFFFFF; // Crisp White
    localparam [23:0] COLOR_TEXT_MUTED = 24'h7085A0; // Muted Cyan-Gray

    // Trit Specific Colors
    // +1 (P) : Emerald Glow
    localparam [23:0] COLOR_TRIT_P_BG  = 24'h0D4424;
    localparam [23:0] COLOR_TRIT_P_BRD = 24'h20E050;
    localparam [23:0] COLOR_TRIT_P_GLY = 24'h70FFA0;

    // 0  (U) : Charcoal / Amber
    localparam [23:0] COLOR_TRIT_U_BG  = 24'h282830;
    localparam [23:0] COLOR_TRIT_U_BRD = 24'hC0A020;
    localparam [23:0] COLOR_TRIT_U_GLY = 24'hFFE060;

    // -1 (N) : Crimson Glow
    localparam [23:0] COLOR_TRIT_N_BG  = 24'h4A1018;
    localparam [23:0] COLOR_TRIT_N_BRD = 24'hE82838;
    localparam [23:0] COLOR_TRIT_N_GLY = 24'hFF7080;

    // -------------------------------------------------------------------------
    // Pixel Coordinate Aliases
    // -------------------------------------------------------------------------
    wire [9:0] px = h_cnt;
    wire [9:0] py = v_cnt;

    // -------------------------------------------------------------------------
    // Helper: Trit Color Function
    // -------------------------------------------------------------------------
    function [23:0] get_trit_bg(input [1:0] t);
        case (t)
            2'b01:   get_trit_bg = COLOR_TRIT_P_BG;
            2'b10:   get_trit_bg = COLOR_TRIT_N_BG;
            default: get_trit_bg = COLOR_TRIT_U_BG;
        endcase
    endfunction

    function [23:0] get_trit_border(input [1:0] t);
        case (t)
            2'b01:   get_trit_border = COLOR_TRIT_P_BRD;
            2'b10:   get_trit_border = COLOR_TRIT_N_BRD;
            default: get_trit_border = COLOR_TRIT_U_BRD;
        endcase
    endfunction

    function [23:0] get_trit_glyph_col(input [1:0] t);
        case (t)
            2'b01:   get_trit_glyph_col = COLOR_TRIT_P_GLY;
            2'b10:   get_trit_glyph_col = COLOR_TRIT_N_GLY;
            default: get_trit_glyph_col = COLOR_TRIT_U_GLY;
        endcase
    endfunction

    // -------------------------------------------------------------------------
    // Glyph Functions: Render '+' , '0' , '-' inside a given bounding box
    // -------------------------------------------------------------------------
    function is_trit_glyph(
        input [9:0] x, input [9:0] y,
        input [9:0] cx, input [9:0] cy,
        input [1:0] t
    );
        reg signed [9:0] dx;
        reg signed [9:0] dy;
        begin
            dx = x - cx;
            dy = y - cy;
            case (t)
                2'b01: begin
                    // Plus Glyph '+'
                    is_trit_glyph = ((dx >= -16 && dx <= 16 && dy >= -4 && dy <= 4) ||
                                     (dy >= -16 && dy <= 16 && dx >= -4 && dx <= 4));
                end
                2'b10: begin
                    // Minus Glyph '-'
                    is_trit_glyph = (dx >= -16 && dx <= 16 && dy >= -4 && dy <= 4);
                end
                default: begin
                    // Zero Glyph '0' (Square Ring)
                    is_trit_glyph = ((dx >= -14 && dx <= 14 && dy >= -14 && dy <= 14) &&
                                    !(dx >= -8  && dx <= 8  && dy >= -8  && dy <= 8));
                end
            endcase
        end
    endfunction

    // -------------------------------------------------------------------------
    // UI Layout Rendering Stages
    // -------------------------------------------------------------------------
    reg [23:0] rgb_out;

    // Header Region Elements
    wire in_header_box   = (px >= 20 && px <= 620 && py >= 12 && py <= 52);
    wire in_header_brd   = in_header_box && (px <= 22 || px >= 618 || py <= 14 || py >= 50);
    wire in_heartbeat    = (px >= 580 && px <= 606 && py >= 20 && py <= 44);
    wire in_mode_badge   = (px >= 32 && px <= 230 && py >= 20 && py <= 44);
    wire in_mode_badge_b = in_mode_badge && (px <= 34 || px >= 228 || py <= 22 || py >= 42);

    // Mode 1: 9-Trit Register Coordinates
    // 9 Boxes: index 0..8, each box 54x90 px
    integer i;
    reg in_m1_box;
    reg in_m1_brd;
    reg in_m1_gly;
    reg [1:0] m1_active_trit;

    always @(*) begin
        in_m1_box      = 1'b0;
        in_m1_brd      = 1'b0;
        in_m1_gly      = 1'b0;
        m1_active_trit = 2'b00;

        for (i = 0; i < 9; i = i + 1) begin
            // x from (40 + i*63) to (40 + i*63 + 54)
            if (px >= (40 + i * 63) && px <= (40 + i * 63 + 54) && py >= 120 && py <= 210) begin
                in_m1_box      = 1'b1;
                m1_active_trit = reg_trits[i*2 +: 2];
                if (px <= (40 + i * 63 + 2) || px >= (40 + i * 63 + 52) ||
                    py <= 122 || py >= 208) begin
                    in_m1_brd = 1'b1;
                end
                if (is_trit_glyph(px, py, 40 + i * 63 + 27, 165, m1_active_trit)) begin
                    in_m1_gly = 1'b1;
                end
            end
        end
    end

    // Mode 1 Info & Summary Card (py: 240..450)
    wire in_m1_card     = (px >= 40 && px <= 600 && py >= 240 && py <= 450);
    wire in_m1_card_brd = in_m1_card && (px <= 42 || px >= 598 || py <= 242 || py >= 448);

    // Mode 2: ALU / Full Adder Display
    // Box A:      px: 60..180,  py: 100..220
    // Plus '+':   px: 205..235, py: 145..175
    // Box B:      px: 260..380, py: 100..220
    // Equals '=': px: 405..435, py: 145..175
    // Box Sum:    px: 460..580, py: 100..220
    // Carry In:   px: 160..280, py: 260..360
    // Carry Out:  px: 360..480, py: 260..360

    wire in_alu_a_box   = (px >= 60  && px <= 180 && py >= 100 && py <= 220);
    wire in_alu_a_brd   = in_alu_a_box && (px <= 63 || px >= 177 || py <= 103 || py >= 217);
    wire in_alu_a_gly   = in_alu_a_box && is_trit_glyph(px, py, 120, 160, alu_a);

    wire in_alu_b_box   = (px >= 260 && px <= 380 && py >= 100 && py <= 220);
    wire in_alu_b_brd   = in_alu_b_box && (px <= 263 || px >= 377 || py <= 103 || py >= 217);
    wire in_alu_b_gly   = in_alu_b_box && is_trit_glyph(px, py, 320, 160, alu_b);

    wire in_alu_sum_box = (px >= 460 && px <= 580 && py >= 100 && py <= 220);
    wire in_alu_sum_brd = in_alu_sum_box && (px <= 463 || px >= 577 || py <= 103 || py >= 217);
    wire in_alu_sum_gly = in_alu_sum_box && is_trit_glyph(px, py, 520, 160, alu_res);

    wire in_alu_cin_box = (px >= 160 && px <= 280 && py >= 260 && py <= 360);
    wire in_alu_cin_brd = in_alu_cin_box && (px <= 163 || px >= 277 || py <= 263 || py >= 357);
    wire in_alu_cin_gly = in_alu_cin_box && is_trit_glyph(px, py, 220, 310, alu_cin);

    wire in_alu_cou_box = (px >= 360 && px <= 480 && py >= 260 && py <= 360);
    wire in_alu_cou_brd = in_alu_cou_box && (px <= 363 || px >= 477 || py <= 263 || py >= 357);
    wire in_alu_cou_gly = in_alu_cou_box && is_trit_glyph(px, py, 420, 310, alu_cout);

    // Operator symbols in Mode 2
    wire in_op_plus = ((px >= 210 && px <= 230 && py >= 157 && py <= 163) ||
                       (px >= 217 && px <= 223 && py >= 150 && py <= 170));
    wire in_op_eq   = (px >= 410 && px <= 430 && ((py >= 153 && py <= 157) || (py >= 163 && py <= 167)));

    // Mode 2 Bottom Telemetry Card
    wire in_alu_footer     = (px >= 60 && px <= 580 && py >= 390 && py <= 455);
    wire in_alu_footer_brd = in_alu_footer && (px <= 62 || px >= 578 || py <= 392 || py >= 453);

    // -------------------------------------------------------------------------
    // Main Rendering Combinational Logic
    // -------------------------------------------------------------------------
    always @(*) begin
        rgb_out = COLOR_BG;

        // Subtle background grid effect
        if ((px[4:0] == 5'd0) || (py[4:0] == 5'd0)) begin
            rgb_out = COLOR_PANEL_BG;
        end

        // 1. Render Persistent Header Bar
        if (in_header_box) begin
            if (in_header_brd) begin
                rgb_out = COLOR_BORDER;
            end else if (in_heartbeat) begin
                rgb_out = frame_tick[23] ? COLOR_TRIT_P_BRD : COLOR_PANEL_BG;
            end else if (in_mode_badge) begin
                if (in_mode_badge_b)
                    rgb_out = COLOR_BORDER;
                else
                    rgb_out = (mode == 2'b00) ? 24'h103850 :
                              (mode == 2'b01) ? 24'h402050 : 24'h104030;
            end else begin
                rgb_out = 24'h121824;
            end
        end

        // 2. Mode-Specific Render Branches
        else if (mode == 2'b00) begin
            // MODE 1: 9-Trit Register Matrix
            if (in_m1_box) begin
                if (in_m1_gly)
                    rgb_out = get_trit_glyph_col(m1_active_trit);
                else if (in_m1_brd)
                    rgb_out = get_trit_border(m1_active_trit);
                else
                    rgb_out = get_trit_bg(m1_active_trit);
            end else if (in_m1_card) begin
                if (in_m1_card_brd)
                    rgb_out = COLOR_BORDER_DIM;
                else begin
                    // Telemetry card interior
                    rgb_out = COLOR_PANEL_BG;
                    // Horizontal accent bar
                    if (py >= 290 && py <= 292 && px >= 60 && px <= 580)
                        rgb_out = COLOR_BORDER;
                    // UART status dot
                    if (px >= 70 && px <= 85 && py >= 415 && py <= 430)
                        rgb_out = (last_uart_rx != 8'd0) ? COLOR_TRIT_P_BRD : COLOR_TRIT_N_BRD;
                end
            end
        end else if (mode == 2'b01) begin
            // MODE 2: Balanced ALU / Full Adder Live View
            if (in_alu_a_box) begin
                if (in_alu_a_gly)      rgb_out = get_trit_glyph_col(alu_a);
                else if (in_alu_a_brd) rgb_out = get_trit_border(alu_a);
                else                   rgb_out = get_trit_bg(alu_a);
            end else if (in_alu_b_box) begin
                if (in_alu_b_gly)      rgb_out = get_trit_glyph_col(alu_b);
                else if (in_alu_b_brd) rgb_out = get_trit_border(alu_b);
                else                   rgb_out = get_trit_bg(alu_b);
            end else if (in_alu_sum_box) begin
                if (in_alu_sum_gly)      rgb_out = get_trit_glyph_col(alu_res);
                else if (in_alu_sum_brd) rgb_out = get_trit_border(alu_res);
                else                     rgb_out = get_trit_bg(alu_res);
            end else if (in_alu_cin_box) begin
                if (in_alu_cin_gly)      rgb_out = get_trit_glyph_col(alu_cin);
                else if (in_alu_cin_brd) rgb_out = get_trit_border(alu_cin);
                else                     rgb_out = get_trit_bg(alu_cin);
            end else if (in_alu_cou_box) begin
                if (in_alu_cou_gly)      rgb_out = get_trit_glyph_col(alu_cout);
                else if (in_alu_cou_brd) rgb_out = get_trit_border(alu_cout);
                else                     rgb_out = get_trit_bg(alu_cout);
            end else if (in_op_plus || in_op_eq) begin
                rgb_out = COLOR_TEXT_WHITE;
            end else if (in_alu_footer) begin
                if (in_alu_footer_brd)
                    rgb_out = COLOR_BORDER_DIM;
                else
                    rgb_out = COLOR_PANEL_BG;
            end
        end else begin
            // MODE 3: Default Truth Table View
            if (px >= 100 && px <= 540 && py >= 100 && py <= 420) begin
                if (px <= 103 || px >= 537 || py <= 103 || py >= 417)
                    rgb_out = COLOR_BORDER;
                else
                    rgb_out = COLOR_PANEL_BG;
            end
        end
    end

    // -------------------------------------------------------------------------
    // Pixel Output Pipeline
    // -------------------------------------------------------------------------
    always @(posedge clk_pix or negedge rst_n) begin
        if (!rst_n) begin
            pixel_r <= 8'd0;
            pixel_g <= 8'd0;
            pixel_b <= 8'd0;
        end else if (active_area) begin
            pixel_r <= rgb_out[23:16];
            pixel_g <= rgb_out[15:8];
            pixel_b <= rgb_out[7:0];
        end else begin
            pixel_r <= 8'd0;
            pixel_g <= 8'd0;
            pixel_b <= 8'd0;
        end
    end

endmodule

