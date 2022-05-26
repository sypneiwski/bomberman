#ifndef CONNECTIONS_HPP
#define CONNECTIONS_HPP
#include <iostream>
#include <string>
#include <exception>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include <endian.h>
#include "program_options.hpp"

class BufferError : public std::exception {
public:
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

	size_t size() const;

	char* get_data();

	void clear();

private:
	char data[BUFFER_SIZE];
	char *ptr, *end;

	template<typename T>
	void write(T);
};

class Connection {
public:
	virtual void write(Buffer &buffer) = 0;

	Connection& read8(uint8_t&);

	Connection& read16(uint16_t&);

	Connection& read32(uint32_t&);

	Connection& read_string(std::string&);

protected:
	virtual void read(void*, size_t) = 0;	
};

class GUIReadError : public std::exception {};

class GUIConnection : public Connection {
public:
	GUIConnection(boost::asio::io_context&, Options&);

	void write(Buffer&) override;

	bool has_more() const;

private:
	using udp = boost::asio::ip::udp;
	udp::socket socket;
	udp::endpoint endpoint;

	static constexpr size_t MAX_UDP = 65535;
	char data[MAX_UDP];
	char *ptr, *end;

	void read(void*, size_t);
};

class ServerReadError : public std::exception {
public:
	const char * what () const throw () {
		return "Unable to parse message from server.";
	}
};

class ServerConnection : public Connection {
public:
	ServerConnection(boost::asio::io_context&, Options&);

	void write(Buffer&) override;

private:
	using tcp = boost::asio::ip::tcp;
	tcp::socket socket;

	void read(void*, size_t);
};

#endif // CONNECTIONS_HPP
