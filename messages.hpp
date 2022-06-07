#ifndef MESSAGES_HPP
#define MESSAGES_HPP
#include <string>
#include <vector>
#include <exception>
#include <map>
#include <set>
#include "connections.hpp"

// This file includes declarations for structures used for
// serializing and deserializing messages.

class GUIReadError : public std::exception {};

class ClientReadError : public std::exception {};

class ServerReadError : public std::exception {
public:
  const char * what () const throw () {
    return "Unable to parse message from server";
  }
};

enum struct Direction : uint8_t {
  Up = 0, Right = 1, Down = 2, Left = 3, MAX = 3
};

struct Position {
  uint16_t x, y;
  bool operator<(const Position&) const;

  Position() = default;
  Position(uint16_t, uint16_t);
  Position(Connection&);
  void serialize(Buffer&) const;
};

struct Player {
  using PlayerId = uint8_t;
  using Score = uint32_t;
  std::string name, address;

  Player() = default;
  Player(std::string, std::string);
  Player(Connection&);
  void serialize(Buffer&) const;
};

struct Bomb {
  using BombId = uint32_t;
  Position position;
  uint16_t timer;

  Bomb() = default;
  Bomb(Position, uint16_t);
  void serialize(Buffer&) const;
};

enum struct ClientToServerType : uint8_t {
  Join = 0, PlaceBomb = 1, PlaceBlock = 2, Move = 3, MAX = 3
};

// Struct holding data for messages to the server.
struct ClientToServer {
  ClientToServerType type;
  std::string name;
  Direction direction;

  ClientToServer() = default;
  ClientToServer(Connection&);
  void serialize(Buffer&) const;
};

enum struct EventType : uint8_t {
  BombPlaced = 0, BombExploded = 1, PlayerMoved = 2, 
  BlockPlaced = 3, MAX = 3
};

struct Event {
  EventType type;
  Bomb::BombId bomb_id;
  Player::PlayerId player_id;
  Position position;
  std::vector<Player::PlayerId> robots_destroyed;
  std::vector<Position> blocks_destroyed;

  Event() = default;
  Event(Connection&);
  void serialize(Buffer&) const;
};

enum struct ServerToClientType : uint8_t {
  Hello = 0, AcceptedPlayer = 1, GameStarted = 2, 
  Turn = 3, GameEnded = 4, MAX = 4
};

// Struct holding data for messages from the server.
struct ServerToClient {
  ServerToClientType type;
  std::string server_name;
  uint8_t player_count;
  uint16_t size_x, size_y;
  uint16_t game_length;
  uint16_t explosion_radius, bomb_timer;
  Player::PlayerId player_id;
  Player player;
  std::map<Player::PlayerId, Player> players;
  uint16_t turn;
  std::vector<Event> events;
  std::map<Player::PlayerId, Player::Score> scores;

  ServerToClient() = default;
  ServerToClient(Connection&);
  void serialize(Buffer&) const;
};

enum struct GUIToClientType : uint8_t {
  PlaceBomb = 0, PlaceBlock = 1, Move = 2, MAX = 2
};

// Struct holding data for messages from GUI.
struct GUIToClient {
  GUIToClientType type;
  Direction direction;

  GUIToClient(Connection&);
};

enum struct ClientToGUIType : uint8_t {
  Lobby = 0, Game = 1, MAX = 1
};

// Struct holding data for messages to GUI.
struct ClientToGUI {
  ClientToGUIType type;
  std::string server_name;
  uint8_t player_count;
  uint16_t size_x, size_y;
  uint16_t game_length;
  uint16_t explosion_radius, bomb_timer;
  std::map<Player::PlayerId, Player> players;
  uint16_t turn;
  std::map<Player::PlayerId, Position> player_positions;
  std::set<Position> blocks;
  std::map<Bomb::BombId, Bomb> bombs;
  std::set<Position> explosions;
  std::map<Player::PlayerId, Player::Score> scores;

  ClientToGUI() = default;
  void serialize(Buffer&) const;
};

#endif // MESSAGES_HPP