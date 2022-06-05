#ifndef PROGRAM_OPTIONS_HPP
#define PROGRAM_OPTIONS_HPP
#include <string>
#include <exception>

class OptionsError : public std::invalid_argument {
public:
	OptionsError(std::string const& e) : std::invalid_argument(e) {}
};

// Struct for parsing and storing all options from the commandline.
struct Options {
	std::string gui_address,
	            gui_port,
				      player_name,
				      server_address,
				      server_port;
	uint16_t port;
	Options(int, char*[]);
};

#endif // PROGRAM_OPTIONS_HPP