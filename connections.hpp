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
class BufferError : public std::exception {
	const char * what () const throw () {
		return "Buffer error.";
	}
};

class Buffer {
public:
	static constexpr size_t BUFFER_SIZE = 65535;
	Buffer();

	Buffer &write8(uint8_t);

	Buffer &write16(uint16_t);

	Buffer &write32(uint32_t);

	Buffer &write_string(std::string);

	size_t size();

	char* get_data();

	void clear();

private:
	char data[BUFFER_SIZE];
	char *ptr, *end;

	template<typename T>
	void write(T);
};

class GUIConnection {
public:
	GUIConnection(boost::asio::io_context&, options::Options&);

	void write(Buffer&);

	GUIConnection& read8(uint8_t&);

	GUIConnection& read16(uint16_t&);

	GUIConnection& read32(uint32_t&);

	GUIConnection& read_string(std::string&);

private:
	using udp = boost::asio::ip::udp;
	udp::socket socket;
	udp::endpoint endpoint;

	static constexpr size_t MAX_UDP = 65535;
	char data[MAX_UDP];
	char *ptr, *end;

	template<typename T>
	void read(T*, size_t);
};

class ServerConnection {
public:
	ServerConnection(boost::asio::io_context&, options::Options&);

	ServerConnection& read8(uint8_t&);

	ServerConnection& read16(uint16_t&);

	ServerConnection& read32(uint32_t&);

	ServerConnection& read_string(std::string&);

private:
	using tcp = boost::asio::ip::tcp;
	tcp::socket socket;

	template<typename T>
	void read(T*, size_t);
};

#endif // CONNECTIONS_HPP
