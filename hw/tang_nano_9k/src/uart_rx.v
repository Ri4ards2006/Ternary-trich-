// =============================================================================
// Robust UART Receiver (8-N-1)
// Parameterized Clock Frequency and Baud Rate
// =============================================================================

`timescale 1ns / 1ps

module uart_rx #(
    parameter CLK_FREQ  = 25200000,
    parameter BAUD_RATE = 115200
)(
    input  wire       clk,
    input  wire       rst_n,
    input  wire       rx_pin,
    output reg  [7:0] rx_data,
    output reg        rx_valid
);

    localparam CLKS_PER_BIT = CLK_FREQ / BAUD_RATE;
    localparam CNT_WIDTH    = $clog2(CLKS_PER_BIT);

    // Double-flop synchronizer for asynchronous input
    reg [2:0] rx_sync;
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            rx_sync <= 3'b111;
        end else begin
            rx_sync <= {rx_sync[1:0], rx_pin};
        end
    end

    wire rx_in = rx_sync[2];

    localparam STATE_IDLE  = 2'b00;
    localparam STATE_START = 2'b01;
    localparam STATE_DATA  = 2'b10;
    localparam STATE_STOP  = 2'b11;

    reg [1:0]           state;
    reg [CNT_WIDTH-1:0] clk_cnt;
    reg [2:0]           bit_idx;
    reg [7:0]           shift_reg;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state     <= STATE_IDLE;
            clk_cnt   <= {CNT_WIDTH{1'b0}};
            bit_idx   <= 3'd0;
            shift_reg <= 8'd0;
            rx_data   <= 8'd0;
            rx_valid  <= 1'b0;
        end else begin
            rx_valid <= 1'b0; // default 1-cycle strobe

            case (state)
                STATE_IDLE: begin
                    clk_cnt <= {CNT_WIDTH{1'b0}};
                    bit_idx <= 3'd0;
                    if (rx_in == 1'b0) begin
                        // Falling edge detected -> start bit
                        state <= STATE_START;
                    end
                end

                STATE_START: begin
                    // Sample in the middle of start bit
                    if (clk_cnt == (CLKS_PER_BIT / 2)) begin
                        if (rx_in == 1'b0) begin
                            clk_cnt <= {CNT_WIDTH{1'b0}};
                            state   <= STATE_DATA;
                        end else begin
                            // False alarm / glitch
                            state <= STATE_IDLE;
                        end
                    end else begin
                        clk_cnt <= clk_cnt + 1'b1;
                    end
                end

                STATE_DATA: begin
                    if (clk_cnt == (CLKS_PER_BIT - 1)) begin
                        clk_cnt <= {CNT_WIDTH{1'b0}};
                        shift_reg[bit_idx] <= rx_in;
                        if (bit_idx == 3'd7) begin
                            bit_idx <= 3'd0;
                            state   <= STATE_STOP;
                        end else begin
                            bit_idx <= bit_idx + 1'b1;
                        end
                    end else begin
                        clk_cnt <= clk_cnt + 1'b1;
                    end
                end

                STATE_STOP: begin
                    // Wait until middle of stop bit
                    if (clk_cnt == (CLKS_PER_BIT - 1)) begin
                        clk_cnt  <= {CNT_WIDTH{1'b0}};
                        state    <= STATE_IDLE;
                        if (rx_in == 1'b1) begin
                            rx_data  <= shift_reg;
                            rx_valid <= 1'b1;
                        end
                    end else begin
                        clk_cnt <= clk_cnt + 1'b1;
                    end
                end

                default: state <= STATE_IDLE;
            endcase
        end
    end

endmodule

