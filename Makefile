CC = g++
CFLAGS = -std=gnu++20 -Wall -Wextra -Wconversion -Werror -O2
LIBS = -lboost_program_options -pthread

.PHONY: all clean

all: robots-client robots-server

robots-client: robots-client.o program_options.o messages.o connections.o
	$(CC) $(CFLAGS) -o $@ robots-client.o program_options.o messages.o connections.o $(LIBS)

robots-server: robots-server.o program_options.o messages.o connections.o
	$(CC) $(CFLAGS) -o $@ robots-server.o program_options.o messages.o connections.o $(LIBS)

.cpp.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f robots-client robots-server *.o
