#include <boost/program_options.hpp>
#include <boost/optional.hpp>
#include <string>
#include <iostream>
#include "program_options.hpp"
#include "err.hpp"

namespace options {
	Options get_client_options(int argc, char* argv[]) {
		namespace po = boost::program_options;
		Options options;
		try {
			po::options_description desc("Allowed options");
			boost::optional<std::string> gui_address, player_name, server_address;
			boost::optional<uint16_t> port;
			desc.add_options()
					("gui-address,d", po::value(&gui_address), "gui address")
					("help,h", "produce help message")
					("player-name,n", po::value(&player_name), "player name")
					("port,p", po::value(&port), "port")
					("server-address,s", po::value(&server_address), "server address")
					;
			po::variables_map vm;
			po::store(po::parse_command_line(argc, argv, desc), vm);
			po::notify(vm);

			if (vm.count("help")) {
				std::cout << desc << "\n";
				exit(0);
			}
			if (!gui_address || !player_name || !port || !server_address) {
				fatal("Not all required options set.");
			}
			options =  {*gui_address, *player_name, *port, *server_address};
		}
		catch(std::exception& e) {
			fatal(e.what());
		}
		//check options, maybe unique ptr?
		return options;
	}
}