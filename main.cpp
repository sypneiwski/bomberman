#include <iostream>
#include <string>
#include <exception>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include "program_options.hpp"
#include "messages.hpp"

using boost::asio::ip::tcp;

class ServerConnection {
public:
	ServerConnection(boost::asio::io_context &io_context, options::Options &options)
			: io_context(io_context), socket(io_context) {
		tcp::resolver resolver(io_context);
		tcp::resolver::query query(options.server_address, options.server_port);
		resolver.async_resolve(query, boost::bind(
				&ServerConnection::resolve_handler,
				this,
				boost::asio::placeholders::error
		));
	}

	~ServerConnection() {
		std::cout << "destroyed ServerConnection\n";
	}

private:
	void resolve_handler(const boost::system::error_code& ec) {
		if (!ec) {
			/*
			boost::asio::async_connect(socket, iterator, boost::bind(
					&ServerConnection::connect_handler,
					shared_from_this(),
					boost::asio::placeholders::error
					));
					*/
			std::cout << "ok!\n";
		}
		else {
			//throw std::invalid_argument(ec.message());
			std::cout << ec.message() << "\n";
		}
	}

	void connect_handler(const boost::system::error_code& ec) {
		if (!ec) {
			std::cout << "Connected!\n" << "socket local: "
					  << socket.local_endpoint()
					  << "\nsocket remote: " << socket.remote_endpoint()
					  << "\n";
		}
		else {
			throw std::invalid_argument("Could not connect to server");
		}
	}

	boost::asio::io_context& io_context;
	tcp::socket socket;
};

int main(int argc, char *argv[]) {
	options::Options options = options::Options(argc, argv);
	std::cout << "options:\n"
	<< "gui " << options.gui_address << ":"  << options.gui_port << "\n"
	<< "player name " << options.player_name << "\n"
	<< "port " << options.port << "\n"
	<< "server " << options.server_address << ":" << options.server_port << "\n";

	/*
	const uint8_t buffer[] = {3, 0, 44, 0, 0, 0, 3, 2, 3, 0, 2, 0, 4, 2, 4, 0, 3, 0, 5, 0, 0, 0, 0, 5, 0, 5, 0, 7};
	size_t size = sizeof(buffer) / sizeof(char);
	std::cout << "size: " << size << "\n";
	messages::ServerMessage x = messages::ServerMessage((const char*) &buffer, size);
	*/

	try {
		boost::asio::io_context io_context;
		ServerConnection conn(io_context, options);
		io_context.run();
	}
	catch (std::exception& e) {
		std::cerr << "ERROR: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return 0;
}