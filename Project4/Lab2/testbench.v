//Verilog HDL for "PLL", "testbench" "functional"

module testbench (rst);

output rst;
reg rst;

initial begin
	rst=1'b0;
	#500 rst = 1'b1;
end

endmodule
