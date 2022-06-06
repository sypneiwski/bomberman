#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <vector>
#include "program_options.hpp"
#include "messages.hpp"
#include "connections.hpp"

namespace {
  using tcp = boost::asio::ip::tcp;

  class Server {
  public:
    // Current game state.
    enum struct GameState {
      Lobby = 0, Game = 1
    };

    GameState game_state{GameState::Lobby};
    boost::asio::io_context io_context{};
    ServerOptions options;
    std::vector<std::thread> client_threads;

    Server(ServerOptions &options) : options(options) {}
  };

  void handle_client(tcp::socket &&socket) {
    TCPConnection client_connection(std::move(socket));
    ServerToClient out;

    
    std::cout << "succeeded\n";
  }

  void accept_new_connections(Server &server) {
    tcp::acceptor acc(
      server.io_context,
      tcp::endpoint(tcp::v6(), server.options.port)
    );
    for (;;) {
      tcp::socket socket(server.io_context);
      acc.accept(socket);
      std::cout << "received connection!\n";
      server.client_threads.push_back(std::thread(
        handle_client,
        std::move(socket)
      ));
    }
  }
} // anonymous namespace

int main(int argc, char *argv[]) {
  try {
    ServerOptions options = ServerOptions(argc, argv);
    std::cout
      << "bomb timer: " << options.bomb_timer << "\n"
      << "player count: " << options.players_count << "\n"
      << "turn duration: " << options.turn_duration << "\n"
      << "explosion radius: " << options.explosion_radius << "\n"
      << "initial blocks: " << options.initial_blocks << "\n"
      << "game length: " << options.game_length << "\n"
      << "server name: " << options.server_name << "\n"
      << "port: " << options.port << "\n"
      << "seed: " << options.seed << "\n"
      << "size x, y: " << options.size_x << " " << options.size_y << "\n\n\n";
    Server server(options);
    std::thread acceptor(accept_new_connections, std::ref(server));
    acceptor.join();
  }
  catch (std::exception &e) {
    std::cerr << "ERROR : " << e.what() << "\n";
    exit(EXIT_FAILURE);
  }
  return 0;
}