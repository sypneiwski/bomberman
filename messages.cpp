#include <variant>
#include <string>
#include <vector>
#include <endian.h>
#include <iostream> // debug
#include "messages.hpp"

namespace messages {
	Position::Position(const char* buffer, size_t size) {
		if (size < 4)
			throw DeserializingError();
		x = be16toh(*(uint16_t*) buffer);
		y = be16toh(*(uint16_t*) (buffer + 2));
		std::cout << "x: " << x << " y: " << y << "\n";
	}

	BombPlacedMessage::BombPlacedMessage(const char* buffer, size_t size, size_t* event_size) {
		if (size < 8)
			throw DeserializingError();
		id = Bomb::BombId(be32toh(*(uint32_t*) buffer));
		std::cout << "BombId: " << id << "\n";
		position = Position(buffer + 4, size - 4);
		*event_size += 8;
	}

	PlayerMovedMessage::PlayerMovedMessage(const char* buffer, size_t size, size_t* event_size) {
		if (size < 5)
			throw DeserializingError();
		id = Player::PlayerId(*(uint8_t*) buffer);
		std::cout << "PlayerId: " << (int)id << "\n";
		position = Position(buffer + 1, size - 1);
		*event_size += 5;
	}

	Event::Event(const char* buffer, size_t size, size_t* event_size) : event_variant(EmptyMessage()) {
		if (size < 2)
			throw DeserializingError();
		type = EventType(*buffer);
		*event_size += 1;
		switch (type) {
			case EventType::BombPlaced:
				std::cout << "BombPlaced:\n";
				event_variant = BombPlacedMessage(buffer + 1, size - 1, event_size);
				break;
			case EventType::PlayerMoved:
				std::cout << "PlayerMoved:\n";
				event_variant = PlayerMovedMessage(buffer + 1, size - 1, event_size);
				break;
			default:
				break;
		}
	}

	TurnMessage::TurnMessage(const char* buffer, size_t size) {
		if (size < 2)
			throw DeserializingError();
		turn = be16toh(*(uint16_t*) buffer);
		std::cout << "turn : " << turn << "\n";
		uint32_t len = be32toh(*(uint32_t*) (buffer + 2));
		size_t read_bytes = 6, event_size;
		for(uint32_t i = 0; i < len; i++) {
			if (read_bytes >= size)
				throw DeserializingError();
			event_size = 0;
			Event event = Event(buffer + read_bytes, size - read_bytes, &event_size);
			events.push_back(event);
			read_bytes += event_size;
		}
	}

	ServerMessage::ServerMessage(const char* buffer, size_t size) {
		if (size < 2)
			throw DeserializingError();
		type = ServerMessageType(*buffer);
		switch (type) {
			case ServerMessageType::Turn:
				message_variant = TurnMessage(buffer + 1, size - 1);
				break;
			default:
				break;
		}
	}
}