#ifndef MESSAGES_HPP
#define MESSAGES_HPP
#include <string>
#include <vector>
#include <exception>
#include <unordered_map>
#include "connections.hpp"

namespace messages {
	class DeserializingError : public std::exception {
		const char * what () const throw () {
			return "Deserializing error.";
		}
	};

	enum struct Direction : uint8_t {
		Up = 0, Right = 1, Down = 2, Left = 3
	};

	struct Position {
		uint16_t x, y;

		Position() = default;
		Position(uint16_t, uint16_t);
		Position(ServerConnection&);
		void serialize(Buffer&) const;
	};

	struct Player {
		using PlayerId = uint8_t;
		using Score = uint32_t;
		std::string name, address;

		Player() = default;
		Player(std::string, std::string);
		Player(ServerConnection&);
		void serialize(Buffer&) const;
	};

	struct Bomb {
		using BombId = uint32_t;
		Position position;
		uint16_t timer;

		Bomb() = default;
		Bomb(Position, uint16_t);
		Bomb(ServerConnection&);
		void serialize(Buffer&) const;
	};

	enum struct ClientToServerType : uint8_t {
		Join = 0, PlaceBomb = 1, PlaceBlock = 2, Move = 3
	};

	struct ClientToServer {
		ClientToServerType type;
		std::string name;
		Direction direction;

		ClientToServer() = default;
		void serialize(Buffer&) const;
	};

	enum struct EventType : uint8_t {
		BombPlaced = 0, BombExploded = 1, PlayerMoved = 2, BlockPlaced = 3
	};

	struct Event {
		EventType type;
		Bomb::BombId bomb_id;
		Player::PlayerId player_id;
		Position position;

		Event(ServerConnection&);
		void serialize(Buffer&) const;
	};

	enum struct ServerToClientType : uint8_t {
		Hello = 0, AcceptedPlayer = 1, GameStarted = 2, Turn = 3, GameEnded = 4
	};

	struct ServerToClient {
		ServerToClientType type;
		std::string server_name;
		uint8_t player_count;
		uint16_t size_x, size_y;
		uint16_t game_length;
		uint16_t explosion_radius, bomb_timer;
		Player::PlayerId player_id;
		Player player;
		std::unordered_map<Player::PlayerId, Player> players;
		uint16_t turn;
		std::vector<Event> events;
	
		ServerToClient(ServerConnection&);
		void serialize(Buffer&) const;
	};

	enum struct GUIToClientType : uint8_t {
		PlaceBomb = 0, PlaceBlock = 1, Move = 2
	};

	struct GUIToClient {
		GUIToClientType type;
		Direction direction;

		GUIToClient(GUIConnection&);
		void serialize(Buffer&) const;
	};

	enum struct ClientToGUIType : uint8_t {
		Lobby = 0, Game = 1
	};

	struct ClientToGUI {
		ClientToGUIType type;
		std::string server_name;
		uint8_t player_count;
		uint16_t size_x, size_y;
		uint16_t game_length;
		uint16_t explosion_radius, bomb_timer;
		std::unordered_map<Player::PlayerId, Player> players;
		std::unordered_map<Player::PlayerId, Position> player_positions;
		std::vector<Position> blocks;
		std::vector<Bomb> bombs;
		std::vector<Position> explosions;
		std::unordered_map<Player::PlayerId, Player::Score> scores;

		ClientToGUI() = default;
		void serialize(Buffer&) const;
	};
}

#endif // MESSAGES_HPP