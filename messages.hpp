#ifndef MESSAGES_HPP
#define MESSAGES_HPP
#include <variant>
#include <string>
#include <vector>
#include <exception>
#include <unordered_map>
#include "connections.hpp"
#include "buffer_wrapper.hpp"

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

		Position() : x(0), y(0) {};
		Position(uint16_t, uint16_t);
		Position(ServerConnection*);
		void serialize(buffers::Buffer&) const;
	};

	struct Player {
		using PlayerId = uint8_t;
		std::string name, address;

		Player(std::string, std::string);
		Player(ServerConnection*);
		void serialize(buffers::Buffer&) const;
	};

	struct Bomb {
		using BombId = uint32_t;
		Position position;
		uint16_t timer;

		Bomb(Position, uint16_t);
		Bomb(ServerConnection*);
		void serialize(buffers::Buffer&) const;
	};

	// is this needed?
	struct EmptyMessage {
		EmptyMessage() = default;
	};

	enum struct ClientToServerType : uint8_t {
		Join = 0, PlaceBomb = 1, PlaceBlock = 2, Move = 3
	};

	struct JoinMessage {
		std::string name;
	
		JoinMessage(std::string);
		JoinMessage(ServerConnection*);
		void serialize(buffers::Buffer&) const;
	};

	struct PlaceBombMessage {
		PlaceBombMessage() = default;
		void serialize(buffers::Buffer&) const;
	};

	struct PlaceBlockMessage {
		PlaceBlockMessage() = default;
		void serialize(buffers::Buffer&) const;
	};

	struct MoveMessage {
		Direction direction;
	
		MoveMessage(std::string);
		MoveMessage(ServerConnection*);
		void serialize(buffers::Buffer&) const;
	};

	struct ClientToServer {
		using ClientToServerVariant = std::variant<JoinMessage,
												  PlaceBombMessage,
												  PlaceBlockMessage,
												  MoveMessage>;
		ClientToServerType type;
		ClientToServerVariant data;
	
		ClientToServer(ClientToServerType, ClientToServerVariant);
		ClientToServer(ServerConnection*);
		void serialize(buffers::Buffer&) const;
	};

	enum struct EventType : uint8_t {
		BombPlaced = 0, BombExploded = 1, PlayerMoved = 2, BlockPlaced = 3
	};

	struct BombPlacedMessage {
		Bomb::BombId id;
		Position position;
	
		BombPlacedMessage(Bomb::BombId, Position);
		BombPlacedMessage(ServerConnection*);
		void serialize(buffers::Buffer&) const;
	};

	struct BombExplodedMessage {};

	struct PlayerMovedMessage {
		Player::PlayerId id;
		Position position;
	
		PlayerMovedMessage(Player::PlayerId, Position);
		PlayerMovedMessage(ServerConnection*);
		void serialize(buffers::Buffer&) const;
	};

	struct BlockPlacedMessage {};

	struct Event {
		using EventVariant = std::variant<BombPlacedMessage,
										  BombExplodedMessage,
										  PlayerMovedMessage,
										  BlockPlacedMessage,
										  EmptyMessage>; //needed?
		EventType type;
		EventVariant data;
	
		Event(EventType, EventVariant);
		Event(ServerConnection*);
		void serialize(buffers::Buffer&) const;
	};

	enum struct ServerToClientType : uint8_t {
		Hello = 0, AcceptedPlayer = 1, GameStarted = 2, Turn = 3, GameEnded = 4
	};

	struct HelloMessage {
		std::string server_name;
		uint8_t players_count;
		uint16_t size_x, size_y;
		uint16_t game_length;
		uint16_t explosion_radius, bomb_timer;
	
		HelloMessage(ServerConnection*);
	};

	struct AcceptedPlayerMessage {};

	struct GameStartedMessage {};

	struct TurnMessage {
		uint16_t turn;
		std::vector<Event> events;
	
		TurnMessage(uint16_t, std::vector<Event>);
		TurnMessage(ServerConnection*);
		void serialize(buffers::Buffer&) const;
	};

	struct GameEndedMessage {};

	struct ServerToClient {
		using ServerToClientVariant = std::variant<HelloMessage,
												  AcceptedPlayerMessage,
												  GameStartedMessage,
												  TurnMessage,
												  GameEndedMessage,
												  EmptyMessage>;
		ServerToClientType type;
		ServerToClientVariant data;
	
		ServerToClient(ServerToClientType, ServerToClientVariant);
		ServerToClient(ServerConnection*);
		void serialize(buffers::Buffer&) const;
	};

	enum struct GUIToClientType : uint8_t {
		PlaceBomb = 0, PlaceBlock = 1, Move = 2
	};

	struct GUIToClient {
		using GUIToClientVariant = std::variant<PlaceBombMessage,
												PlaceBlockMessage,
												MoveMessage>;
		GUIToClientType type;
		GUIToClientVariant data;

		GUIToClient(GUIToClientType, GUIToClientVariant);
		GUIToClient(ServerConnection*);
		void serialize(buffers::Buffer&) const; // Buffer
	};

	enum struct ClientToGUIType : uint8_t {
		Lobby = 0, Game = 1
	};

	struct LobbyMessage {
		std::string server_name;
		uint8_t players_count;
		uint16_t size_x, size_y;
		uint16_t game_length;
		uint16_t explosion_radius, bomb_timer;
		std::unordered_map<Player::PlayerId, Player> players;

		LobbyMessage(std::string, uint8_t, uint16_t, uint16_t,
					 uint16_t, uint16_t, uint16_t, std::unordered_map<Player::PlayerId, Player>);
		void serialize(buffers::Buffer&) const;
	};

	struct GameMessage {};

	struct ClientToGUI {
		using ClientToGUIVariant = std::variant<LobbyMessage,
												GameMessage>;
		ClientToGUIType type;
		ClientToGUIVariant data;

		ClientToGUI(ClientToGUIType, ClientToGUIVariant);
		void serialize(buffers::Buffer&) const;
	};
}

#endif // MESSAGES_HPP