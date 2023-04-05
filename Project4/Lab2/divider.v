//Verilog HDL for "PLL", "divider_d" "functional"

module divider_d(out,in,rst);
input in,rst;
output reg out ;
reg [15:0] cnt;
always@(posedge in or negedge rst) 
begin
  if (!rst)
    cnt<= 0;
  else if (cnt == 15)
    cnt <= 0;
  else
    cnt <= cnt+1;
end
always@(posedge in or negedge rst)
begin 
  if (!rst)
    out<= 0 ;
  else if (cnt<8)  
    out<= 0;
  else 
    out <= 1;
end
endmodule
