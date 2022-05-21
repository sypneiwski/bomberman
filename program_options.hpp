#ifndef PROGRAM_OPTIONS_HPP
#define PROGRAM_OPTIONS_HPP
#include <string>

namespace options {
	struct Options {
		std::string gui_address,
					gui_port,
					player_name,
					server_address,
					server_port;
		uint16_t port;
		Options(int, char*[]);
	};
}

#endif // PROGRAM_OPTIONS_HPP