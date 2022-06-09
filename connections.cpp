#include <string>
#include <exception>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include <endian.h>
#include "connections.hpp"

Buffer::Buffer() : ptr(data), end(data + BUFFER_SIZE) {}

template<typename T>
void Buffer::write(T number) {
  uint8_t *ptr = (uint8_t *) &number; 
  data.insert(data.end(), ptr, ptr + sizeof(T));
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
  uint8_t len = (uint8_t) buffer.size();
  write8(len);
  data.insert(data.end(), buffer.c_str(), buffer.c_str() + len);
  return *this;
}

void Buffer::clear() {
  data.clear();
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

UDPConnection::UDPConnection(
  boost::asio::io_context &io_context, 
  uint16_t &local_port,
  std::string &remote_address,
  std::string &remote_port)
  : socket(io_context, udp::endpoint(udp::v6(), local_port)), 
    ptr(data), 
    end(data) {
  udp::resolver resolver(io_context);
  boost::system::error_code ec;
  auto endpoints = resolver.resolve(
    remote_address, 
    remote_port, 
    ec
  );
  if (ec)
    throw std::invalid_argument("Could not connect to GUI");
  endpoint = *endpoints.begin();	
}

void UDPConnection::write(Buffer &buffer) {
  socket.send_to(
    boost::asio::buffer(buffer.data), 
    endpoint
  );
  buffer.clear();
}

void UDPConnection::read(void* buffer, size_t size) {
  while (ptr + size > end) {
    // Wait for datagram big enough to satisfy request.
    size_t len = socket.receive(boost::asio::buffer(data, MAX_UDP));
    end = data + len;
    ptr = data;
  }
  memcpy(buffer, ptr, size);
  ptr += size;
}

bool UDPConnection::has_more() const {
  return ptr < end;
}

void UDPConnection::close() {
  if (closed)
    throw std::runtime_error("Connection already closed");
  closed = true;
  boost::system::error_code ec;
  socket.shutdown(udp::socket::shutdown_both, ec);
  socket.close();
}

TCPConnection::TCPConnection(
  boost::asio::io_context &io_context, 
  std::string &address,
  std::string &port)
  : socket(io_context) {
  tcp::resolver resolver(io_context);
  boost::system::error_code ec;
  auto endpoints = resolver.resolve(
    address,
    port,
    ec
  );
  if (ec)
    throw std::invalid_argument("Could not resolve server address");
  boost::asio::connect(socket, endpoints, ec);
  socket.set_option(tcp::no_delay(true));
  if (ec)
    throw std::invalid_argument("Could not connect to server");
}

TCPConnection::TCPConnection(tcp::socket &&socket)
: socket(std::move(socket)) {}

void TCPConnection::write(Buffer &buffer) {
  boost::asio::write(
    socket, 
    boost::asio::buffer(buffer.data)
  );
  buffer.clear();
}

void TCPConnection::read(void* buffer, size_t size) {
  boost::asio::read(socket, boost::asio::buffer(buffer, size));
}

void TCPConnection::close() {
  if (closed)
    throw std::runtime_error("Connection already closed");
  closed = true;
  socket.close();
}