
module ram16x8 (data_out, clk, w_mem, address, data_in);
   
input[3:0] address;
input[7:0] data_in;
input w_mem, clk;
output[7:0] data_out;

reg[7:0] ram_data[16:0];
reg[7:0] data_out;

always@(posedge clk)begin
	if (w_mem == 1'b0)
		data_out = ram_data[address];
	else if(w_mem == 1'b1)
		ram_data[address] = data_in;
	else
		data_out = 8'hz;
end
endmodule

module cpu(out, in, clk, reset);

parameter DATA_IN=0, REG_C_O=1, ALU_OUT=2, MEM_OUT=3;
   
input[7:0] in;
input clk, reset;
output[7:0] out;

reg[7:0] reg_a, reg_b, reg_c, instr, write_data, out;
wire[7:0] in, mem_out, alu_out;
wire clk, reset;
wire [3:0] addr;
wire [1:0] sel;
wire ready, w_out, w_reg_a, w_reg_b, w_reg_c, w_mem;

always@(alu_out or in or mem_out or reg_c or sel)begin
	if (sel == DATA_IN)
		write_data = in;
	else if (sel == ALU_OUT)
		write_data = alu_out;
	else if (sel==MEM_OUT)
		write_data=mem_out; 
end   
   
always@(posedge clk)begin
	if(reset)begin
		instr = 0;
		reg_a = 0;
		reg_b = 0;
		reg_c = 0;
		out = 0;
	end
	else begin
	    instr = in;
		if(ready)begin
			if (w_reg_c)
				reg_c = write_data;
			else if (w_reg_a)
				reg_a = write_data;
			else if (w_reg_b)
				reg_b = write_data;
			else
				out = 0;
		end
	end
end

ram16x8 r1(
	//output
	.data_out	(mem_out[7:0]),
	//input
	.address	(addr[3:0]),
	.data_in	(write_data[7:0]),
	.w_mem		(w_mem),
	.clk		(clk));
   
ALU a1(
	//output
	.alu_out	(alu_out[7:0]),
	//input
	.reg_a		(reg_a[7:0]),
	.reg_b		(reg_b[7:0]),
	.sel_fun	(instr[6:4]));

Control c1(
	//output
	.addr		(addr[3:0]),
	.sel		(sel[1:0]),
	.ready		(ready),
	.w_out		(w_out),
	.w_reg_a	(w_reg_a),
	.w_reg_b	(w_reg_b),
	.w_reg_c	(w_reg_c),
	.w_mem		(w_mem),
	//input
	.opcode		(instr[7:0]),
	.clk		(clk),
	.reset		(reset));
   
endmodule

module Control(opcode, clk, reset, addr, sel, ready,
		w_out, w_reg_a, w_reg_b, w_reg_c, w_mem) ;

parameter DATA_IN=0, REG_C_O=1, ALU_OUT=2, MEM_OUT=3;
parameter ST_0=0, ST_1=1, ST_2=2, ST_3=3;
   
input[7:0] opcode;
input clk, reset;
output[3:0] addr;
output[1:0] sel;
output ready, w_out, w_reg_a, w_reg_b, w_reg_c, w_mem;

reg[3:0] addr;
reg[1:0] sel;
reg ready, w_mem, w_out, w_reg_a, w_reg_b, w_reg_c;
reg[1:0] current_state, next_state;
   
always@(posedge clk)begin
	if (!reset)
		current_state = next_state;
	else
		current_state = ST_1;
	end

always@(current_state or opcode)begin
	case(current_state)
	ST_0:begin
		ready = 1;
		addr = opcode[3:0];
		case (opcode[7:4])
		4'b0000:begin
				w_out=0; w_reg_a=0; w_reg_b=0; w_reg_c=1; w_mem=0;
				sel=ALU_OUT; next_state=ST_0; 
				end
		4'b0001:begin
				w_out=0; w_reg_a=0; w_reg_b=0; w_reg_c=1; w_mem=0; 
				sel=ALU_OUT; next_state=ST_0;
				end
		4'b0010:begin
				w_out=0; w_reg_a=0; w_reg_b=0; w_reg_c=1; w_mem=0; 
				sel=ALU_OUT; next_state=ST_0;
				end
		4'b0011:begin
				w_out=0; w_reg_a=0; w_reg_b=0; w_reg_c=1; w_mem=0; 
				sel=ALU_OUT; next_state=ST_0;
				end
		4'b0100:begin
				w_out=0; w_reg_a=0; w_reg_b=0; w_reg_c=0; w_mem=0;
				sel=ALU_OUT; next_state=ST_0;
				end
		4'b0101:begin
				w_out=0; w_reg_a=0; w_reg_b=0; w_reg_c=0; w_mem=0;
				sel=ALU_OUT; next_state=ST_0;
				end
		4'b0110:begin
				w_out=0; w_reg_a=1; w_reg_b=0; w_reg_c=0; w_mem=0;
				sel=DATA_IN; next_state=ST_1;
				end
		4'b0111:begin
				w_out=0; w_reg_a=0; w_reg_b=1; w_reg_c=0; w_mem=0; 
				sel=DATA_IN; next_state=ST_0;
				end
		4'b1000:begin
				w_out=0; w_reg_a=0; w_reg_b=0; w_reg_c=0; w_mem=1; 
				sel=DATA_IN; next_state=ST_1;
				end
		4'b1001:begin
				w_out=0; w_reg_a=0; w_reg_b=0; w_reg_c=0; w_mem=1; 
				sel=REG_C_O; next_state=ST_0;
				end
		4'b1010:begin
				w_out=0; w_reg_a=0; w_reg_b=0; w_reg_c=0; w_mem=0; 
				sel=REG_C_O; next_state=ST_3;  
				end
		4'b1011:begin
				w_out=1; w_reg_a=0; w_reg_b=0; w_reg_c=0; w_mem=0;
				sel=REG_C_O; next_state=ST_2;
				end
		4'b1100:begin
				w_out=0; w_reg_a=0; w_reg_b=0; w_reg_c=0; w_mem=0; 
				sel=REG_C_O; next_state=ST_3; 
				end
		4'b1101:begin
				w_out=0; w_reg_a=1; w_reg_b=0; w_reg_c=0; w_mem=0;
				sel=REG_C_O; next_state=ST_3;
				end
		4'b1110:begin
				w_out=0; w_reg_a=0; w_reg_b=1; w_reg_c=0; w_mem=0;
				sel=REG_C_O; next_state=ST_1;
				end
		default:begin
				w_out=0; w_reg_a=0; w_reg_b=0; w_reg_c=0; w_mem=0;
				sel=ALU_OUT; next_state=ST_0;
				end
		endcase
	end
	ST_1 : begin
			ready = 0; addr = 0;
			w_out = 0; w_reg_a = 0; w_reg_b = 0; w_reg_c = 0; w_mem = 0;
			sel = DATA_IN; next_state = ST_2;
			end
	ST_2 : begin
			ready = 1; addr = 0;
			w_out = 0; w_reg_a = 0; w_reg_b = 0; w_reg_c = 1; w_mem = 0;
			sel = MEM_OUT; next_state = ST_3;
			end
	ST_3 : begin
			ready = 1; addr = 0;
			w_out = 1; w_reg_a = 0; w_reg_b = 0; w_reg_c = 0; w_mem = 0;
			sel = MEM_OUT; next_state = ST_1;
			end       
	endcase 
end
endmodule

module ALU(alu_out, reg_a, reg_b, sel_fun) ;

input[7:0]  reg_a, reg_b;
input[2:0] 	sel_fun;
output[7:0] alu_out;
reg[7:0] 	alu_out;
   
always@(reg_a or reg_b or sel_fun)begin
	case (sel_fun) 
	3'b000 : alu_out = reg_a - reg_b;
	3'b001 : alu_out = reg_a + reg_b;
	3'b010 : alu_out = reg_a + 1;
	3'b011 : alu_out = reg_a - 1;
	3'b100 : alu_out = reg_a + reg_b - 1;
	3'b101 : alu_out = reg_a;
	default: alu_out = 8'bx;
	endcase
end
endmodule