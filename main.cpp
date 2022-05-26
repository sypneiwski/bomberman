#include <iostream>
#include <string>
#include <exception>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <optional>
#include "program_options.hpp"
#include "messages.hpp"
#include "connections.hpp"

namespace {
	class Client {
	private:
		enum struct GameState {
			Lobby = 0, Game = 1
		};

		GameState game_state{GameState::Lobby};
		boost::asio::io_context io_context{};
		std::string player_name;
		ServerConnection serv_conn;
		GUIConnection gui_conn;

	public:
		Client(options::Options &options)
		: player_name(options.player_name),
		  serv_conn(io_context, options), 
		  gui_conn(io_context, options) {}

		std::optional<std::reference_wrapper<messages::ClientToGUI>> process_server_message() {
			using namespace messages; // można tak?
			static ClientToGUI out;
			ServerToClient in(serv_conn);

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
					out.player_positions.clear();
					out.blocks.clear();
					out.bombs.clear();
					break;
				case ServerToClientType::Turn:
					out.explosions.clear();
					for (auto &[id, bomb] : out.bombs)
						bomb.timer--;

					// Process this turn's events.
					for (const Event &event : in.events) {
						switch (event.type) {
							case EventType::BombPlaced:
								out.bombs[event.bomb_id] = Bomb(event.position, out.bomb_timer);
								break;
							case EventType::BombExploded:
								// add all 
								for (const Position& position : event.destroyed_blocks)
									
								out.bombs.erase(event.bomb_id);
								break;
							case EventType::PlayerMoved:
								out.player_positions[event.player_id] = event.position;	
								break;
							case EventType::BlockPlaced:
								out.blocks.insert(event.position);		
						}
					}

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

		messages::ClientToServer& process_gui_message() {
			using namespace messages;
			static ClientToServer out;
			GUIToClient in(gui_conn);

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

		void send_gui_message(messages::ClientToGUI &message) {
			static Buffer serialized;
			message.serialize(serialized);
			gui_conn.write(serialized);
			serialized.clear();
		}

		void send_server_message(messages::ClientToServer &message) {
			static Buffer serialized;
			message.serialize(serialized);
			serv_conn.write(serialized);
			serialized.clear();
		}
	};

	void listen_for_server(Client &client) {
		for (;;) {
			auto out = client.process_server_message();
			if (out)
				client.send_gui_message(out->get());
		}
	}

	void listen_for_gui(Client &client) {
		for (;;) {
			try {
				messages::ClientToServer out = client.process_gui_message();
				client.send_server_message(out);
			}
			catch (messages::NonTerminatingError &e) {
				continue;
			}
		}
	}
}

int main(int argc, char *argv[]) {
	try {
		options::Options options = options::Options(argc, argv);
		Client client(options);
		boost::thread t1(boost::bind(&listen_for_server, std::ref(client)));
		boost::thread t2(boost::bind(&listen_for_gui, std::ref(client)));
		t1.join();
		t2.join();
	}
	catch (messages::DeserializingError &e) {
		std::cerr << "ERROR : " << e.what() << "\n";
		exit(EXIT_SUCCESS);
	}
	catch (std::exception &e) {
		std::cerr << "ERROR : " << e.what() << "\n";
		exit(EXIT_FAILURE);
	}
	return 0;
}