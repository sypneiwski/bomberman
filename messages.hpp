#ifndef MESSAGES_HPP
#define MESSAGES_HPP
#include <variant>
#include <string>
#include <vector>
#include <exception>
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

	class Position {
	private:
		uint16_t x, y;
	public:
		Position() : x(0), y(0) {};
		Position(uint16_t, uint16_t);
		Position(buffers::Buffer&);
		uint32_t serialize(buffers::Buffer&) const;
	};

	class Player {
	private:
		std::string name, address;
	public:
		using PlayerId = uint8_t;
		Player(std::string, std::string);
		Player(buffers::Buffer&);
		uint32_t serialize(buffers::Buffer&) const;
	};

	class Bomb {
	private:
		Position position;
		uint16_t timer;
	public:
		using BombId = uint32_t;
		Bomb(Position, uint16_t);
		Bomb(buffers::Buffer&);
		uint32_t serialize(buffers::Buffer&) const;
	};

	// is this needed?
	class EmptyMessage {
	public:
		EmptyMessage() = default;
	};

	enum struct ClientToServerType : uint8_t {
		Join = 0, PlaceBomb = 1, PlaceBlock = 2, Move = 3
	};

	class JoinMessage {
	private:
		std::string name;
	public:
		JoinMessage(std::string);
		JoinMessage(buffers::Buffer&);
		uint32_t serialize(buffers::Buffer&) const;
	};

	class PlaceBombMessage {
	public:
		PlaceBombMessage() = default;
		uint32_t serialize(buffers::Buffer&) const;
	};

	class PlaceBlockMessage {
	public:
		PlaceBlockMessage() = default;
		uint32_t serialize(buffers::Buffer&) const;
	};

	class MoveMessage {
	private:
		Direction direction;
	public:
		MoveMessage(std::string);
		MoveMessage(buffers::Buffer&);
		uint32_t serialize(buffers::Buffer&) const;
	};

	class ClientToServer {
	private:
		using ClientToServerVariant = std::variant<JoinMessage,
												  PlaceBombMessage,
												  PlaceBlockMessage,
												  MoveMessage>;
		ClientToServerType type;
		ClientToServerVariant message_variant;
	public:
		ClientToServer(ClientToServerType, ClientToServerVariant);
		ClientToServer(buffers::Buffer&);
		uint32_t serialize(char *) const;
	};

	enum struct EventType : uint8_t {
		BombPlaced = 0, BombExploded = 1, PlayerMoved = 2, BlockPlaced = 3
	};

	class BombPlacedMessage {
	private:
		Bomb::BombId id;
		Position position;
	public:
		BombPlacedMessage(Bomb::BombId, Position);
		BombPlacedMessage(buffers::Buffer&);
		uint32_t serialize(buffers::Buffer&) const;
	};

	class BombExplodedMessage {};

	class PlayerMovedMessage {
	private:
		Player::PlayerId id;
		Position position;
	public:
		PlayerMovedMessage(Player::PlayerId, Position);
		PlayerMovedMessage(buffers::Buffer&);
		uint32_t serialize(buffers::Buffer&) const;
	};

	class BlockPlacedMessage {};

	class Event {
	private:
		using EventVariant = std::variant<BombPlacedMessage,
										  BombExplodedMessage,
										  PlayerMovedMessage,
										  BlockPlacedMessage,
										  EmptyMessage>; //needed?
		EventType type;
		EventVariant event_variant;
	public:
		Event(EventType, EventVariant);
		Event(buffers::Buffer&);
		uint32_t serialize(char *) const;
	};

	enum struct ServerToClientType : uint8_t {
		Hello = 0, AcceptedPlayer = 1, GameStarted = 2, Turn = 3, GameEnded = 4
	};

	class HelloMessage {
	private:
		std::string server_name;
		uint8_t players_count;
		uint16_t size_x, size_y;
		uint16_t game_length;
		uint16_t explosion_radius, bomb_timer;
	public:
		HelloMessage(buffers::Buffer&);
	};

	class AcceptedPlayerMessage {};

	class GameStartedMessage {};

	class TurnMessage {
	private:
		uint16_t turn;
		std::vector<Event> events;
	public:
		TurnMessage(uint16_t, std::vector<Event>);
		TurnMessage(buffers::Buffer&);
		uint32_t serialize(buffers::Buffer&) const;
	};

	class GameEndedMessage {};

	class ServerToClient {
	private:
		using ServerToClientVariant = std::variant<HelloMessage,
												  AcceptedPlayerMessage,
												  GameStartedMessage,
												  TurnMessage,
												  GameEndedMessage,
												  EmptyMessage>;
		ServerToClientType type;
		ServerToClientVariant message_variant;
	public:
		ServerToClient(ServerToClientType, ServerToClientVariant);
		ServerToClient(buffers::Buffer&);
		uint32_t serialize(buffers::Buffer&) const;
	};

	enum struct GUIToClientType : uint8_t {
		PlaceBomb = 0, PlaceBlock = 1, Move = 2
	};

	class GUIToClient {
	private:
		using GUIToClientVariant = std::variant<PlaceBombMessage,
												PlaceBlockMessage,
												MoveMessage>;
		GUIToClientType type;
		GUIToClientVariant message_variant;
	public:
		GUIToClient(GUIToClientType, GUIToClientVariant);
		GUIToClient(buffers::Buffer&);
		uint32_t serialize(buffers::Buffer&) const;
	};
}

#endif // MESSAGES_HPP