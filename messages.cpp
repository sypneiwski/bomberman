#include <variant>
#include <string>
#include <vector>
#include <endian.h>
#include <iostream> // debug
#include "messages.hpp"
#include "connections.hpp"
#include "buffer_wrapper.hpp"

namespace messages {
	template<typename T>
	void read_list(std::vector<T> &vec, ServerConnection *conn) {
		try {
			uint32_t len = conn->read32();
			for (uint32_t i = 0; i < len; i++) {
				T elem = T(conn);
				vec.push_back(elem);
			}
		}
		catch (std::exception &e) {
			throw DeserializingError();
		}
	}

	template<typename K, typename V>
	void serialize_map(std::unordered_map<K, V> &map, buffers::Buffer& buff) {
		uint32_t size = map.size();
		buff.write32(size);
		for (auto& [key, value]: map) {
			key.serialize(buff);
			value.serialize(buff);
		}
	}

	Position::Position(ServerConnection *conn) {
		try {
			x = conn->read16();
			y = conn->read16();
			std::cout << "x: " << x << " y: " << y << "\n";
		}
		catch (std::exception &e) {
			throw DeserializingError();
		}
	}

	MoveMessage::MoveMessage(ServerConnection *conn) {
		try {
			direction = Direction(conn->read8());
		}
		catch (std::exception &e) {
			throw DeserializingError();
		}
	}

	BombPlacedMessage::BombPlacedMessage(ServerConnection *conn) {
		try {
			id = Bomb::BombId(conn->read32());
			std::cout << "BombId: " << id << "\n";
			position = Position(conn);
		}
		catch (std::exception &e) {
			throw DeserializingError();
		}
	}

	PlayerMovedMessage::PlayerMovedMessage(ServerConnection *conn) {
		try {
			id = Player::PlayerId(conn->read8());
			std::cout << "PlayerId: " << (int) id << "\n";
			position = Position(conn);
		}
		catch (std::exception &e) {
			throw DeserializingError();
		}
	}

	Event::Event(ServerConnection *conn) : data(EmptyMessage()) {
		try {
			type = EventType(conn->read8());
			switch (type) {
				case EventType::BombPlaced:
					std::cout << "BombPlaced:\n";
					data = BombPlacedMessage(conn);
					break;
				case EventType::PlayerMoved:
					std::cout << "PlayerMoved:\n";
					data = PlayerMovedMessage(conn);
					break;
				default:
					throw DeserializingError();
			}
		}
		catch (std::exception &e) {
			throw DeserializingError();
		}
	}

	HelloMessage::HelloMessage(ServerConnection *conn) {
		try {
			uint8_t len = conn->read8();
			conn->read_string(&server_name, len);
			players_count = conn->read8();
			size_x = conn->read16();
			size_y = conn->read16();
			game_length = conn->read16();
			explosion_radius = conn->read16();
			bomb_timer = conn->read16();

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

	TurnMessage::TurnMessage(ServerConnection *conn) {
		try {
			turn = conn->read16();
			std::cout << "turn : " << turn << "\n";
			read_list(events, conn);
		}
		catch (std::exception &e) {
			throw DeserializingError();
		}
	}

	ServerToClient::ServerToClient(ServerConnection *conn) : data(EmptyMessage()){
		try {
			type = ServerToClientType(conn->read8());
			switch (type) {
				case ServerToClientType::Hello:
					data = HelloMessage(conn);
					break;
				case ServerToClientType::Turn:
					data = TurnMessage(conn);
					break;
				default:
					throw DeserializingError();
			}
		}
		catch (std::exception &e) {
			throw DeserializingError();
		}
	}

	GUIToClient::GUIToClient(ServerConnection *conn) {
		try {
			type = GUIToClientType(conn->read8());
			switch (type) {
				case GUIToClientType::PlaceBomb:
					data = PlaceBombMessage();
					break;
				case GUIToClientType::PlaceBlock:
					data = PlaceBlockMessage();
					break;
				case GUIToClientType::Move:
					data = MoveMessage(conn);
					break;
				default:
					throw DeserializingError();
			}
		}
		catch (std::exception &e) {
			throw DeserializingError();
		}
	}

	LobbyMessage::LobbyMessage(std::string server, uint8_t count, uint16_t x, uint16_t y,
							   uint16_t len, uint16_t ex, uint16_t timer, std::unordered_map<Player::PlayerId, Player> map)
	: server_name(server), players_count(count), size_x(x), size_y(y), game_length(len), explosion_radius(ex),
	bomb_timer(timer), players(map) {}

	void LobbyMessage::serialize(buffers::Buffer &buff) const {
		buff.write_string(server_name)
			.write8(players_count)
			.write16(size_x)
			.write16(size_y)
			.write16(game_length)
			.write16(explosion_radius)
			.write16(bomb_timer);
		serialize_map<Player::PlayerId, Player>(players, buff);
	}

	ClientToGUI::ClientToGUI(ClientToGUIType type, ClientToGUIVariant data)
	: type(type), data(data) {}

	void ClientToGUI::serialize(buffers::Buffer &buff) const {
		buff.write8(type);
		data.serialize(buff);
	}
}