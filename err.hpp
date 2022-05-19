#ifndef ERR_H
#define ERR_H
#include <stdlib.h>
#include <iostream>

// Exit with an error message.
void fatal(const char *str) {
	std::cerr << "Error: " << str << "\n";
	exit(EXIT_FAILURE);
}

// Check if errno is non-zero, and if so, print an error message and exit with
// an error.
void print_errno(const char *str) {
	if (errno != 0) {
		std::cerr << "Error: " << str << ", errno " << errno << "\n";
		exit(EXIT_FAILURE);
	}
}

// Check if condition is met, and if not, print an error message and exit with
// an error.
void ensure(bool condition, const char *str) {
	if (!condition)
		fatal(str);
}

#endif // ERR_H