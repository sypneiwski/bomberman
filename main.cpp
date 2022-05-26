#include <iostream>
#include <string>
#include <exception>
#include <boost/asio.hpp>
#include <optional>
#include <set>
#include <utility>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <cassert>
#include "program_options.hpp"
#include "messages.hpp"
#include "connections.hpp"

namespace {
	static std::mutex end_mutex;
	static std::condition_variable end_condition;
	static bool end{false};

	/*
	Client class, responsible for establishing connection with the server
	and GUI, receiving and sending messages and keeping track of thestate
	of the game. 
	*/
	class Client {
	private:
		enum struct GameState {
			Lobby = 0, Game = 1
		};

		std::mutex state_mutex;
		GameState game_state{GameState::Lobby};
		boost::asio::io_context io_context{};
		std::string player_name;
		ServerConnection serv_conn;
		GUIConnection gui_conn;

		void count_explosions(const Event &event, ClientToGUI& out) {
			assert(event.type == EventType::BombExploded);
			static std::vector<std::pair<int32_t,int32_t>> sides = {
				{1,0}, {0,1}, {-1, 0}, {0, -1}
			};
			for (const std::pair<int32_t, int32_t> &side : sides) {
				Position p = out.bombs[event.bomb_id].position;
				int32_t x = p.x, y = p.y;
				for (uint16_t i = 0; i <= out.explosion_radius; i++) {
					p = Position(x, y);
					out.explosions.insert(p);
					if (out.blocks.count(p) == 1) // change to contains
						break;
					x += side.first;
					y += side.second;
					if (x == -1 || x == out.size_x || y == -1 || y == out.size_y)
						break;	
				}
			}
		}
	public:
		Client(Options &options)
		: player_name(options.player_name),
		  serv_conn(io_context, options), 
		  gui_conn(io_context, options) {}

		ServerToClient receive_from_server() {
			ServerToClient in(serv_conn);
			return in;
		}  

		std::optional<std::reference_wrapper<ClientToGUI>> 
		process_server_message(const ServerToClient& in) {
			static ClientToGUI out;
			static std::set<Player::PlayerId> destroyed_players;
			static std::set<Position> destroyed_blocks;
			const std::lock_guard<std::mutex> lock(state_mutex);
			
			switch (in.type) {
				case ServerToClientType::Hello:
					out.server_name = in.server_name;
					out.player_count = in.player_count;
					out.size_x = in.size_x;
					out.size_y = in.size_y;
					out.game_length = in.game_length;
					out.explosion_radius = in.explosion_radius;
					out.bomb_timer = in.bomb_timer;
					break;
				case ServerToClientType::AcceptedPlayer:
					out.players[in.player_id] = in.player;
					out.scores[in.player_id] = 0;
					break;
				case ServerToClientType::GameStarted:
					game_state = GameState::Game;
					out.players = in.players;
					out.scores.clear();
					for (const auto &[id, player] : out.players)
						out.scores[id] = 0;
					out.player_positions.clear();
					out.blocks.clear();
					out.bombs.clear();
					break;
				case ServerToClientType::Turn:
					out.explosions.clear();
					destroyed_players.clear();
					destroyed_blocks.clear();
					out.turn = in.turn;
					for (auto &[id, bomb] : out.bombs)
						bomb.timer--;

					// Process this turn's events.
					for (const Event &event : in.events) {
						switch (event.type) {
							case EventType::BombPlaced:
								out.bombs[event.bomb_id] = Bomb(event.position, out.bomb_timer);
								break;
							case EventType::BombExploded:
								count_explosions(event, out);
								for (const Position &position : event.destroyed_blocks)
									destroyed_blocks.insert(position);
								for (const Player::PlayerId &id : event.destroyed_players)
									destroyed_players.insert(id);
								out.bombs.erase(event.bomb_id);
								break;
							case EventType::PlayerMoved:
								out.player_positions[event.player_id] = event.position;	
								break;
							case EventType::BlockPlaced:
								out.blocks.insert(event.position);		
						}
					}
					for (const Position &position : destroyed_blocks)
						out.blocks.erase(position);
					for (const Player::PlayerId &id : destroyed_players)
						out.scores[id]++;
					break;
				case ServerToClientType::GameEnded:
					game_state = GameState::Lobby;
					out.players.clear();
					out.scores.clear();
					break;
			}
			out.type = static_cast<ClientToGUIType>(game_state);
			if (in.type == ServerToClientType::GameStarted)
				return std::nullopt;
			return out;
		}

		GUIToClient receive_from_gui() {
			GUIToClient in(gui_conn);
			if (gui_conn.has_more())
				throw GUIReadError();
			return in;
		}

		ClientToServer& process_gui_message(const GUIToClient& in) {
			static ClientToServer out;
			const std::lock_guard<std::mutex> lock(state_mutex);

			if (game_state == GameState::Lobby) {
				out.type = ClientToServerType::Join;
				out.name = player_name;
			}
			else {
				switch (in.type) {
					case GUIToClientType::PlaceBomb:
						out.type = ClientToServerType::PlaceBomb;
						break;
					case GUIToClientType::PlaceBlock:
						out.type = ClientToServerType::PlaceBlock;
						break;
					case GUIToClientType::Move:
						out.type = ClientToServerType::Move;
						out.direction = in.direction;
						break;
				}
			}

			return out;
		}

		void send_gui_message(ClientToGUI &message) {
			static Buffer serialized;
			message.serialize(serialized);
			gui_conn.write(serialized);
			serialized.clear();
		}

		void send_server_message(ClientToServer &message) {
			static Buffer serialized;
			message.serialize(serialized);
			serv_conn.write(serialized);
			serialized.clear();
		}
	};

	void listen_for_server(Client &client) {
		for (;;) {
			try {
				ServerToClient in = client.receive_from_server();
				auto out = client.process_server_message(in);
				if (out)
					client.send_gui_message(out->get());
			}
			catch (std::exception &e) {
				end = true;
				end_condition.notify_all();
				std::cerr << "ERROR : " << e.what() << "\n";
				return;
			}
		}
	}

	void listen_for_gui(Client &client) {
		for (;;) {
			try {
				GUIToClient in = client.receive_from_gui();
				ClientToServer out = client.process_gui_message(in);
				client.send_server_message(out);
			}
			catch (GUIReadError &e) {
				continue;
			}
			catch (std::exception &e) {
				end = true;
				end_condition.notify_all();
				std::cerr << "ERROR : " << e.what() << "\n";
				return;
			}
		}
	}
} // anonymous namespace

int main(int argc, char *argv[]) {
	try {
		Options options = Options(argc, argv);
		Client client(options);
		std::thread server_listener(listen_for_server, std::ref(client));
		std::thread gui_listener(&listen_for_gui, std::ref(client));

		std::unique_lock lock(end_mutex);
		end_condition.wait(lock, []{return end;});
		std::cerr << "closing connection\n";
		exit(EXIT_FAILURE);
	}
	catch (std::exception &e) {
		std::cerr << "ERROR : " << e.what() << "\n";
		exit(EXIT_FAILURE);
	}
	return 0;
}