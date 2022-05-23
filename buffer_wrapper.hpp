#ifndef BUFFER_WRAPPER_H
#define BUFFER_WRAPPER_H
#include <exception>
#include <cstdint>

namespace buffers {
	class BufferError : public std::exception {
		const char * what () const throw () {
			return "Buffer error.";
		}
	};

	class Buffer {
	public:
		static constexpr size_t BUFFER_SIZE = 65535;
		Buffer();

		Buffer(const char*, size_t);

		Buffer &write8(uint8_t);

		Buffer &write16(uint16_t);

		Buffer &write32(uint32_t);

		Buffer &write_string(const char*, size_t);

		uint8_t read8();

		uint16_t read16();

		uint32_t read32();

		void read_string(std::string*, size_t);

		bool has_more();

		size_t size();

		void reset();

		void reset(const char*, size_t);

	private:
		char data[BUFFER_SIZE];
		char *write_ptr, *read_ptr, *end;

		template<typename T>
		void write(T);

		template<typename T>
		T read();
	};
}

#endif // BUFFER_WRAPPER_H