#ifndef CONNECTIONS_HPP
#define CONNECTIONS_HPP
#include <iostream>
#include <string>
#include <exception>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include <endian.h>
#include "program_options.hpp"

//todo namespace

class GUIConnection {
public:
	GUIConnection(boost::asio::io_context &io_context, options::Options &options) :
			socket(io_context, udp::endpoint(udp::v4(), options.port)) {
	}

private:
	using udp = boost::asio::ip::udp;
	udp::socket socket;
};

class ServerConnection {
private:
	using tcp = boost::asio::ip::tcp;
	tcp::socket socket;

	template<typename T>
	T read();

public:
	ServerConnection(boost::asio::io_context&, options::Options&);

	uint8_t read8();

	uint16_t read16();

	uint32_t read32();

	void read_string(std::string*, size_t);
};

#endif // CONNECTIONS_HPP
