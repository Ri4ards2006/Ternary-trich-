// =============================================================================
// Balanced Ternary Arithmetic & Logic Unit (ALU)
//
// 2B1T Encoding:
//   2'b00 : 0  (Zero / U)
//   2'b01 : +1 (Positive / P)
//   2'b10 : -1 (Negative / N)
//   2'b11 : Reserved / Treated as 0
// =============================================================================

`timescale 1ns / 1ps

module ternary_alu (
    input  wire [1:0] a,          // Operand A
    input  wire [1:0] b,          // Operand B
    input  wire [1:0] c_in,       // Carry In
    input  wire [2:0] op,         // Operation Select
    output reg  [1:0] result,     // Result
    output reg  [1:0] c_out       // Carry Out (for addition)
);

    // Opcodes
    localparam OP_ADD = 3'd0;     // Full Adder (A + B + Cin)
    localparam OP_MIN = 3'd1;     // Ternary AND / Minimum
    localparam OP_MAX = 3'd2;     // Ternary OR / Maximum
    localparam OP_INV = 3'd3;     // Inversion / Negation (-A)
    localparam OP_XOR = 3'd4;     // Symmetric Difference
    localparam OP_CYC = 3'd5;     // Successor / Cycle
    localparam OP_MUL = 3'd6;     // Ternary Multiplication

    // Internal signed values
    wire signed [2:0] s_a = (a == 2'b01) ? 3'sd1 : ((a == 2'b10) ? -3'sd1 : 3'sd0);
    wire signed [2:0] s_b = (b == 2'b01) ? 3'sd1 : ((b == 2'b10) ? -3'sd1 : 3'sd0);
    wire signed [2:0] s_c = (c_in == 2'b01) ? 3'sd1 : ((c_in == 2'b10) ? -3'sd1 : 3'sd0);

    wire signed [3:0] total_sum = s_a + s_b + s_c;

    // Full adder result
    reg [1:0] add_sum;
    reg [1:0] add_carry;
    always @(*) begin
        case (total_sum)
            4'sd3:  begin add_sum = 2'b00; add_carry = 2'b01; end //  3 = +1*3 + 0
            4'sd2:  begin add_sum = 2'b10; add_carry = 2'b01; end //  2 = +1*3 - 1
            4'sd1:  begin add_sum = 2'b01; add_carry = 2'b00; end //  1 =  0*3 + 1
            4'sd0:  begin add_sum = 2'b00; add_carry = 2'b00; end //  0 =  0*3 + 0
            -4'sd1: begin add_sum = 2'b10; add_carry = 2'b00; end // -1 =  0*3 - 1
            -4'sd2: begin add_sum = 2'b01; add_carry = 2'b10; end // -2 = -1*3 + 1
            -4'sd3: begin add_sum = 2'b00; add_carry = 2'b10; end // -3 = -1*3 + 0
            default: begin add_sum = 2'b00; add_carry = 2'b00; end
        endcase
    end

    // MIN (Ternary AND)
    function [1:0] fn_min(input [1:0] op1, input [1:0] op2);
        if (op1 == 2'b10 || op2 == 2'b10)
            fn_min = 2'b10; // -1
        else if (op1 == 2'b00 || op2 == 2'b00)
            fn_min = 2'b00; // 0
        else
            fn_min = 2'b01; // +1
    endfunction

    // MAX (Ternary OR)
    function [1:0] fn_max(input [1:0] op1, input [1:0] op2);
        if (op1 == 2'b01 || op2 == 2'b01)
            fn_max = 2'b01; // +1
        else if (op1 == 2'b00 || op2 == 2'b00)
            fn_max = 2'b00; // 0
        else
            fn_max = 2'b10; // -1
    endfunction

    // Invert (-op)
    function [1:0] fn_inv(input [1:0] op1);
        if (op1 == 2'b01)
            fn_inv = 2'b10; // +1 -> -1
        else if (op1 == 2'b10)
            fn_inv = 2'b01; // -1 -> +1
        else
            fn_inv = 2'b00; // 0 -> 0
    endfunction

    // XOR (Symmetric Difference)
    function [1:0] fn_xor(input [1:0] op1, input [1:0] op2);
        if (op1 == op2)
            fn_xor = 2'b00; // identical -> 0
        else if ((op1 == 2'b01 && op2 == 2'b10) || (op1 == 2'b10 && op2 == 2'b01))
            fn_xor = 2'b10; // diff is 2 -> -1
        else
            fn_xor = 2'b01; // diff is 1 -> +1
    endfunction

    // CYCLE (Successor: -1 -> 0 -> +1 -> -1)
    function [1:0] fn_cycle(input [1:0] op1);
        case (op1)
            2'b10: fn_cycle = 2'b00; // -1 -> 0
            2'b00: fn_cycle = 2'b01; //  0 -> +1
            2'b01: fn_cycle = 2'b10; // +1 -> -1
            default: fn_cycle = 2'b00;
        endcase
    endfunction

    // MUL
    function [1:0] fn_mul(input [1:0] op1, input [1:0] op2);
        if (op1 == 2'b00 || op2 == 2'b00)
            fn_mul = 2'b00;
        else if (op1 == op2)
            fn_mul = 2'b01; // (+1)*(+1) = +1 or (-1)*(-1) = +1
        else
            fn_mul = 2'b10; // (+1)*(-1) = -1
    endfunction

    // ALU Multiplexer
    always @(*) begin
        c_out = 2'b00;
        case (op)
            OP_ADD: begin
                result = add_sum;
                c_out  = add_carry;
            end
            OP_MIN: begin
                result = fn_min(a, b);
            end
            OP_MAX: begin
                result = fn_max(a, b);
            end
            OP_INV: begin
                result = fn_inv(a);
            end
            OP_XOR: begin
                result = fn_xor(a, b);
            end
            OP_CYC: begin
                result = fn_cycle(a);
            end
            OP_MUL: begin
                result = fn_mul(a, b);
            end
            default: begin
                result = 2'b00;
            end
        endcase
    end

endmodule
