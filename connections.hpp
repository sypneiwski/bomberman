#ifndef CONNECTIONS_HPP
#define CONNECTIONS_HPP
#include <iostream>
#include <string>
#include <exception>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include <endian.h>
#include "program_options.hpp"
#include "buffer_wrapper.hpp"

//todo namespace

class GUIConnection {
public:
	GUIConnection(boost::asio::io_context&, options::Options&);

	void write(buffers::Buffer&);

private:
	using udp = boost::asio::ip::udp;
	udp::socket socket;
	udp::endpoint endpoint;
	static constexpr size_t MAX_UDP = 65535;
	char data[MAX_UDP];
	char *ptr;
};

class ServerConnection {
public:
	ServerConnection(boost::asio::io_context&, options::Options&);

	uint8_t read8();

	uint16_t read16();

	uint32_t read32();

	void read_string(std::string*, size_t);

private:
	using tcp = boost::asio::ip::tcp;
	tcp::socket socket;

	template<typename T>
	T read();
};

#endif // CONNECTIONS_HPP
