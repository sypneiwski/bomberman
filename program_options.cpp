#include <boost/program_options.hpp>
#include <boost/optional.hpp>
#include <string>
#include <iostream>
#include <exception>
#include <cstdint>
#include <chrono>
#include "program_options.hpp"

// Function retrieves the port from a valid address.
bool resolve_address(
  std::string &full_address, 
  std::string *address, 
  std::string *port
  ) {
  std::string::size_type last_colon;
  last_colon = full_address.rfind(":");
  if (last_colon == std::string::npos)
    return false;
  *address = full_address.substr(0, last_colon);
  *port = full_address.substr(last_colon + 1);
  return true;
}

// Function checks if the provided value of an options is valid.
template<typename T>
void bound_check(
  int64_t value,
  T &option,
  const std::string &name,
  bool non_zero = true
  ) {
  if (value < (int64_t) non_zero || value > std::numeric_limits<T>::max())
    throw OptionsError("Please provide a valid " + name + " value");
  option = (T) value;  
}

ClientOptions::ClientOptions(int argc, char* argv[]) {
  namespace po = boost::program_options;
  po::options_description desc("Allowed options");
  std::string gui_address_, server_address_;
  int64_t port_;
  desc.add_options()
    ("gui-address,d", po::value<std::string>(&gui_address_)->required(), "gui address")
    ("help,h", "produce help message")
    ("player-name,n", po::value<std::string>(&player_name)->required(), "player name")
    ("port,p", po::value<int64_t>(&port_)->required(), "port")
    ("server-address,s", po::value<std::string>(&server_address_)->required(), "server address")
    ;
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);

  if (vm.count("help")) {
    // Print the help message.
    std::cout << desc << "\n";
    exit(EXIT_SUCCESS);
  }
  po::notify(vm);

  if (!resolve_address(gui_address_, &gui_address, &gui_port))
    throw OptionsError("Please provide a valid gui address");
  if (!resolve_address(server_address_, &server_address, &server_port))
    throw OptionsError("Please provide a valid server address");
  bound_check(port_, port, "port");
}

ServerOptions::ServerOptions(int argc, char* argv[]) {
  namespace po = boost::program_options;
  po::options_description desc("Allowed options");
  int64_t bomb_timer_, players_count_, explosion_radius_,
          initial_blocks_, game_length_,
          port_, seed_, size_x_, size_y_;
  seed = static_cast<uint32_t>(
    std::chrono::system_clock::now().time_since_epoch().count()
  );  
        
  desc.add_options()
    ("bomb-timer,b", po::value<int64_t>(&bomb_timer_)->required(), "bomb timer")
    ("players-count,c", po::value<int64_t>(&players_count_)->required(), "players count")
    ("turn-duration,d", po::value<uint64_t>(&turn_duration)->required(), "turn duration")
    ("explosion-radius,e", po::value<int64_t>(&explosion_radius_)->required(), "explosion radius")
    ("help,h", "produce help message")
    ("initial-blocks,k", po::value<int64_t>(&initial_blocks_)->required(), "initial blocks")
    ("game-length,l", po::value<int64_t>(&game_length_)->required(), "game length")
    ("server-name,n", po::value<std::string>(&server_name)->required(), "server name")
    ("port,p", po::value<int64_t>(&port_)->required(), "port")
    ("seed,s", po::value<int64_t>(&seed_)->default_value(seed), "seed")
    ("size-x,x", po::value<int64_t>(&size_x_), "size x")
    ("size-y,y", po::value<int64_t>(&size_y_), "size y")
    ;
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);

  if (vm.count("help")) {
    // Print the help message.
    std::cout << desc << "\n";
    exit(EXIT_SUCCESS);
  }
  po::notify(vm);

  bound_check(bomb_timer_, bomb_timer, "bomb timer");
  bound_check(players_count_, players_count, "players count");
  bound_check(explosion_radius_, explosion_radius, "explosion radius", false);
  bound_check(initial_blocks_, initial_blocks, "initial blocks", false);
  bound_check(game_length_, game_length, "game length");
  bound_check(port_, port, "port");
  bound_check(seed_, seed, "seed", false);
  bound_check(size_x_, size_x, "size x");
  bound_check(size_y_, size_y, "size y");
}