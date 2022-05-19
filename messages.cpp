#include <variant>
#include <string>
#include <vector>
#include <endian.h>
#include "messages.hpp"

namespace messages {
	Position::Position(const char* buffer, size_t size) {
		if (size < 4)
			throw DeserializingError();
		x = be16toh(buffer);
		y = be16toh(buffer + 2);
	}

	BombPlacedMessage::BombPlacedMessage(const char* buffer, size_t size, size_t* event_size) {
		if (size < 8)
			throw DeserializingError();
		id = Bomb::BombId(be32toh(buffer));
		position = Position(buffer + 4, size - 4);
		*event_size += 8;
	}

	PlayerMovedMessage::PlayerMovedMessage(const char* buffer, size_t size, size_t* event_size) {
		if (size < 5)
			throw DeserializingError();
		id = Player::PlayerId(buffer);
		position = Position(buffer + 1, size - 1);
		*event_size += 3;
	}

	Event::Event(const char* buffer, size_t size, size_t* event_size) {
		if (size < 2)
			throw DeserializingError();
		type = EventType(buffer);
		*event_size += 1;
		switch (type) {
			case EventType::BombPlaced:
				event_variant = BombPlacedMessage(buffer + 1, size - 1, event_size);
				break;
			case EventType::PlayerMoved:
				event_variant = PlayerMovedMessage(buffer + 1, size - 1, event_size);
				break;
			default:
				break;
		}
	}

	TurnMessage::TurnMessage(const char* buffer, size_t size) {
		if (size < 2)
			throw DeserializingError();
		turn = be15toh(buffer);
		size_t read_bytes = 2, event_size;
		while (read_bytes < size) {
			Event event = Event(buffer + read_bytes, size - read_bytes, &event_size);
			events.push_back(event);
			read_bytes += event_size;
		}
	}

	ServerMessage::ServerMessage(const char* buffer, size_t size) {
		if (size < 2)
			throw DeserializingError();
		type = ServerMessageType(buffer);
		switch (type) {
			case ServerMessageType::Turn:
				message_variant = TurnMessage(buffer + 1, size - 1);
				break;
			default:
				break;
		}
	}
}