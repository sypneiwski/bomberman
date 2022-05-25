#include <string>
#include <vector>
#include <endian.h>
#include <iostream> // debug
#include "messages.hpp"
#include "connections.hpp"

namespace messages {
	// for now it goes here
	template<typename T>
	void read_list(std::vector<T> &vec, ServerConnection &conn) {
		uint32_t len;
		conn.read32(len);
		for (uint32_t i = 0; i < len; i++) {
			T elem = T(conn);
			vec.push_back(elem);
		}
	}


	Position::Position(ServerConnection &conn) {
		conn.read16(x)
			.read16(y);
	}

	Player::Player(ServerConnection &conn) {
		conn.read_string(name)
			.read_string(address);
	}

	void Player::serialize(Buffer& buff) const {
		buff.write_string(name)
			.write_string(address);
	}

	void ClientToServer::serialize(Buffer& buff) const {
		buff.write8(static_cast<uint8_t>(type));
		switch (type) {
			case ClientToServerType::Join:
				buff.write_string(name);
				break;
			case ClientToServerType::Move:
				buff.write8(static_cast<uint8_t>(direction));
				break;
			default:
				break;		
		}
	}

	Event::Event(ServerConnection &conn) {
		uint8_t u8;
		conn.read8(u8);
		type = EventType(u8);
		switch (type) {
			case EventType::BombPlaced:
				conn.read32(bomb_id);
				position = Position(conn);
				break;
			case EventType::PlayerMoved:
				conn.read8(player_id);
				position = Position(conn);
				break;
			default:
				throw DeserializingError();
		}
	}

	ServerToClient::ServerToClient(ServerConnection &conn) {
		uint8_t u8;
		conn.read8(u8);
		type = ServerToClientType(u8);
		switch (type) {
			case ServerToClientType::Hello:
				conn.read_string(server_name)
					.read8(player_count)
					.read16(size_x)
					.read16(size_y)
					.read16(game_length)
					.read16(explosion_radius)
					.read16(bomb_timer);
				break;
			case ServerToClientType::AcceptedPlayer:
				conn.read8(player_id);
				player = Player(conn);
				break;
			case ServerToClientType::GameStarted:
				uint32_t len;
				conn.read32(len);
				for (uint32_t i = 0; i < len; i++) {
					conn.read8(player_id);
					player = Player(conn);
					players[player_id] = player;
				}
				break;
			case ServerToClientType::Turn:
				conn.read16(turn);
				read_list(events, conn);
				break;
			default:
				throw DeserializingError();
		}
	}

	GUIToClient::GUIToClient(GUIConnection &conn) {
		uint8_t u8;
		conn.read8(u8);
		type = GUIToClientType(u8);
		switch (type) {
			case GUIToClientType::Move:
				conn.read8(u8);
				direction = Direction(u8);
				break;
			default:
				break;
		}
	}

	void ClientToGUI::serialize(Buffer &buff) const {
		switch (type) {
			case ClientToGUIType::Lobby:
				buff.write8(static_cast<uint8_t>(type))
					.write_string(server_name)
					.write8(player_count)
					.write16(size_x)
					.write16(size_y)
					.write16(game_length)
					.write16(explosion_radius)
					.write16(bomb_timer)
					.write32(players.size());
				for (auto &[key, value] : players) {
					buff.write8(static_cast<uint8_t>(key));
					value.serialize(buff);
				}
				break;
			default:
				return; // Game
		}
	}
}