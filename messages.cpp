#include <string>
#include <vector>
#include <endian.h>
#include <iostream> // debug
#include "messages.hpp"
#include "connections.hpp"

namespace messages {
	bool Position::operator<(const Position &other) const {
		if (x == other.x)
			return y < other.y;
		return x < other.x;	
	}

	Position::Position(uint16_t x, uint16_t y) : x(x), y(y) {}

	Position::Position(ServerConnection &conn) {
		conn.read16(x)
			.read16(y);
	}

	Player::Player(std::string name, std::string address)
	: name(name), address(address) {}

	Player::Player(ServerConnection &conn) {
		conn.read_string(name)
			.read_string(address);
	}

	void Player::serialize(Buffer& buff) const {
		buff.write_string(name)
			.write_string(address);
	}

	Bomb::Bomb(Position position, uint16_t timer) 
	: position(position), timer(timer) {}

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
		if (u8 > static_cast<uint8_t>(EventType::MAX))
			throw DeserializingError();
		type = EventType(u8);
		switch (type) {
			case EventType::BombPlaced:
				conn.read32(bomb_id);
				position = Position(conn);
				break;
			case EventType::BombExploded:
				uint32_t len;
				conn.read32(len);
				for (uint32_t i = 0; i < len; i++) {
					conn.read8(player_id);
					destroyed_players.push_back(player_id);
				}
				conn.read32(len);
				for (uint32_t i = 0; i < len; i++) {
					position = Position(conn);
					destroyed_blocks.push_back(position);
				}
				break;
			case EventType::PlayerMoved:
				conn.read8(player_id);
				position = Position(conn);
				break;
			case EventType::BlockPlaced:
				position = Position(conn);
				break;
		}
	}

	ServerToClient::ServerToClient(ServerConnection &conn) {
		uint8_t u8;
		conn.read8(u8);
		if (u8 > static_cast<uint8_t>(ServerToClientType::MAX))
			throw DeserializingError();
		type = ServerToClientType(u8);
		uint32_t len;	
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
				conn.read32(len);
				for (uint32_t i = 0; i < len; i++) {
					conn.read8(player_id);
					player = Player(conn);
					players[player_id] = player;
				}
				break;
			case ServerToClientType::Turn:
				conn.read16(turn)
					.read32(len);
				for (uint32_t i = 0; i < len; i++) {
					Event event(conn);
					events.push_back(event);
				}	
				break;
			case ServerToClientType::GameEnded:
				conn.read32(len);
				for (uint32_t i = 0; i < len; i++) {
					Player::Score score;
					conn.read8(player_id)
						.read32(score);
					scores[player_id] = score;	
				}
				break;
		}
	}

	GUIToClient::GUIToClient(GUIConnection &conn) {
		uint8_t u8;
		conn.read8(u8);
		if (u8 > static_cast<uint8_t>(GUIToClientType::MAX))
			throw NonTerminatingError();
		type = GUIToClientType(u8);
		switch (type) {
			case GUIToClientType::Move:
				conn.read8(u8);
				if (u8 > static_cast<uint8_t>(Direction::MAX))
					throw DeserializingError();
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
				for (const auto &[id, player] : players) {
					buff.write8(static_cast<uint8_t>(id));
					player.serialize(buff);
				}
				break;
			default:
				return; // Game
		}
	}
}