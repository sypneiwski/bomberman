#include <iostream>
#include <string>
#include <exception>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "program_options.hpp"
#include "messages.hpp"
#include "buffer_wrapper.hpp"
#include "connections.hpp"


void worker(ServerConnection *conn) {
	for (;;) {
		messages::ServerToClient m(conn);
	}
}

int main(int argc, char *argv[]) {

	options::Options options = options::Options(argc, argv);
	std::cout << "options:\n"
	<< "gui " << options.gui_address << ":"  << options.gui_port << "\n"
	<< "player name " << options.player_name << "\n"
	<< "port " << options.port << "\n"
	<< "server " << options.server_address << ":" << options.server_port << "\n";

	/*
	try {
		const char buf[] = {3, 0, 44, 0, 0, 0, 3, 2, 3, 0, 2, 0, 4, 2, 4, 0, 3,
							0, 5, 0, 0, 0, 0, 5, 0, 5, 0, 7};
		size_t size = 28;
		buffers::Buffer buffer(buf, size);
		messages::ServerToClient x(buffer);
	}
	catch (std::exception &e) {
		std::cerr << "ERROR: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	*/

	buffers::Buffer buffer();

	try {
		boost::asio::io_context io_context;
		ServerConnection conn(io_context, options);
		boost::thread t(boost::bind(&worker, &conn));
		t.join();
	}
	catch (std::exception &e) {
		std::cerr << "ERROR: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return 0;
}