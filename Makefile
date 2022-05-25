CC = g++
CFLAGS = -Wall -Wextra -std=c++17 -I ../boost_1_79_0
LIBS = -L/usr/local/lib -lboost_program_options -pthread -lboost_thread -lboost_system

robots-client: main.o program_options.o messages.o connections.o
	$(CC) $(CFLAGS) -o $@ main.o program_options.o messages.o connections.o $(LIBS)

.cpp.o:
	$(CC) $(CFLAGS) -c $<
