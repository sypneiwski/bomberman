#include <iostream>
#include <string>
#include <exception>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include <endian.h>
#include "connections.hpp"

ServerConnection::ServerConnection(boost::asio::io_context &io_context, options::Options &options)
		: socket(io_context) {
	tcp::resolver resolver(io_context);
	boost::system::error_code ec;
	auto endpoints = resolver.resolve(options.server_address, options.server_port, ec);
	if (ec)
		throw std::invalid_argument("Could not connect to server");
	boost::asio::connect(socket, endpoints, ec);
	if (ec)
		throw std::invalid_argument("Could not connect to server");
}

template<typename T>
T ServerConnection::read() {
	T ret;
	// exception?
	boost::asio::read(socket, boost::asio::buffer(&ret, sizeof(T)));
	return ret;
}

uint8_t ServerConnection::read8() {
	return read<uint8_t>();
}

uint16_t ServerConnection::read16() {
	return be16toh(read<uint16_t>());
}

uint32_t ServerConnection::read32() {
	return be32toh(read<uint32_t>());
}

void ServerConnection::read_string(std::string *buffer, size_t len) {
	char ret[len];
	boost::asio::read(socket, boost::asio::buffer(ret, len));
	*buffer = std::string(ret);
}