#include <endian.h>
#include <algorithm>
#include <cstring>
#include <iostream> //debug
#include "buffer_wrapper.hpp"

namespace buffers {
	Buffer::Buffer() : write_ptr(data), read_ptr(data), end(data + BUFFER_SIZE) {}

	Buffer::Buffer(const char* buffer, size_t size)
	: read_ptr(data), end(data + BUFFER_SIZE) {
		memcpy(data, buffer, std::min(size, BUFFER_SIZE));
		write_ptr = data + std::min(size, BUFFER_SIZE);
	}

	template<typename T>
	void Buffer::write(T number) {
		if (write_ptr + sizeof(T) > end)
			throw BufferError();
		*(T *) write_ptr = number;
		write_ptr += sizeof(T);
	}

	template<typename T>
	T Buffer::read() {
		if (read_ptr + sizeof(T) > write_ptr)
			throw BufferError();
		T ret = *(T *) read_ptr;
		read_ptr += sizeof(T);
		return ret;
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

	Buffer &Buffer::write_string(const char* buffer, size_t len) {
		if (write_ptr + len > end)
			throw BufferError();
		memcpy(write_ptr, buffer, len);
		write_ptr += len;
		return *this;
	}

	uint8_t Buffer::read8() {
		return read<uint8_t>();
	}

	uint16_t Buffer::read16() {
		return be16toh(read<uint16_t>());
	}

	uint32_t Buffer::read32() {
		return be32toh(read<uint32_t>());
	}

	void Buffer::read_string(std::string *buffer, size_t len) {
		if (read_ptr + len > write_ptr)
			throw BufferError();
		*buffer = std::string(read_ptr, len);
		read_ptr += len;
	}

	bool Buffer::has_more() {
		return read_ptr < write_ptr;
	}

	size_t Buffer::size() {
		return write_ptr - data;
	}

	void Buffer::reset() {
		write_ptr = data;
		read_ptr = data;
	}

	void Buffer::reset(const char* buffer, size_t size) {
		memcpy(data, buffer, std::min(size, BUFFER_SIZE));
		write_ptr = data + std::min(size, BUFFER_SIZE);
		read_ptr = data;
	}
}