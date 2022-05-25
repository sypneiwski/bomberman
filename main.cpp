#include <iostream>
#include <string>
#include <exception>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "program_options.hpp"
#include "messages.hpp"
#include "connections.hpp"

namespace {
	enum struct GameState {
		Lobby = 0, Game = 1
	};

	class Client {
	private:
		GameState game_state{GameState::Lobby};
		boost::asio::io_context io_context{};
		ServerConnection serv_conn;
		GUIConnection gui_conn;
	public:
		Client(options::Options &options)
		: serv_conn(io_context, options), gui_conn(io_context, options) {}

		messages::ClientToGUI& process_server_message() {
			using namespace messages; // można tak?
			static ClientToGUI out_message;
			ServerToClient in_message(serv_conn);

			switch (in_message.type) {
				case ServerToClientType::Hello:
					out_message.server_name = in_message.server_name;
					out_message.player_count = in_message.player_count;
					out_message.size_x = in_message.size_x;
					out_message.size_y = in_message.size_y;
					out_message.game_length = in_message.game_length;
					out_message.explosion_radius = in_message.explosion_radius;
					out_message.bomb_timer = in_message.bomb_timer;
					break;
				case ServerToClientType::AcceptedPlayer:
					out_message.players[in_message.player_id] = in_message.player;
					break;
				default:
					throw std::invalid_argument("tu powinno byc cos innego");
			}
			out_message.type = static_cast<ClientToGUIType>(game_state);
			return out_message;
		}

		messages::ClientToServer& process_gui_message() {
			using namespace messages;
			static ClientToServer out_message;
			GUIToClient in_message(gui_conn);

			// somehow send join
			switch (in_message.type) {
				case GUIToClientType::PlaceBomb:
					out_message.type = ClientToServerType::PlaceBomb;
					break;
				case GUIToClientType::PlaceBlock:
					out_message.type = ClientToServerType::PlaceBlock;
					break;
				case GUIToClientType::Move:
					out_message.type = ClientToServerType::Move;
					out_message.direction = in_message.direction;
					break;
			}

			return out_message;
		}

		void send_gui_message(messages::ClientToGUI &message) {
			static Buffer serialized;
			message.serialize(serialized);
			gui_conn.write(serialized);
			serialized.clear();
		}
	};

	void listen_for_server(Client &client) {
		try {
			for (;;) {
				messages::ClientToGUI out_message = client.process_server_message();
				client.send_gui_message(out_message);
			}
		}
		catch (std::exception &e) {
			std::cout << "ERROR : " << e.what() << "\n";
		}
	}

	void listen_for_gui(Client &client) {
		try {
			for (;;) {
				messages::ClientToServer out_message = client.process_gui_message();
			}
		}
		catch (std::exception &e) {
			std::cout << "ERROR : " << e.what() << "\n";
		}
	}
}

int main(int argc, char *argv[]) {
	options::Options options = options::Options(argc, argv);
	Client client(options);
	//boost::thread t1(boost::bind(&listen_for_server, std::ref(client)));
	boost::thread t2(boost::bind(&listen_for_gui, std::ref(client)));
	//t1.join();
	t2.join();
	return 0;
}