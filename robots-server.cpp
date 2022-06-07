#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <vector>
#include <condition_variable>
#include <map>
#include <chrono>
#include <random>
#include "program_options.hpp"
#include "messages.hpp"
#include "connections.hpp"

namespace {
  using tcp = boost::asio::ip::tcp;

  class Server {
  public:
    enum struct GameState {
      Lobby = 0, Game = 1
    };

    // Game state variables.
    GameState game_state{GameState::Lobby};
    uint8_t current_id{0};
    std::map<Player::PlayerId, Player> players;
    std::map<Player::PlayerId, Player::Score> scores;
    uint16_t current_turn{0};
    std::vector<std::vector<Event>> turns;

    // Server variables.
    boost::asio::io_context io_context{};
    std::mutex server_mutex;
    std::condition_variable new_players, game_start, new_turn;
    ServerOptions options;
    std::minstd_rand random;
    uint32_t iteration{0};

    // Variables for client connections handling.
    std::vector<std::mutex> player_moves_mutex;
    std::vector<ClientToServer> player_moves;

    Server(ServerOptions &options) 
    : options(options),
      random(options.seed),
      player_moves_mutex((size_t) options.players_count),
      player_moves((size_t) options.players_count) {}

    bool add_player(std::string name, std::string address, uint8_t &id) {
      std::unique_lock lock(server_mutex);
      if (game_state == GameState::Game) {
        std::cout << "ignoring\n";
        return false;
      }
      
      players[current_id] = Player(name, address);
      id = current_id;
      current_id++;
      std::cout << "players now at " << (int)current_id << "\n";

      new_players.notify_all();
      if (current_id == options.players_count) {
        game_state = GameState::Game;
        scores.clear();
        game_start.notify_all();
      }
      return true;
    }

    void end_game() {
      std::unique_lock lock(server_mutex);
      game_state = GameState::Lobby;
      iteration++;
      current_id = 0;
      players.clear();
      current_turn = 0;
      turns.clear();
    }
  };

  struct Client {
    TCPConnection conn;
    std::string address;
    uint32_t thread_id;

    Client(tcp::socket &&socket, std::string address, uint32_t id) 
    : conn(std::move(socket)),
      address(address),
      thread_id(id) {}
  };
  using ClientPtr = std::shared_ptr<Client>;

  void send_to_client(Server &server, ClientPtr client) {
    for (;;) {
      Buffer serialized;
      ServerToClient out;
      uint8_t current_id = 0;
      uint16_t current_turn = 0;

      try {
        // Send hello message.
        out.type = ServerToClientType::Hello;
        out.server_name = server.options.server_name;
        out.player_count = server.options.players_count;
        out.size_x = server.options.size_x;
        out.size_y = server.options.size_y;
        out.game_length = server.options.game_length;
        out.explosion_radius = server.options.explosion_radius;
        out.bomb_timer = server.options.bomb_timer;
        out.serialize(serialized);
        client->conn.write(serialized);
        std::cout << "Sent hello message\n";

        // Locking mutex for access to the player map.
        std::unique_lock lock(server.server_mutex);
        // Send accepted player messages.
        while (server.game_state == Server::GameState::Lobby) {
          server.new_players.wait(
            lock,
            [&]{return server.current_id != current_id;}
          );
          // Send new accepted players.
          for (; current_id < server.current_id; current_id++) {
            out.type = ServerToClientType::AcceptedPlayer;
            out.player_id = current_id;
            out.player = server.players[current_id];
            out.serialize(serialized);
            client->conn.write(serialized);
            std::cout << "sent accepted player\n";
          }
        }
        // Sending game started.
        out.type = ServerToClientType::GameStarted;
        out.players = server.players; // Maybe a reference somehow?
        out.serialize(serialized);
        client->conn.write(serialized);
        std::cout << "sent game started!\n";

        while (server.game_state == Server::GameState::Game) {
          server.new_turn.wait(
            lock,
            [&]{return server.current_turn != current_turn;}
          );
          // Send turns.
          for (; current_turn < server.current_turn; current_turn++) {
            out.type = ServerToClientType::Turn;
            out.turn = current_turn;
            out.events = server.turns[current_turn]; // Maybe a reference somehow?
            out.serialize(serialized);
            client->conn.write(serialized);
            std::cout << "sent turn\n";
          }
        }
        //Sending game ended.
        out.type = ServerToClientType::GameEnded;
        out.scores = server.scores; // Maybe a reference somehow?
        out.serialize(serialized);
        client->conn.write(serialized);
        std::cout << "sent game ended\n";
      }
      catch (std::exception &e) {
        client->conn.close();
      }
    }
  }

  void receive_from_client(Server &server, ClientPtr client) {
    uint32_t current_iteration = 0;
    bool joined = false;
    uint8_t id = 0;
    try {
      for (;;) {
        ClientToServer in(client->conn);
        {
          std::unique_lock lock(server.server_mutex);
          if (server.iteration > current_iteration) {
            joined = false;
            current_iteration = server.iteration;
          }
        }
        switch (in.type) {
          case ClientToServerType::Join:
            if (joined)
              break;
            std::cout << "received join message\n";
            if (server.add_player(in.name, client->address, id))
              joined = true;
            break;
          default:
            if (!joined)
              break;
            std::unique_lock lock(server.player_moves_mutex[id]);
            server.player_moves[id] = in;
            break;
        }
      }
    }
    catch (std::exception &e) {
      client->conn.close();
      std::cout << "closing client\n";
    }
  }

  void accept_new_connections(Server &server) {
    uint32_t id = 0;
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
        client_address.str(),
        id++
      );

      std::cout << "accepted connection from " + client_address.str() + "!\n";
      std::thread sender(
        send_to_client,
        std::ref(server),
        client
      );
      std::thread receiver(
        receive_from_client,
        std::ref(server),
        client
      );
      sender.detach();
      receiver.detach();
    }
  }

  void handle_game(Server &server) {
    for (;;) {
      // Handles game logic.
      std::vector<Event> current_events;
      std::map<Player::PlayerId, Position> player_positions;
      std::set<Position> blocks;
      std::map<Bomb::BombId, Bomb> bombs;
      Event event;

      std::unique_lock lock(server.server_mutex);
      server.game_start.wait(
        lock,
        [&]{return server.game_state == Server::GameState::Game;}
      );
      lock.unlock();

      // Prepare first turn message.
      for (uint8_t id = 0; id < server.options.players_count; id++) {
        event.type = EventType::PlayerMoved;
        event.player_id = id;
        event.position = Position(
          (uint16_t) random() % server.options.size_x,
          (uint16_t) random() % server.options.size_y
        );
        current_events.push_back(event);
      }
      for (uint16_t i = 0; i < server.options.initial_blocks; i++) {
        event.type = EventType::BlockPlaced;
        event.position = Position(
          (uint16_t) random() % server.options.size_x,
          (uint16_t) random() % server.options.size_y
        );
        current_events.push_back(event);
      }
      
      for (uint16_t turn = 0; turn < server.options.game_length; turn++) {
        lock.lock();
        server.current_turn++;
        server.turns.push_back(current_events);
        current_events.clear();
        lock.unlock();
        for (uint8_t id = 0; id < server.options.players_count; id++) {
          std::unique_lock player_lock(server.player_moves_mutex[id]);
          // Resets requested player moves.
          server.player_moves[id] = ClientToServer();
        }

        server.new_turn.notify_all();
        std::this_thread::sleep_for(std::chrono::milliseconds(
          server.options.turn_duration
        ));

        // process turn :(((
      }
      server.new_turn.notify_all();
      server.end_game();
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
    std::thread game_handler(handle_game, std::ref(server));
    std::thread acceptor(accept_new_connections, std::ref(server));
    acceptor.join();
    game_handler.join();
  }
  catch (std::exception &e) {
    std::cerr << "ERROR : " << e.what() << "\n";
    exit(EXIT_FAILURE);
  }
  return 0;
}
