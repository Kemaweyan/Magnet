#include <iostream>
#include "magnet.h"
#include <cstdlib>
#include <getopt.h>

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

void handleError(int error)
{
	switch (error) {
		case TE_FILE_NOT_FOUND:
			std::cerr << "ERROR: File not found!" << std::endl;
			break;
		case TE_READING_FILE:
			std::cerr << "ERROR: Cannot read the file!" << std::endl;
			break;
		case TE_PARSING_INT:
			std::cerr << "ERROR: Cannot parse integer value!" << std::endl;
			break;
		case TE_PARSING_STRING:
			std::cerr << "ERROR: Cannot parse string value!" << std::endl;
			break;
		case TE_INVALID_FILE_STRUCTURE:
			std::cerr << "ERROR: Invalid file structure!" << std::endl;
			break;
		case TE_CANNOT_FIND_CLOSE_TAG:
			std::cerr << "ERROR: Cannot find close tag!" << std::endl;
			break;
		case TE_UNEXPECTED_END_OF_FILE:
			std::cerr << "ERROR: Unexpected end of file!" << std::endl;
			break;
		case TE_UNABLE_DETERMINE_STRING_LENGTH:
			std::cerr << "ERROR: Unable determine string length!" << std::endl;
			break;
		default:
			break;
	}
}

void printHelp()
{
	std::cout << "Usage: magnet [OPTION]... [FILE]..\n";
	std::cout << "Create magnet links from the FILEs (the current directory by default).\n\n";

	std::cout << "Mandatory arguments to long options are mandatory for short options too.\n";
	std::cout << "  -a, --add-announces\t\tadd announces to the magnet links from the FILEs\n";
	std::cout << "  -i, --info\t\t\tshow information of the FILEs\n";
	std::cout << "  -e, --exec=PROGRAM\t\trun PROGRAM with magnet links\n";
	std::cout << "  -h, --help\t\t\tdisplay this help end exit\n\n" << std::endl;

	exit(0);
}

void printInfo(TorrentFile *torrent)
{
	std::string info;
	torrent->getStructure(info);

	handleError(torrent->errorCode());

	std::cout << "\n\nStructure of file " << torrent->getFileName();
	std::cout << "\n================================================================================\n";
	std::cout << info << std::endl;
}

void startDownload(TorrentFile *torrent, std::string& execApp, bool keepAnnounces = false)
{
	int error = 0;
	std::string magnet;
	if (error = torrent->createMagnet(magnet, keepAnnounces)) {
		handleError(error);
		return;
	}

	if (execApp.length()) {
#ifdef WIN32
		STARTUPINFO si= { sizeof(si) };
		PROCESS_INFORMATION pi;
		execApp += " ";
		execApp += magnet;
		CreateProcessA(NULL, execApp, NULL, NULL, false, 0, NULL, NULL, &si, &pi);
#else
		pid_t pid = fork();

		if (!pid)
			execlp(execApp.c_str(), execApp.c_str(), magnet.c_str(), NULL);
#endif
	} else
		std::cout << magnet << std::endl;
}

int main(int argc, char *argv[])
{
	int c;
	struct option long_options[] = {
		{"add-announces", no_argument, 0, 'a'},
		{"info", no_argument, 0, 'i'},
		{"help", no_argument, 0, 'h'},
		{"exec", required_argument, 0, 'e'},
		{0, 0, 0, 0}
	};

	int option_index = 0;

	bool info_flag = false;
	bool keep_announces_flag = false;
	std::string exec_app;
	std::vector <int> errors;

	for (;;) {
		c = getopt_long(argc, argv, "aihe:", long_options, &option_index);

     		if (c == -1)
			break;

		switch (c) {
			case 'i':
				info_flag = true;
				break;
			case 'a':
				keep_announces_flag = true;
				break;
			case 'h':
				printHelp();
				break;
			case 'e':
				exec_app = optarg;
				break;
		}
	}

	if (optind >= argc)
		printHelp();

	while (optind < argc) {
		TorrentFile *torrent = new TorrentFile(argv[optind++]);

		errors.push_back(torrent->errorCode());

		if (errors.back()) {
			delete torrent;
			continue;
		}

		if (info_flag)
			printInfo(torrent);
		else
			startDownload(torrent, exec_app, keep_announces_flag);

		delete torrent;
	}

	for (int i = 0; i < errors.size(); ++i)
		handleError(errors[i]);

	return 0;
}
