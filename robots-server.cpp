#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <vector>
#include <condition_variable>
#include <map>
#include <chrono>
#include <random>
#include <string>
#include "program_options.hpp"
#include "messages.hpp"
#include "connections.hpp"
#include "misc.hpp"

namespace {
  using tcp = boost::asio::ip::tcp;

  // Server class, holding all game related variables.
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
    std::map<Player::PlayerId, Position> player_positions;
    std::set<Position> blocks;
    std::map<Bomb::BombId, Bomb> bombs;
    Bomb::BombId current_bomb{0};

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

    // Function for adding joining players during Lobby state.
    bool add_player(std::string name, std::string address, uint8_t &id) {
      std::unique_lock lock(server_mutex);
      // If game has already started, ignore join messages.
      if (game_state == GameState::Game)
        return false;
      
      debug("[Server] player " + name + " joined");
      players[current_id] = Player(name, address);
      id = current_id;
      current_id++;

      // Alert all sender threads.
      new_players.notify_all();
      // If enough players joined, start game.
      if (current_id == options.players_count) {
        game_state = GameState::Game;
        current_turn = 0;
        turns.clear();
        for (uint8_t id = 0; id < options.players_count; id++)
          scores[id] = 0;
        // Alert game handler.
        game_start.notify_all();
      }
      return true;
    }

    // Function for reseting game state and starting new game.
    void end_game() {
      std::unique_lock lock(server_mutex);
      debug("[Server] Game ended");
      game_state = GameState::Lobby;
      iteration++;
      current_id = 0;
      players.clear();
    }
  };

  // Struct holding client connection and address, to be shared by
  // receiving and sending threads.
  struct Client {
    TCPConnection conn;
    std::string address;

    Client(tcp::socket &&socket, std::string address) 
    : conn(std::move(socket)),
      address(address) {}
  };
  using ClientPtr = std::shared_ptr<Client>;

  // Function for sending all messages to the client.
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

        // Locking mutex for access to the player map.
        std::unique_lock lock(server.server_mutex);
        debug("Sent hello message to client " + client->address);
        while (server.game_state == Server::GameState::Lobby) {
          // Wait for new players.
          server.new_players.wait(
            lock,
            [&]{return server.current_id != current_id;}
          );
          // Send new accepted player messages.
          for (; current_id < server.current_id; current_id++) {
            out.type = ServerToClientType::AcceptedPlayer;
            out.player_id = current_id;
            out.player = server.players[current_id];
            out.serialize(serialized);
            client->conn.write(serialized);
            debug("Sent accepted player message to client " + client->address);
          }
        }
        // Sending game started.
        out.type = ServerToClientType::GameStarted;
        out.players = server.players;
        out.serialize(serialized);
        client->conn.write(serialized);

        while (server.game_state == Server::GameState::Game) {
          // Wait for the next turn.
          server.new_turn.wait(
            lock,
            [&]{return server.current_turn != current_turn;}
          );
          // Send turn messages.
          for (; current_turn < server.current_turn; current_turn++) {
            out.type = ServerToClientType::Turn;
            out.turn = current_turn;
            out.events = server.turns[current_turn];
            out.serialize(serialized);
            client->conn.write(serialized);
            debug("Sent turn message to client " + client->address);
          }
        }
        //Sending game ended.
        out.type = ServerToClientType::GameEnded;
        out.scores = server.scores;
        out.serialize(serialized);
        client->conn.write(serialized);
        debug("Sent game ended to client " + client->address);
      }
      catch (std::exception &e) {
        try {
        client->conn.close();
        }
        catch (std::exception &e) {}
      }
    }
  }

  // Function listening for messages from the client.
  void receive_from_client(Server &server, ClientPtr client) {
    uint32_t current_iteration = 0;
    bool joined = false;
    uint8_t id = 0;
    try {
      for (;;) {
        ClientToServer in(client->conn);
        {
          std::unique_lock lock(server.server_mutex);
          // If a new game has begun, reset join status.
          if (server.iteration > current_iteration) {
            joined = false;
            current_iteration = server.iteration;
          }
        }
        switch (in.type) {
          case ClientToServerType::Join:
            if (joined)
              break;
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
      try {
      client->conn.close();
      }
      catch (std::exception &e) {}
    }
  }

  void accept_new_connections(Server &server) {
    tcp::acceptor acceptor(
      server.io_context,
      tcp::endpoint(tcp::v6(), server.options.port)
    );
    for (;;) {
      tcp::socket socket(server.io_context);
      acceptor.accept(socket);
      socket.set_option(tcp::no_delay(true));
      std::ostringstream client_address;
      client_address << socket.remote_endpoint();
      ClientPtr client = std::make_shared<Client>(
        std::move(socket),
        client_address.str()
      );

      debug("[Acceptor] Accepted connection from client " + 
            client_address.str()
      );
      // Start both client threads.
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

  // Helper function for processing bomb explosions.
  void process_bombs(
    Server &server, 
    std::vector<Event> &current_events,
    std::set<Player::PlayerId> &robots_destroyed
    ) {
    static std::vector<std::pair<int32_t,int32_t>> sides = {
      {1,0}, {0,1}, {-1, 0}, {0, -1}
    };
    std::set<Position> blocks_destroyed;
    std::set<Bomb::BombId> bombs_exploded;
    for (auto &[bomb_id, bomb] : server.bombs) {
      bomb.timer--;
      if (bomb.timer == 0) {
        Event event;
        event.type = EventType::BombExploded;
        event.bomb_id = bomb_id;

        for (const std::pair<int32_t, int32_t> &side : sides) {
          Position position = bomb.position;
          int32_t x = position.x, y = position.y;
          for (uint16_t i = 0; i <= server.options.explosion_radius; i++) {
            position = Position((uint16_t) x, (uint16_t) y);
            // Check if any robots were destroyed.
            for (const auto &[id, p] : server.player_positions) {
              if (position.x == p.x && position.y == p.y) {
                robots_destroyed.insert(id);
                if (i > 0 || side == sides[0])
                  event.robots_destroyed.push_back(id);
              }
            }
            // If the explosion reaches a block, it stops.
            if (server.blocks.contains(position)) {
              blocks_destroyed.insert(position);
              if (i > 0 || side == sides[0])
                event.blocks_destroyed.push_back(position);
              break;
            }
            x += side.first;
            y += side.second;
            if (x == -1 || x == server.options.size_x || 
                y == -1 || y == server.options.size_y)
              break;
          }
        }
        current_events.push_back(event);
        bombs_exploded.insert(bomb_id);
      }
    }
    for (const Position &position : blocks_destroyed)
      server.blocks.erase(position);
    for (const Bomb::BombId &bomb_id : bombs_exploded)
      server.bombs.erase(bomb_id);
  }

  // Helper function for processing one turn.
  void process_turn(Server &server, std::vector<Event> &current_events) {
    std::set<Player::PlayerId> robots_destroyed;
    
    process_bombs(server, current_events, robots_destroyed);

    Event event;
    for (uint8_t id = 0; id < server.options.players_count; id++) {
      // Ignore destroyed robots' moves.
      if (robots_destroyed.contains(id)) {
        event.type = EventType::PlayerMoved;
        event.player_id = id;
        event.position = Position(
          (uint16_t) random() % server.options.size_x,
          (uint16_t) random() % server.options.size_y
        );
        server.player_positions[id] = event.position;
        current_events.push_back(event);
        server.scores[id]++;
      }
      else {
        std::unique_lock player_lock(server.player_moves_mutex[id]);
        ClientToServer move = server.player_moves[id];
        player_lock.unlock();

        switch (move.type) {
          case ClientToServerType::PlaceBomb:
            event.type = EventType::BombPlaced;
            event.bomb_id = server.current_bomb;
            event.position = server.player_positions[id];
            server.bombs[server.current_bomb] = Bomb(
              event.position, 
              server.options.bomb_timer
            );
            server.current_bomb++;
            current_events.push_back(event);
            break;
          case ClientToServerType::PlaceBlock:
            if (!server.blocks.contains(server.player_positions[id])) {
              event.type = EventType::BlockPlaced;
              event.position = server.player_positions[id];
              server.blocks.insert(server.player_positions[id]);
              current_events.push_back(event);
            }
            break;
          case ClientToServerType::Move:
            {
            event.type = EventType::PlayerMoved;
            event.player_id = id;
            event.position = server.player_positions[id];
            uint16_t x = event.position.x, y = event.position.y;
            switch (move.direction) {
              case Direction::Up:
                if (y + 1 < server.options.size_y)
                  event.position = Position(x, (uint16_t) (y + 1));
                break;
              case Direction::Right:
                if (x + 1 < server.options.size_x)
                  event.position = Position((uint16_t) (x + 1), y);
                break;
              case Direction::Down:
                if (y - 1 >= 0)
                  event.position = Position(x, (uint16_t) (y - 1));
                break;
              case Direction::Left:
                if (x - 1 >= 0)
                  event.position = Position((uint16_t) (x - 1), y);
                break;
            }
            if (server.blocks.contains(event.position))
              break;
            if (event.position.x != x || event.position.y != y) {
              server.player_positions[id] = event.position;
              current_events.push_back(event);
            }
            break;
            }
          case ClientToServerType::Join:
            // Player did not send a move this turn.
            break;
        } 
      }
    }
  }

  // Handles game logic.
  void handle_game(Server &server) {
    for (;;) {
      server.player_positions.clear();
      server.blocks.clear();
      server.bombs.clear();
      server.current_bomb = 0;
      std::vector<Event> current_events;

      {
        // Wait for the start of the game.
        std::unique_lock lock(server.server_mutex);
        server.game_start.wait(
          lock,
          [&]{return server.game_state == Server::GameState::Game;}
        );
      }

      // Prepare the first turn message.
      for (uint8_t id = 0; id < server.options.players_count; id++) {
        Event event;
        event.type = EventType::PlayerMoved;
        event.player_id = id;
        event.position = Position(
          (uint16_t) random() % server.options.size_x,
          (uint16_t) random() % server.options.size_y
        );
        server.player_positions[id] = event.position;
        current_events.push_back(event);
      }
      for (uint16_t i = 0; i < server.options.initial_blocks; i++) {
        Event event;
        event.type = EventType::BlockPlaced;
        event.position = Position(
          (uint16_t) random() % server.options.size_x,
          (uint16_t) random() % server.options.size_y
        );
        // Check if there was already a block at this position.
        if (server.blocks.insert(event.position).second)
          current_events.push_back(event);
      }
      
      for (uint16_t turn = 0; turn <= server.options.game_length; turn++) {
        {
          std::unique_lock lock(server.server_mutex);
          server.current_turn++;
          server.turns.push_back(current_events);
          current_events.clear();
          debug("[Game Handler] Turn processed");
        }

        for (uint8_t id = 0; id < server.options.players_count; id++) {
          std::unique_lock player_lock(server.player_moves_mutex[id]);
          // Resets requested player moves.
          server.player_moves[id] = ClientToServer();
        }

        server.new_turn.notify_all();
        if (turn == server.options.game_length)
          break;
        std::this_thread::sleep_for(std::chrono::milliseconds(
          server.options.turn_duration
        ));

        process_turn(server, current_events);
      }
      server.end_game();
      server.new_turn.notify_all();
    }
  }
} // anonymous namespace

int main(int argc, char *argv[]) {
  try {
    ServerOptions options = ServerOptions(argc, argv);
    debug("[Server] Listening for clients on port " + 
          std::to_string(options.port)
    );
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
