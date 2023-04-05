module test;

reg[7:0] in;
reg clk,reset;
wire[7:0] out;

initial begin
	clk=0;
	forever
    #5 clk = !clk;
end

integer seed = 123;

initial begin
    $dumpfile("cpu.vcd");
    $dumpvars;	
end

cpu cc(.in(in), .clk(clk), .reset(reset), .out(out));

initial begin
	in    =8'b00000000;
	reset =1'b1;

#12 in    =8'b01101111;
	reset =1'b0;
     
//write your test pattern----------------

#10 in = 8'h01; // 1 cycle
#10 in = 8'h12; // 1 cycle
#10 in = 8'h23; // 1 cycle
#10 in = 8'h34; // 1 cycle

#20 in = 8'h89; // 2 cycle
#10 in = 8'h9a; // 1 cycle
#20 in = 8'hab; // 2 cycle
#10 in = 8'hbc; // 1 cycle

#20 in = 8'hcd; // 2 cycle
#10 in = 8'hde; // 1 cycle
#10 in = 8'hef; // 1 cycle
#10 in = 8'hf0; // 1 cycle

#10 in = 8'h45; // 1 cycle
#10 in = 8'h56; // 1 cycle
#20 in = 8'h67; // 2 cycle
#20 in = 8'h78; // 2 cycle

//----------------------------------------
#10  $finish;

end
endmodule