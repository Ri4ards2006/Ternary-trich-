// =============================================================================
// TERNARYRATOR — FPGA Balanced Ternary DVI Visualizer & Hardware Core
// Target:  Sipeed Tang Nano 9K (Gowin GW1NR-LV9QN88PC6/I5)
// Toolchain: Yosys + NextPNR-Himbaechel + Apycula + openFPGALoader
// =============================================================================

`timescale 1ns / 1ps

module top (
    input  wire       clk,          // 27 MHz onboard oscillator (Pin 52)
    input  wire       btn_rst_n,    // Active low reset button S2 (Pin 3)
    input  wire       uart_rx,      // UART RX line from USB-Bridge (Pin 18)

    // DVI / HDMI Differential Output Pins
    output wire       tmds_clk_p,   // Pin 69
    output wire       tmds_clk_n,   // Pin 68
    output wire [2:0] tmds_d_p,     // Pins 75 (R), 73 (G), 71 (B)
    output wire [2:0] tmds_d_n,     // Pins 74 (R), 72 (G), 70 (B)

    // Diagnostic LEDs (Active Low)
    output wire [5:0] led           // Pins 10, 11, 13, 14, 15, 16
);

    // -------------------------------------------------------------------------
    // 1. Clock Generation via Gowin rPLL
    // -------------------------------------------------------------------------
    wire clk_p5;      // 126.0 MHz fast serialization clock
    wire clk_pix;     // 25.2 MHz standard pixel clock (640x480 @ 60 Hz)
    wire pll_lock;

    gowin_rpll u_pll (
        .clk_in(clk),
        .rst_n(btn_rst_n),
        .clk_p5(clk_p5),
        .clk_pix(clk_pix),
        .lock(pll_lock)
    );

    // Synchronous reset generation
    reg [2:0] rst_sync;
    always @(posedge clk_pix or negedge pll_lock) begin
        if (!pll_lock)
            rst_sync <= 3'b000;
        else
            rst_sync <= {rst_sync[1:0], btn_rst_n};
    end
    wire sys_rst_n = rst_sync[2];

    // -------------------------------------------------------------------------
    // 2. UART Receiver (115200 Baud @ 25.2 MHz Pixel Clock)
    // -------------------------------------------------------------------------
    wire [7:0] rx_byte;
    wire       rx_valid;

    uart_rx #(
        .CLK_FREQ(25200000),
        .BAUD_RATE(115200)
    ) u_uart (
        .clk(clk_pix),
        .rst_n(sys_rst_n),
        .rx_pin(uart_rx),
        .rx_data(rx_byte),
        .rx_valid(rx_valid)
    );

    // -------------------------------------------------------------------------
    // 3. Register Bank & UART Command Decoder
    // -------------------------------------------------------------------------
    reg [1:0]  mode_reg;        // 0: Mode 1 (9-Trit Reg), 1: Mode 2 (ALU), 2: Mode 3 (Truth)
    reg [17:0] reg_trits;       // 9 Trits (2 bits each: 00=0, 01=+1, 10=-1)
    reg [1:0]  alu_a_reg;       // ALU Operand A
    reg [1:0]  alu_b_reg;       // ALU Operand B
    reg [1:0]  alu_cin_reg;     // ALU Carry In
    reg [2:0]  alu_op_reg;      // ALU Operation (0=ADD, 1=MIN, 2=MAX, 3=INV, 4=XOR, 5=CYC)
    reg [7:0]  last_rx_reg;
    reg [23:0] frame_counter;
    reg [19:0] uart_activity_timer;

    // Helper: Cycle a 2B1T trit (-1 -> 0 -> +1 -> -1)
    function [1:0] cycle_trit(input [1:0] in_t);
        case (in_t)
            2'b10:   cycle_trit = 2'b00; // -1 ->  0
            2'b00:   cycle_trit = 2'b01; //  0 -> +1
            2'b01:   cycle_trit = 2'b10; // +1 -> -1
            default: cycle_trit = 2'b00;
        endcase
    endfunction

    always @(posedge clk_pix or negedge sys_rst_n) begin
        if (!sys_rst_n) begin
            mode_reg            <= 2'b00; // Start in Mode 1
            // Initial sample pattern: [-1, 0, +1, -1, 0, +1, -1, 0, +1]
            reg_trits           <= {2'b01, 2'b00, 2'b10, 2'b01, 2'b00, 2'b10, 2'b01, 2'b00, 2'b10};
            alu_a_reg           <= 2'b01; // +1
            alu_b_reg           <= 2'b10; // -1
            alu_cin_reg         <= 2'b00; // 0
            alu_op_reg          <= 3'd0;  // ADD
            last_rx_reg         <= 8'd0;
            frame_counter       <= 24'd0;
            uart_activity_timer <= 20'd0;
        end else begin
            frame_counter <= frame_counter + 1'b1;

            if (uart_activity_timer > 0)
                uart_activity_timer <= uart_activity_timer - 1'b1;

            if (rx_valid) begin
                last_rx_reg         <= rx_byte;
                uart_activity_timer <= 20'd1000000; // ~40 ms activity flash

                // Mode switching commands
                if (rx_byte == 8'h01 || rx_byte == "1") begin
                    mode_reg <= 2'b00; // Mode 1: 9-Trit Matrix
                end else if (rx_byte == 8'h02 || rx_byte == "2") begin
                    mode_reg <= 2'b01; // Mode 2: ALU / Full Adder
                end else if (rx_byte == 8'h03 || rx_byte == "3") begin
                    mode_reg <= 2'b10; // Mode 3: Truth Tables
                end

                // Interactive ASCII controls (PuTTY / Serial Terminal)
                else if (rx_byte == "a" || rx_byte == "A") begin
                    alu_a_reg <= cycle_trit(alu_a_reg);
                end else if (rx_byte == "b" || rx_byte == "B") begin
                    alu_b_reg <= cycle_trit(alu_b_reg);
                end else if (rx_byte == "c" || rx_byte == "C") begin
                    alu_cin_reg <= cycle_trit(alu_cin_reg);
                end else if (rx_byte == "o" || rx_byte == "O") begin
                    alu_op_reg <= (alu_op_reg == 3'd5) ? 3'd0 : (alu_op_reg + 1'b1);
                end else if (rx_byte == "+" || rx_byte == "p" || rx_byte == "P") begin
                    // Shift in +1
                    reg_trits <= {reg_trits[15:0], 2'b01};
                end else if (rx_byte == "0" || rx_byte == "u" || rx_byte == "U") begin
                    // Shift in 0
                    reg_trits <= {reg_trits[15:0], 2'b00};
                end else if (rx_byte == "-" || rx_byte == "n" || rx_byte == "N") begin
                    // Shift in -1
                    reg_trits <= {reg_trits[15:0], 2'b10};
                end else if (rx_byte == "r" || rx_byte == "R") begin
                    // Reset all trits to 0
                    reg_trits   <= 18'd0;
                    alu_a_reg   <= 2'b00;
                    alu_b_reg   <= 2'b00;
                    alu_cin_reg <= 2'b00;
                end else if (rx_byte == "s" || rx_byte == "S") begin
                    // Rotate register
                    reg_trits <= {reg_trits[1:0], reg_trits[17:2]};
                end

                // Binary Command Packets:
                // Format 1: 8'b1000_ii_vv (ii: 0=A, 1=B, 2=Cin; vv: 2B1T val)
                else if (rx_byte[7:4] == 4'b1000) begin
                    case (rx_byte[3:2])
                        2'b00: alu_a_reg   <= rx_byte[1:0];
                        2'b01: alu_b_reg   <= rx_byte[1:0];
                        2'b10: alu_cin_reg <= rx_byte[1:0];
                        default: ;
                    endcase
                end
                // Format 2: 8'b0100_iiii with 2B1T in lowest bits (direct index set)
                else if (rx_byte[7:6] == 2'b01) begin
                    reg_trits[rx_byte[5:2]*2 +: 2] <= rx_byte[1:0];
                end
            end
        end
    end

    // -------------------------------------------------------------------------
    // 4. Balanced Ternary ALU Core
    // -------------------------------------------------------------------------
    wire [1:0] alu_result;
    wire [1:0] alu_carry_out;

    ternary_alu u_alu (
        .a(alu_a_reg),
        .b(alu_b_reg),
        .c_in(alu_cin_reg),
        .op(alu_op_reg),
        .result(alu_result),
        .c_out(alu_carry_out)
    );

    // -------------------------------------------------------------------------
    // 5. Video Generation & OSD UI Engine
    // -------------------------------------------------------------------------
    wire       vga_hsync;
    wire       vga_vsync;
    wire       vga_de;
    wire [7:0] vga_r;
    wire [7:0] vga_g;
    wire [7:0] vga_b;

    video_gen u_video (
        .clk_pix(clk_pix),
        .rst_n(sys_rst_n),
        .mode(mode_reg),
        .reg_trits(reg_trits),
        .alu_a(alu_a_reg),
        .alu_b(alu_b_reg),
        .alu_cin(alu_cin_reg),
        .alu_res(alu_result),
        .alu_cout(alu_carry_out),
        .alu_op(alu_op_reg),
        .last_uart_rx(last_rx_reg),
        .frame_tick(frame_counter),
        .hsync(vga_hsync),
        .vsync(vga_vsync),
        .de(vga_de),
        .pixel_r(vga_r),
        .pixel_g(vga_g),
        .pixel_b(vga_b)
    );

    // -------------------------------------------------------------------------
    // 6. TMDS 8b/10b Encoders (R, G, B channels)
    // -------------------------------------------------------------------------
    wire [9:0] tmds_r_word;
    wire [9:0] tmds_g_word;
    wire [9:0] tmds_b_word;

    // Blue channel carries HSYNC (c0) and VSYNC (c1) during blanking
    tmds_encoder u_enc_b (
        .clk(clk_pix),
        .rst_n(sys_rst_n),
        .data_in(vga_b),
        .c0(vga_hsync),
        .c1(vga_vsync),
        .de(vga_de),
        .tmds_out(tmds_b_word)
    );

    // Green channel
    tmds_encoder u_enc_g (
        .clk(clk_pix),
        .rst_n(sys_rst_n),
        .data_in(vga_g),
        .c0(1'b0),
        .c1(1'b0),
        .de(vga_de),
        .tmds_out(tmds_g_word)
    );

    // Red channel
    tmds_encoder u_enc_r (
        .clk(clk_pix),
        .rst_n(sys_rst_n),
        .data_in(vga_r),
        .c0(1'b0),
        .c1(1'b0),
        .de(vga_de),
        .tmds_out(tmds_r_word)
    );

    // -------------------------------------------------------------------------
    // 7. DVI Hardware Transmitter (OSER10 + ELVDS_OBUF)
    // -------------------------------------------------------------------------
    dvi_transmitter u_dvi_tx (
        .clk_p5(clk_p5),
        .clk_pix(clk_pix),
        .rst_n(sys_rst_n),
        .tmds_r(tmds_r_word),
        .tmds_g(tmds_g_word),
        .tmds_b(tmds_b_word),
        .tmds_clk_p(tmds_clk_p),
        .tmds_clk_n(tmds_clk_n),
        .tmds_d_p(tmds_d_p),
        .tmds_d_n(tmds_d_n)
    );

    // -------------------------------------------------------------------------
    // 8. Diagnostic LEDs (Active Low)
    // -------------------------------------------------------------------------
    assign led[0] = !pll_lock;                          // LED0: PLL Lock status (ON when locked)
    assign led[1] = !frame_counter[23];                 // LED1: Heartbeat blink (~1.5 Hz)
    assign led[2] = !(uart_activity_timer > 0);         // LED2: UART RX Activity flash
    assign led[3] = (mode_reg == 2'b00) ? 1'b0 : 1'b1;  // LED3: Mode 1 indicator
    assign led[4] = (mode_reg == 2'b01) ? 1'b0 : 1'b1;  // LED4: Mode 2 indicator
    assign led[5] = (mode_reg == 2'b10) ? 1'b0 : 1'b1;  // LED5: Mode 3 indicator

endmodule

