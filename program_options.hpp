#ifndef PROGRAM_OPTIONS_HPP
#define PROGRAM_OPTIONS_HPP
#include <string>
#include <exception>

namespace options {
	class OptionsError : public std::invalid_argument {
	public:
		OptionsError(std::string const& e) : std::invalid_argument(e) {}
	};

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