#ifndef CONNECTIONS_HPP
#define CONNECTIONS_HPP
#include <iostream>
#include <string>
#include <exception>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include <endian.h>

class BufferError : public std::exception {
public:
  const char * what () const throw () {
    return "Buffer error.";
  }
};

// Helper class for serializing outgoing messages.
// The writeX functions convert binary numbers to network order.
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

// Abstract base class for connections.
// Connection classes provide the ability to read from
// and write to sockets.
class Connection {
public:
  virtual void write(Buffer &buffer) = 0;

  Connection& read8(uint8_t&);

  Connection& read16(uint16_t&);

  Connection& read32(uint32_t&);

  Connection& read_string(std::string&);

  virtual void close() = 0;

protected:
  virtual void read(void*, size_t) = 0;	
};

// Class wrapping the boost UDP socket.
class UDPConnection : public Connection {
public:
  UDPConnection(
    boost::asio::io_context&, 
    uint16_t&, 
    std::string&, 
    std::string&
  );

  void write(Buffer&) override;

  bool has_more() const;

  void close() override;

private:
  using udp = boost::asio::ip::udp;
  udp::socket socket;
  udp::endpoint endpoint;

  static constexpr size_t MAX_UDP = 65535;
  char data[MAX_UDP];
  char *ptr, *end;

  void read(void*, size_t) override;
};

// Class wrapping the boost TCP socket
class TCPConnection : public Connection {
public:
  TCPConnection(boost::asio::io_context&, std::string&, std::string&);

  TCPConnection(boost::asio::ip::tcp::socket&&);

  void write(Buffer&) override;

  void close() override;

private:
  using tcp = boost::asio::ip::tcp;
  tcp::socket socket;

  void read(void*, size_t) override;
};

#endif // CONNECTIONS_HPP
