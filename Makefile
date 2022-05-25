CC = g++
CFLAGS = -Wall -Wextra -std=c++17
LIBS = -lboost_program_options -pthread -lboost_thread

robots-client: main.o program_options.o messages.o connections.o
	$(CC) $(CFLAGS) -o $@ main.o program_options.o messages.o connections.o $(LIBS)

.cpp.o:
	$(CC) $(CFLAGS) -c $<
