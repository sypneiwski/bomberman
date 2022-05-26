#include <string>
#include <exception>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include <endian.h>
#include "connections.hpp"

Buffer::Buffer() : ptr(data), end(data + BUFFER_SIZE) {}

template<typename T>
void Buffer::write(T number) {
	if (ptr + sizeof(T) > end)
		throw BufferError();
	*(T *) ptr = number;
	ptr += sizeof(T);
}

Buffer &Buffer::write8(uint8_t number) {
	write(number);
	return *this;
}

Buffer &Buffer::write16(uint16_t number) {
	write(htobe16(number));
	return *this;
}

Buffer &Buffer::write32(uint32_t number) {
	write(htobe32(number));
	return *this;
}

Buffer &Buffer::write_string(std::string buffer) {
	uint8_t len = buffer.size();
	if (ptr + len > end)
		throw BufferError();
	write8(len);
	memcpy(ptr, buffer.c_str(), len);
	ptr += len;
	return *this;
}

size_t Buffer::size() const {
	return ptr - data;
}

char* Buffer::get_data() {
	return data;
}

void Buffer::clear() {
	ptr = data;
}

Connection& Connection::read8(uint8_t &number) {
	read(&number, sizeof(number));
	return *this;
}

Connection& Connection::read16(uint16_t &number) {
	read(&number, sizeof(number));
	number = be16toh(number);
	return *this;
}

Connection& Connection::read32(uint32_t &number) {
	read(&number, sizeof(number));
	number = be32toh(number);
	return *this;
}

Connection& Connection::read_string(std::string &buffer) {
	uint8_t len;
	read8(len);
	char ret[len + 1];
	memset(ret, 0, len + 1);
	read(ret, len);
	buffer = std::string(ret);
	return *this;
}

GUIConnection::GUIConnection(boost::asio::io_context &io_context, Options &options)
		: socket(io_context, udp::endpoint(udp::v6(), options.port)), ptr(data), end(data) {
	udp::resolver resolver(io_context);
	boost::system::error_code ec;
	auto endpoints = resolver.resolve(options.gui_address, options.gui_port, ec);
	if (ec)
		throw std::invalid_argument("Could not connect to GUI");
	endpoint = *endpoints.begin();	
}

void GUIConnection::write(Buffer &buffer) {
	socket.send_to(boost::asio::buffer(buffer.get_data(), buffer.size()), endpoint);
}

void GUIConnection::read(void* buffer, size_t size) {
	while (ptr + size > end) {
		size_t len = socket.receive(boost::asio::buffer(data, MAX_UDP));
		end = data + len;
		ptr = data;
	}
	memcpy(buffer, ptr, size);
	ptr += size;
}

bool GUIConnection::has_more() const {
	return ptr < end;
}

ServerConnection::ServerConnection(boost::asio::io_context &io_context, Options &options)
		: socket(io_context) {
	tcp::resolver resolver(io_context);
	boost::system::error_code ec;
	auto endpoints = resolver.resolve(options.server_address, options.server_port, ec);
	if (ec)
		throw std::invalid_argument("Could not resolve server address");
	boost::asio::connect(socket, endpoints, ec);
	socket.set_option(tcp::no_delay(true));
	if (ec)
		throw std::invalid_argument("Could not connect to server");
}

void ServerConnection::write(Buffer &buffer) {
	boost::asio::write(socket, boost::asio::buffer(buffer.get_data(), buffer.size()));
}

void ServerConnection::read(void* buffer, size_t size) {
	boost::asio::read(socket, boost::asio::buffer(buffer, size));
}