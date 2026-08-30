// =============================================================================
// Standard DVI 1.0 TMDS 8b/10b Encoder
// DC-balanced encoding with control token generation
// =============================================================================

`timescale 1ns / 1ps

module tmds_encoder (
    input  wire       clk,
    input  wire       rst_n,
    input  wire [7:0] data_in,
    input  wire       c0,
    input  wire       c1,
    input  wire       de,
    output reg  [9:0] tmds_out
);

    // Step 1: Count ones in data_in
    wire [3:0] n1d = data_in[0] + data_in[1] + data_in[2] + data_in[3] +
                     data_in[4] + data_in[5] + data_in[6] + data_in[7];

    // Decide between XOR and XNOR minimization
    wire use_xnor = (n1d > 4'd4) || ((n1d == 4'd4) && (data_in[0] == 1'b0));

    // Step 2: Generate 9-bit intermediate word q_m
    wire [8:0] q_m;
    assign q_m[0] = data_in[0];
    assign q_m[1] = use_xnor ? ~(q_m[0] ^ data_in[1]) : (q_m[0] ^ data_in[1]);
    assign q_m[2] = use_xnor ? ~(q_m[1] ^ data_in[2]) : (q_m[1] ^ data_in[2]);
    assign q_m[3] = use_xnor ? ~(q_m[2] ^ data_in[3]) : (q_m[2] ^ data_in[3]);
    assign q_m[4] = use_xnor ? ~(q_m[3] ^ data_in[4]) : (q_m[3] ^ data_in[4]);
    assign q_m[5] = use_xnor ? ~(q_m[4] ^ data_in[5]) : (q_m[4] ^ data_in[5]);
    assign q_m[6] = use_xnor ? ~(q_m[5] ^ data_in[6]) : (q_m[5] ^ data_in[6]);
    assign q_m[7] = use_xnor ? ~(q_m[6] ^ data_in[7]) : (q_m[6] ^ data_in[7]);
    assign q_m[8] = ~use_xnor;

    // Step 3: Count ones and zeros in q_m[7:0]
    wire [3:0] n1q = q_m[0] + q_m[1] + q_m[2] + q_m[3] +
                     q_m[4] + q_m[5] + q_m[6] + q_m[7];
    wire [3:0] n0q = 4'd8 - n1q;

    // Signed disparity bias counter
    reg signed [4:0] dc_bias;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            tmds_out <= 10'd0;
            dc_bias  <= 5'sd0;
        end else if (!de) begin
            dc_bias <= 5'sd0;
            case ({c1, c0})
                2'b00:   tmds_out <= 10'b1101010100;
                2'b01:   tmds_out <= 10'b0010101011;
                2'b10:   tmds_out <= 10'b0101010100;
                default: tmds_out <= 10'b1010101011;
            endcase
        end else begin
            if (dc_bias == 5'sd0 || n1q == n0q) begin
                tmds_out[9]   <= ~q_m[8];
                tmds_out[8]   <= q_m[8];
                tmds_out[7:0] <= q_m[8] ? q_m[7:0] : ~q_m[7:0];

                if (q_m[8] == 1'b0)
                    dc_bias <= dc_bias + (n0q - n1q);
                else
                    dc_bias <= dc_bias + (n1q - n0q);
            end else begin
                if ((dc_bias > 5'sd0 && n1q > n0q) || (dc_bias < 5'sd0 && n0q > n1q)) begin
                    tmds_out[9]   <= 1'b1;
                    tmds_out[8]   <= q_m[8];
                    tmds_out[7:0] <= ~q_m[7:0];
                    dc_bias       <= dc_bias + {q_m[8], 1'b0} + (n0q - n1q);
                end else begin
                    tmds_out[9]   <= 1'b0;
                    tmds_out[8]   <= q_m[8];
                    tmds_out[7:0] <= q_m[7:0];
                    dc_bias       <= dc_bias - {~q_m[8], 1'b0} + (n1q - n0q);
                end
            end
        end
    end

endmodule
