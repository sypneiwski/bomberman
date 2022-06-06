#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <vector>
#include <conditional_variable>
#include <map>
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

    // Guards game state variables.
    std::mutex server_mutex;
    GameState game_state{GameState::Lobby};
    uint8_t current_id = 0;
    std::conditional_variable new_players;
    std::map<Player::PlayerId, Player> players;

    boost::asio::io_context io_context{};
    ServerOptions options;
    std::vector<std::thread> client_threads;

    Server(ServerOptions &options) : options(options) {}
  };

  struct Client {
    TCPConnection conn;
    std::string address;

    Client(tcp::socket &&socket, std::string address) 
    : conn(std::move(socket)),
      address(address) {}
  };

  using ClientPtr = std::shared_ptr<Client>;
  void send_to_client(Server &server, ClientPtr client) {
    Buffer buffer;
    ServerToClient out;

    // Send hello message.
    out.type = ServerToClientType::Hello;
    out.server_name = server.options.server_name;
    out.player_count = server.options.players_count;
    out.size_x = server.options.size_x;
    out.size_y = server.options.size_y;
    out.game_length = server.options.game_length;
    out.explosion_radius = server.options.explosion_radius;
    out.bomb_timer = server.options.bomb_timer;
    out.serialize(buffer);
    client->conn.write(buffer);

    std::unique_lock lock(server.state_mutex);
    // send all accepted players
    while (server.current_id < server.options.players_count) {
      uint8_t current_id = server.current_id;
      cv.wait(lock, []{return server.current_id != current_id});
      // send new accepted player
    }
  }

  void receive_from_client(Server &server, ClientPtr client) {
    for (;;) {
      ClientToServer in(client->conn);
      std::cout << server.options.players_count << "\n";
    }
  }

  void accept_new_connections(Server &server) {
    tcp::acceptor acc(
      server.io_context,
      tcp::endpoint(tcp::v6(), server.options.port)
    );
    for (;;) {
      tcp::socket socket(server.io_context);
      acc.accept(socket);
      std::ostringstream client_address;
      client_address << socket.remote_endpoint();
      ClientPtr client = std::make_shared<Client>(
        std::move(socket),
        client_address.str()
      );

      std::cout << "received connection from " + client_address.str() + "!\n";
      server.client_threads.push_back(std::thread(
        send_to_client,
        std::ref(server),
        client
      ));
      server.client_threads.push_back(std::thread(
        receive_from_client,
        std::ref(server),
        client
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