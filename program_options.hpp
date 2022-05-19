#ifndef PROGRAM_OPTIONS_H
#define PROGRAM_OPTIONS_H
#include <string>

namespace options {
	struct Options {
		std::string gui_address;
		std::string player_name;
		uint16_t port;
		std::string server_address;
	};

	Options get_client_options(int, char*[]);
}

#endif // PROGRAM_OPTIONS_H