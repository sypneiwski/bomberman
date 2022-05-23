#include <iostream>
#include <string>
#include <exception>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include "program_options.hpp"
#include "messages.hpp"
#include "buffer_wrapper.hpp"

using boost::asio::ip::tcp;
using boost::asio::ip::udp;

char buffer[1000]; // debug

class GUIConnection {
public:
	GUIConnection(boost::asio::io_context &io_context, options::Options &options) :
		socket(io_context, udp::endpoint(udp::v4(), options.port)) {}

private:
	udp::socket socket;
};

class ServerConnection {
public:
	ServerConnection(boost::asio::io_context &io_context_, options::Options &options)
			: socket(io_context_), resolver(io_context_) {
		tcp::resolver::query query(options.server_address, options.server_port);
		resolver.async_resolve(query, boost::bind(
				&ServerConnection::resolve_handler,
				this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::iterator
		));
	}

private:
	void resolve_handler(const boost::system::error_code& ec,
						 tcp::resolver::iterator endpoints) {
		if (!ec) {
			boost::asio::async_connect(socket, endpoints, boost::bind(
					&ServerConnection::connect_handler,
					this,
					boost::asio::placeholders::error
			));
		}
		else {
			throw std::invalid_argument("Could not connect to server");
		}
	}

	void connect_handler(const boost::system::error_code& ec) {
		// check rest of endpoints?
		// https://www.boost.org/doc/libs/1_40_0/doc/html/boost_asio/example/http/client/async_client.cpp
		if (!ec) {
			std::cout << "Connected!\n" << "socket local: "
					  << socket.local_endpoint()
					  << "\nsocket remote: " << socket.remote_endpoint()
					  << "\n";
			//memset(buffer, 0, 1000);
			std::cout << "starting read\n";
			boost::asio::async_read(socket, boost::asio::buffer(buffer, 43), boost::bind(
					&ServerConnection::read_handler,
					this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred
					));

			//boost::asio::read(socket, boost::asio::buffer(buffer, 20));
			//std::cout << buffer << "\n";
		}
		else {
			throw std::invalid_argument("Could not connect to server");
		}
	}

	void read_handler(const boost::system::error_code &ec,
				   std::size_t bytes_read) {
		if (!ec) {
			std::cout << bytes_read << "\n";
			buffers::Buffer buf(buffer, bytes_read);
			messages::ServerToClient m(buf);
		}
		else {
			std::cout << "nothing came\n";
		}
	}

	tcp::socket socket;
	tcp::resolver resolver;
};

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

	try {
		boost::asio::io_context io_context;
		ServerConnection conn(io_context, options);
		io_context.run();
	}
	catch (std::exception &e) {
		std::cerr << "ERROR: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return 0;
}