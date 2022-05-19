#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include "program_options.hpp"
#include "err.hpp"
#include "messages.hpp"

int main(int argc, char *argv[]) {
	options::Options options = options::get_client_options(argc, argv);
	std::cout << "options : \n" << options.gui_address << "\n" << options.player_name << "\n"
			  << options.port << "\n" << options.server_address << "\n";
	std::cout << "done.\n";


	try
	{
		using boost::asio::ip::tcp;
		boost::asio::io_context io_context;

		tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), options.port));

		for (;;)
		{
			tcp::socket socket(io_context);
			acceptor.accept(socket);
			std::cout << "accepted connection\n";

			std::string message = "test\n";

			boost::system::error_code ignored_error;
			boost::asio::write(socket, boost::asio::buffer(message), ignored_error);
		}
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}


	return 0;
}