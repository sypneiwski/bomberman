CC = g++
CFLAGS = -std=gnu++20 -Wall -Wextra -Wconversion -Werror -O2
LIBS = -lboost_program_options -pthread -lboost_system

.PHONY: clean

robots-client: robots-client.o program_options.o messages.o connections.o
	$(CC) $(CFLAGS) -o $@ robots-client.o program_options.o messages.o connections.o $(LIBS)

.cpp.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f robots-client *.o
