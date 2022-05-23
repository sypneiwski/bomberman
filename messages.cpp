#include <variant>
#include <string>
#include <vector>
#include <endian.h>
#include <iostream> // debug
#include "messages.hpp"
#include "buffer_wrapper.hpp"

namespace messages {
	template<typename T>
	void read_list(std::vector<T> &vec, buffers::Buffer &buffer) {
		try {
			uint32_t len = buffer.read32();
			for (uint32_t i = 0; i < len; i++) {
				T elem = T(buffer);
				vec.push_back(elem);
			}
		}
		catch (std::exception &e) {
			throw DeserializingError();
		}
	}

	Position::Position(buffers::Buffer &buffer) {
		try {
			x = buffer.read16();
			y = buffer.read16();
			std::cout << "x: " << x << " y: " << y << "\n";
		}
		catch (std::exception &e) {
			throw DeserializingError();
		}
	}

	MoveMessage::MoveMessage(buffers::Buffer &buffer) {
		try {
			direction = Direction(buffer.read8());
		}
		catch (std::exception &e) {
			throw DeserializingError();
		}
	}

	BombPlacedMessage::BombPlacedMessage(buffers::Buffer &buffer) {
		try {
			id = Bomb::BombId(buffer.read32());
			std::cout << "BombId: " << id << "\n";
			position = Position(buffer);
		}
		catch (std::exception &e) {
			throw DeserializingError();
		}
	}

	PlayerMovedMessage::PlayerMovedMessage(buffers::Buffer &buffer) {
		try {
			id = Player::PlayerId(buffer.read8());
			std::cout << "PlayerId: " << (int) id << "\n";
			position = Position(buffer);
		}
		catch (std::exception &e) {
			throw DeserializingError();
		}
	}

	Event::Event(buffers::Buffer &buffer) : event_variant(EmptyMessage()) {
		try {
			type = EventType(buffer.read8());
			switch (type) {
				case EventType::BombPlaced:
					std::cout << "BombPlaced:\n";
					event_variant = BombPlacedMessage(buffer);
					break;
				case EventType::PlayerMoved:
					std::cout << "PlayerMoved:\n";
					event_variant = PlayerMovedMessage(buffer);
					break;
				default:
					throw DeserializingError();
			}
		}
		catch (std::exception &e) {
			throw DeserializingError();
		}
	}

	HelloMessage::HelloMessage(buffers::Buffer &buffer) {
		try {
			uint8_t len = buffer.read8();
			buffer.read_string(&server_name, len);
			players_count = buffer.read8();
			size_x = buffer.read16();
			size_y = buffer.read16();
			game_length = buffer.read16();
			explosion_radius = buffer.read16();
			bomb_timer = buffer.read16();

			std::cout << server_name << "\n"
			          << (int)players_count << "\n"
			          << size_x << " " << size_y << "\n"
			          << game_length << "\n"
			          << explosion_radius << " " << bomb_timer << "\n";
		}
		catch (std::exception &e) {
			throw DeserializingError();
		}
	}

	TurnMessage::TurnMessage(buffers::Buffer &buffer) {
		try {
			turn = buffer.read16();
			std::cout << "turn : " << turn << "\n";
			read_list(events, buffer);
		}
		catch (std::exception &e) {
			throw DeserializingError();
		}
	}

	ServerToClient::ServerToClient(buffers::Buffer &buffer) : message_variant(EmptyMessage()){
		try {
			type = ServerToClientType(buffer.read8());
			switch (type) {
				case ServerToClientType::Hello:
					message_variant = HelloMessage(buffer);
					break;
				case ServerToClientType::Turn:
					message_variant = TurnMessage(buffer);
					break;
				default:
					throw DeserializingError();
			}
		}
		catch (std::exception &e) {
			throw DeserializingError();
		}
	}

	GUIToClient::GUIToClient(buffers::Buffer &buffer) {
		try {
			type = GUIToClientType(buffer.read8());
			switch (type) {
				case GUIToClientType::PlaceBomb:
					message_variant = PlaceBombMessage();
					break;
				case GUIToClientType::PlaceBlock:
					message_variant = PlaceBlockMessage();
					break;
				case GUIToClientType::Move:
					message_variant = MoveMessage(buffer);
					break;
				default:
					throw DeserializingError();
			}
		}
		catch (std::exception &e) {
			throw DeserializingError();
		}
	}
}