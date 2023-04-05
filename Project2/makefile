C = gcc
G = g++

exeName = B101016.o
Head_and_Cpp = Lab2.cpp declare.h load_cell_lib.h load_verilog.h output_load.h output_delay_and_path.h
Merge_All = B101016.cpp

$(exeName): $(Head_and_Cpp)
	$(G) -std=c++11 $(Merge_All) -o $(exeName)

clean:
	rm *B101016*.txt *.o *.gch $(exeName)