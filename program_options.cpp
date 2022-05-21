#include <boost/program_options.hpp>
#include <boost/optional.hpp>
#include <string>
#include <iostream>
#include <exception>
#include "program_options.hpp"

namespace options {
	bool resolve_address(boost::optional<std::string> &full_address,
							 	std::string *address, std::string *port) {
		if (!full_address)
			return false;
		std::string::size_type last_colon;
		last_colon = full_address->rfind(":");
		if (last_colon == std::string::npos)
			return false;
		*address = full_address->substr(0, last_colon);
		*port = full_address->substr(last_colon + 1);
		return true;
	}

	Options::Options(int argc, char* argv[]) {
		namespace po = boost::program_options;
		try {
			po::options_description desc("Allowed options");
			boost::optional<std::string> gui_address_, player_name_, server_address_;
			boost::optional<uint16_t> port_;
			desc.add_options()
					("gui-address,d", po::value(&gui_address_), "gui address")
					("help,h", "produce help message")
					("player-name,n", po::value(&player_name_), "player name")
					("port,p", po::value(&port_), "port")
					("server-address,s", po::value(&server_address_), "server address")
					;
			po::variables_map vm;
			po::store(po::parse_command_line(argc, argv, desc), vm);
			po::notify(vm);

			if (vm.count("help")) {
				std::cout << desc << "\n";
				exit(EXIT_SUCCESS);
			}

			if (!resolve_address(gui_address_, &gui_address, &gui_port))
				throw std::invalid_argument("Please provide a valid gui address");
			if (!resolve_address(server_address_, &server_address, &server_port))
				throw std::invalid_argument("Please provide a valid server address");
			if (!player_name_)
				throw std::invalid_argument("Please provide a player name");
			player_name = *player_name_;
			if (!port)
				throw std::invalid_argument("Please provide a valid port number");
			port = *port_;
		}
		catch(std::exception& e) {
			std::cerr << "ERROR: " << e.what() << std::endl;
			exit(EXIT_FAILURE);
		}
	}
}