#include <iostream>
#include <string>
#include <exception>
#include <boost/asio.hpp>
#include <set>
#include <utility>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "program_options.hpp"
#include "messages.hpp"
#include "connections.hpp"
#include "misc.hpp"

namespace {
  // Mutex and conditional variable used for safely closing
  // all connections in case of an error.
  static std::mutex end_mutex;
  static std::condition_variable end_condition;
  static bool end{false};

  // Client class, responsible for establishing connection with the server
  // and GUI, receiving and sending messages and keeping track of thestate
  // of the game.
  class Client {
  private:
    enum struct GameState {
      Lobby = 0, Game = 1
    };

    std::mutex state_mutex;
    GameState game_state{GameState::Lobby};
    boost::asio::io_context io_context{};
    std::string player_name;
    TCPConnection server_conn;
    UDPConnection gui_conn;

    // Helper function to find all squares on the map
    // that have exploded in the current event.
    void calculate_explosions(const Event &event, ClientToGUI& out) {
      if (event.type != EventType::BombExploded)
        throw std::runtime_error("Something went wrong");
      static std::vector<std::pair<int32_t,int32_t>> sides = {
        {1,0}, {0,1}, {-1, 0}, {0, -1}
      };
      for (const std::pair<int32_t, int32_t> &side : sides) {
        Position p = out.bombs[event.bomb_id].position;
        int32_t x = p.x, y = p.y;
        for (uint16_t i = 0; i <= out.explosion_radius; i++) {
          p = Position((uint16_t) x, (uint16_t) y);
          out.explosions.insert(p);
          // If the explosion reaches a block, it stops.
          if (out.blocks.contains(p))
            break;
          x += side.first;
          y += side.second;
          if (x == -1 || x == out.size_x || y == -1 || y == out.size_y)
            break;
        }
      }
    }

    // Helper function to process one turn's events.
    void process_events(
      const std::vector<Event> &events,
      ClientToGUI& out,
      std::set<Player::PlayerId> &robots_destroyed,
      std::set<Position> &blocks_destroyed
      ) {
      for (const Event &event : events) {
        switch (event.type) {
          case EventType::BombPlaced:
            out.bombs[event.bomb_id] =
              Bomb(event.position, out.bomb_timer);
            break;
          case EventType::BombExploded:
            calculate_explosions(event, out);
            for (const Position &position : event.blocks_destroyed)
              blocks_destroyed.insert(position);
            for (const Player::PlayerId &id : event.robots_destroyed)
              robots_destroyed.insert(id);
            out.bombs.erase(event.bomb_id);
            break;
          case EventType::PlayerMoved:
            out.player_positions[event.player_id] = event.position;
            break;
          case EventType::BlockPlaced:
            out.blocks.insert(event.position);
        }
      }
    }

  public:
    Client(ClientOptions &options)
    : player_name(options.player_name),
      server_conn(io_context, options.server_address, options.server_port),
      gui_conn(io_context, options.port, options.gui_address, options.gui_port) {}

    ServerToClient receive_from_server() {
      ServerToClient in(server_conn);
      return in;
    }

    ClientToGUI& process_server_message(const ServerToClient& in) {
      // The `out` message will statically hold all information
      // about the game state that gui will use.
      static ClientToGUI out;
      static std::set<Player::PlayerId> robots_destroyed;
      static std::set<Position> blocks_destroyed;
      // Guards access to the game_state variable.
      std::lock_guard<std::mutex> lock(state_mutex);

      switch (in.type) {
        case ServerToClientType::Hello:
          debug("Received Hello from server");
          out.server_name = in.server_name;
          out.player_count = in.player_count;
          out.size_x = in.size_x;
          out.size_y = in.size_y;
          out.game_length = in.game_length;
          out.explosion_radius = in.explosion_radius;
          out.bomb_timer = in.bomb_timer;
          break;
        case ServerToClientType::AcceptedPlayer:
          debug("Received Accepted Player from server");
          out.players[in.player_id] = in.player;
          out.scores[in.player_id] = 0;
          break;
        case ServerToClientType::GameStarted:
          debug("Received Game Started from server");
          game_state = GameState::Game;
          out.players = in.players;
          out.scores.clear();
          for (const auto &[id, player] : out.players)
            out.scores[id] = 0;
          out.player_positions.clear();
          out.blocks.clear();
          out.bombs.clear();
          break;
        case ServerToClientType::Turn:
          debug("Received Turn from server");
          out.explosions.clear();
          robots_destroyed.clear();
          blocks_destroyed.clear();
          out.turn = in.turn;

          // Decrease bomb timers
          for (auto &[id, bomb] : out.bombs)
            bomb.timer--;

          // Process this turn's events.
          process_events(in.events, out, robots_destroyed, blocks_destroyed);

          // Calculate the scores for this turn and erase destroyed
          // blocks.
          for (const Position &position : blocks_destroyed)
            out.blocks.erase(position);
          for (const Player::PlayerId &id : robots_destroyed)
            out.scores[id]++;
          break;
        case ServerToClientType::GameEnded:
          debug("Received Game Ended from server");
          game_state = GameState::Lobby;
          out.players.clear();
          out.scores.clear();
          break;
      }
      out.type = static_cast<ClientToGUIType>(game_state);
      return out;
    }

    GUIToClient receive_from_gui() {
      GUIToClient in(gui_conn);
      if (gui_conn.has_more())
        throw GUIReadError();
      return in;
    }

    ClientToServer& process_gui_message(const GUIToClient& in) {
      static ClientToServer out;
      // Guards access to the game state variable.
      std::lock_guard<std::mutex> lock(state_mutex);

      // If the game is in lobby state, send a JOIN
      // message to the server regardless of `in` type.
      if (game_state == GameState::Lobby) {
        debug("Received Join from GUI");
        out.type = ClientToServerType::Join;
        out.name = player_name;
      }
      else {
        switch (in.type) {
          case GUIToClientType::PlaceBomb:
            debug("Received PlaceBomb from GUI");
            out.type = ClientToServerType::PlaceBomb;
            break;
          case GUIToClientType::PlaceBlock:
            debug("Received PlaceBlock from GUI");
            out.type = ClientToServerType::PlaceBlock;
            break;
          case GUIToClientType::Move:
            debug("Received Move from GUI");
            out.type = ClientToServerType::Move;
            out.direction = in.direction;
            break;
        }
      }

      return out;
    }

    void send_gui_message(ClientToGUI &message) {
      static Buffer serialized;
      message.serialize(serialized);
      gui_conn.write(serialized);
    }

    void send_server_message(ClientToServer &message) {
      static Buffer serialized;
      message.serialize(serialized);
      server_conn.write(serialized);
    }

    void close_sockets() {
      server_conn.close();
      gui_conn.close();
    }
  };

  // Handles exception in the worker threads
  // by waking up the main thread.
  void handle_exception(std::exception &e) {
    std::unique_lock lock(end_mutex);
    if (end)
      return;
    end = true;
    end_condition.notify_all();
    std::cerr << "ERROR : " << e.what() << "\n";
  }

  void server_messages_handler(Client &client) {
    for (;;) {
      try {
        ServerToClient in = client.receive_from_server();
        ClientToGUI out = client.process_server_message(in);
        if (in.type != ServerToClientType::GameStarted)
          client.send_gui_message(out);
      }
      catch (std::exception &e) {
        handle_exception(e);
        return;
      }
    }
  }

  void gui_messages_handler(Client &client) {
    for (;;) {
      try {
        GUIToClient in = client.receive_from_gui();
        ClientToServer out = client.process_gui_message(in);
        client.send_server_message(out);
      }
      catch (GUIReadError &e) {
        // If the message is corrupted, ignore it.
        continue;
      }
      catch (std::exception &e) {
        handle_exception(e);
        return;
      }
    }
  }
} // anonymous namespace

int main(int argc, char *argv[]) {
  try {
    ClientOptions options = ClientOptions(argc, argv);
    Client client(options);
    // Starting to listen for messages.
    std::thread server_thread(server_messages_handler, std::ref(client));
    std::thread gui_thread(gui_messages_handler, std::ref(client));
    debug("Listening for GUI messages on port " + std::to_string(options.port));

    // Waiting for any exceptions in the threads.
    std::unique_lock lock(end_mutex);
    end_condition.wait(lock, []{return end;});
    end_mutex.unlock();

    debug("Closing connection");
    client.close_sockets();
    server_thread.join();
    gui_thread.join();
    exit(EXIT_FAILURE);
  }
  catch (std::exception &e) {
    std::cerr << "ERROR : " << e.what() << "\n";
    exit(EXIT_FAILURE);
  }
  return 0;
}