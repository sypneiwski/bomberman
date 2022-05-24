#include <endian.h>
#include <algorithm>
#include <cstring>
#include <iostream> //debug
#include "buffer_wrapper.hpp"

namespace buffers {
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
		size_t len = buffer.size();
		if (ptr + len > end)
			throw BufferError();
		memcpy(ptr, buffer.c_str(), len);
		ptr += len;
		return *this;
	}

	size_t Buffer::size() {
		return ptr - data;
	}

	char* Buffer::get_data() {
		return data;
	}

	void Buffer::reset() {
		ptr = data;
	}
}