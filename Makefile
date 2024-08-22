all: discotool

discotool:
	g++ -o discotool discotool.cpp -ldevice -lbe -lboost_program_options

clean:
	rm -f discotool *.o

.PHONY: clean all
