#include <boost/program_options.hpp>
#include <boost/optional.hpp>
#include <string>
#include <iostream>
#include <exception>
#include <cstdint>
#include "program_options.hpp"

bool resolve_address(std::string &full_address,
							std::string *address, std::string *port) {
	std::string::size_type last_colon;
	last_colon = full_address.rfind(":");
	if (last_colon == std::string::npos)
		return false;
	*address = full_address.substr(0, last_colon);
	*port = full_address.substr(last_colon + 1);
	return true;
}

Options::Options(int argc, char* argv[]) {
	namespace po = boost::program_options;
	try {
		po::options_description desc("Allowed options");
		std::string gui_address_, server_address_;
		int64_t port_;
		desc.add_options()
				("gui-address,d", po::value<std::string>(&gui_address_)->required(), "gui address")
				("help,h", "produce help message")
				("player-name,n", po::value<std::string>(&player_name)->required(), "player name")
				("port,p", po::value<int64_t>(&port_)->required(), "port")
				("server-address,s", po::value<std::string>(&server_address_)->required(), "server address")
				;
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);

		if (vm.count("help")) {
			std::cout << desc << "\n";
			exit(EXIT_SUCCESS);
		}
		po::notify(vm);

		if (!resolve_address(gui_address_, &gui_address, &gui_port))
			throw std::invalid_argument("Please provide a valid gui address");
		if (!resolve_address(server_address_, &server_address, &server_port))
			throw std::invalid_argument("Please provide a valid server address");
		if (port_ <= 0 || port_ > std::numeric_limits<uint16_t>::max())
			throw std::invalid_argument("Please provide a valid port value");
		port = (uint16_t) port_;
	}
	catch(std::exception& e) {
		std::cerr << "ERROR: " << e.what() << std::endl;
		exit(EXIT_FAILURE);
	}
}