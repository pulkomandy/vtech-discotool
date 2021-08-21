/*
 * Disco-Cart C++ tool
 * Copyright (C) 2021 pulkomandy <pulkomandy@kitt>
 *
 * Distributed under terms of the MIT license.
 *
 * I did not want to port node/js to Haiku just for this, so I made my own
 * thing instead...
 */

#include <File.h>
#include <SerialPort.h>

#include <string.h>
#include <stdio.h>

#include <boost/program_options.hpp>

#include <iostream>

int main(int argc, char* argv[])
{
	// TODO parse command line:
	// - Dump or flash
	// - Also implement R, W and erase
	
	std::string portName;
	std::string bank;
	std::string outFile;
	std::string mode;
	int length;

	boost::program_options::options_description options("Disco-cart dumper");
	options.add_options()
		("help", "show available options")
		("port", boost::program_options::value<std::string>(&portName)->default_value("usb0"),
			"select serial port")
		("bank", boost::program_options::value<std::string>(&bank)->default_value("LOW"),
			"select bank (HIGH or LOW)")
		("out", boost::program_options::value<std::string>(&outFile)->default_value("dump.bin"),
			"File to write when dumping")
		("mode", boost::program_options::value<std::string>(&mode),
			"One of sddump, dump, or write")
		("length", boost::program_options::value<int>(&length)->default_value(0x100000),
			"Number of bytes to read")
	;

	boost::program_options::variables_map parsedOptions;
	boost::program_options::store(boost::program_options::parse_command_line(argc, argv, options), parsedOptions);
	boost::program_options::notify(parsedOptions);

	if (parsedOptions.count("help") || !parsedOptions.count("mode")) {
		std::cout << options << "\n";
		exit(1);
	}

	std::string portPath = "/dev/ports/" + portName;
	printf("Using port %s\n", portPath.c_str());
	BSerialPort port;
	status_t r = port.Open(portPath.c_str());
	if (r < 0) {
		printf("Failed to open serial port: %s\n", strerror(r));
		exit(-1);
	}

	bank = "B" + bank + "%";
	port.Write(bank.c_str(), bank.size());

	if (mode == "sddump") {
		std::stringstream command;
		command << "D" << outFile << ":" << std::hex << length << "%";
		port.Write(command.str().c_str(), command.str().size());

		char* done;
		do {
			char buffer[257] = {0};
			int len = port.Read(buffer, 256);

			// We get either an error message ("Failed to preallocate!") or a
			// success one ("Done!"). Hopefully nothing else has a '!' in it?
			char* done = strchr(buffer, '!');
			printf(buffer);
		} while (done == NULL);
	} else if (mode == "dump") {
		int index = 0;
		BFile file(outFile.c_str(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		while (index < length) {
			std::stringstream command;
			command << "R" << std::hex << index << "%";
			port.Write(command.str().c_str(), command.str().size());

			char buffer[257] = {0};
			int len = port.Read(buffer, 256);
			uint16_t parsed = strtol(buffer, NULL, 16);
			file.Write(&parsed, 2);

			index++;
			if ((index & 0xFFF) == 0) {
				printf(".");
				fflush(stdout);
			}
		}
		puts("");
		printf("Read %d words.\n", index);
	} else if (mode == "write") {
		BFile file(outFile.c_str(), B_READ_ONLY);
		port.Write("ERS%", 4);
		char buffer[257] = {0};
		int len = port.Read(buffer, 256);

		uint16_t value;
		int index = 0;
		while (file.Read(&value, 2) == 2) {
			char buffer[12];
			sprintf(buffer, "W%02x:%02x%", index, value);

			port.Write(buffer, strlen(buffer));
			char buffer[257] = {0};
			int len = port.Read(buffer, 256);

			index++;
			if ((index & 0xFFF) == 0) {
				printf(".");
				fflush(stdout);
			}
		}
		puts("");
		printf("Wrote %d words.\n", index);
	} else {
		printf("Unknown operation mode\n");
	}

	port.Close();
}
