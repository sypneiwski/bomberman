#ifndef MESSAGES_H
#define MESSAGES_H
#include <variant>
#include <string>
#include <vector>
#include <exception>

namespace messages {
	class DeserializingError : public std::exception {};

	enum struct Direction : uint8_t {
		Up = 0, Right = 1, Down = 2, Left = 3
	};

	class Position {
	private:
		uint16_t x, y;
	public:
		Position(uint16_t, uint16_t);
		Position(const char*, size_t);
		uint32_t encode(char*) const;
	};

	class Player {
	private:
		std::string name, address;
	public:
		using PlayerId = uint8_t;
		Player(std::string, std::string);
		Player(const char*, size_t);
		uint32_t encode(char*) const;
	};

	class Bomb {
	private:
		Position position;
		uint16_t timer;
	public:
		using BombId = uint32_t;
		Bomb(Position, uint16_t);
		Bomb(const char*, size_t);
		uint32_t encode(char*) const;
	};

	enum struct ClientMessageType : uint8_t {
		Join = 0, PlaceBomb = 1, PlaceBlock = 2, Move = 3
	};

	class JoinMessage {
	private:
		std::string name;
	public:
		JoinMessage(std::string);
		JoinMessage(const char*, size_t);
		uint32_t encode(char*) const;
	};

	class PlaceBombMessage {
	public:
		PlaceBombMessage() = default;
		uint32_t encode(char*) const;
	};

	class PlaceBlockMessage {
	public:
		PlaceBlockMessage() = default;
		uint32_t encode(char*) const;
	};

	class MoveMessage {
	private:
		Direction direction;
	public:
		MoveMessage(std::string);
		MoveMessage(const char*, size_t);
		uint32_t encode(char*) const;
	};

	class ClientMessage {
	private:
		using ClientMessageVariant = std::variant<JoinMessage,
												  PlaceBombMessage,
												  PlaceBlockMessage,
												  MoveMessage>;
		ClientMessageType type;
		ClientMessageVariant message_variant;
	public:
		ClientMessage(ClientMessageType, ClientMessageVariant);
		ClientMessage(const char*, size_t);
		uint32_t encode(char *) const;
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
		BombPlacedMessage(const char*, size_t, size_t*);
		uint32_t encode(char*) const;
	};

	class BombExplodedMessage {};

	class PlayerMovedMessage {
	private:
		Player::PlayerId id;
		Position position;
	public:
		PlayerMovedMessage(Player::PlayerId, Position);
		PlayerMovedMessage(const char*, size_t, size_t*);
		uint32_t encode(char*) const;
	};

	class BlockPlacedMessage {};

	class Event {
	private:
		using EventVariant = std::variant<BombPlacedMessage,
										  BombExplodedMessage,
										  PlayerMovedMessage,
										  BlockPlacedMessage>;
		EventType type;
		EventVariant event_variant;
	public:
		Event(EventType, EventVariant);
		Event(const char*, size_t, size_t*);
		uint32_t encode(char *) const;
	};

	enum struct ServerMessageType : uint8_t {
		Hello = 0, AcceptedPlayer = 1, GameStarted = 2, Turn = 3, GameEnded = 4
	};

	class HelloMessage {};

	class AcceptedPlayerMessage {};

	class GameStartedMessage {};

	class TurnMessage {
	private:
		uint16_t turn;
		std::vector<Event> events;
	public:
		TurnMessage(uint16_t, std::vector<Event>);
		TurnMessage(const char*, size_t);
		uint32_t encode(char*) const;
	};

	class GameEndedMessage {};

	class ServerMessage {
	private:
		using ServerMessageVariant = std::variant<HelloMessage,
												  AcceptedPlayerMessage,
												  GameStartedMessage,
												  TurnMessage,
												  GameEndedMessage>;
		ServerMessageType type;
		ServerMessageVariant message_variant;
	public:
		ServerMessage(ServerMessageType, ServerMessageVariant);
		ServerMessage(const char*, size_t);
		uint32_t encode(char*) const;
	};
}

#endif // MESSAGES_H