all:
	g++ -o discotool main.cpp -ldevice -lbe -lboost_program_options

recompile:
	rm discotool
	make
