all:
	g++ -o discotool discotool.cpp -ldevice -lbe -lboost_program_options

recompile:
	rm discotool
	make
