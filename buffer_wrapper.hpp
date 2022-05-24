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

		Buffer &write8(uint8_t);

		Buffer &write16(uint16_t);

		Buffer &write32(uint32_t);

		Buffer &write_string(std::string);

		size_t size();

		char* get_data();

		void reset();

	private:
		char data[BUFFER_SIZE];
		char *ptr, *end;

		template<typename T>
		void write(T);
	};
}

#endif // BUFFER_WRAPPER_H