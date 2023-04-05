module test;

reg[7:0] in;
reg clk,reset;
wire[7:0] out;

initial begin
	clk=0;
	forever
    #5 clk = !clk;
end

initial begin
    $dumpfile("cpu.vcd");
    $dumpvars;	
end

cpu cc(.in(in), .clk(clk), .reset(reset), .out(out));


integer seed = 123;
initial begin
	in    =8'b00000000;
	reset =1'b1;

#12 in    =8'b01101111;
	reset =1'b0;
     
//write your test pattern----------------

#12 in = $random(seed)%256; // 8'b110_1111;
#12 in = $random(seed)%256; // 8'b1;
#12 in = $random(seed)%256; // 8'b11_1101;
#12 in = $random(seed)%256; // 8'b1011_0010;
#12 in = $random(seed)%256; // 8'b1_0101;

#12 in = $random(seed)%256; // 8'b11_0001;
#12 in = $random(seed)%256; // 8'b1001_1001;
#12 in = $random(seed)%256; // 8'b1_0101;
#12 in = $random(seed)%256; // 8'b1010_0110;
#12 in = $random(seed)%256; // 8'b1110_0011;

#12 in = $random(seed)%256; // 8'b111_0010;
#12 in = $random(seed)%256; // 8'b1101_0000;
#12 in = $random(seed)%256; // 8'b101_1011;
#12 in = $random(seed)%256; // 8'b1111_1111;
#12 in = $random(seed)%256; // 8'b100_1110;

#12 in = $random(seed)%256; // 8'b1100_1011;
#12 in = $random(seed)%256; // 8'b101_0011;
#12 in = $random(seed)%256; // 8'b1101_1101;
#12 in = $random(seed)%256; // 8'b1110_1011;
#12 in = $random(seed)%256; // 8'b1_1001;


//----------------------------------------
#10  $finish;

end
endmodule