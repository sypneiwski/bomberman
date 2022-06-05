#ifndef PROGRAM_OPTIONS_HPP
#define PROGRAM_OPTIONS_HPP
#include <string>
#include <exception>

class OptionsError : public std::invalid_argument {
public:
  OptionsError(std::string const& e) : std::invalid_argument(e) {}
};

// Struct for parsing and storing all client options from the command line.
struct ClientOptions {
  std::string gui_address,
              gui_port,
              player_name,
              server_address,
              server_port;
  uint16_t port;

  ClientOptions(int, char*[]);
};

// Struct for parsing and storing all server options from the command line.
struct ServerOptions {
  std::string server_name;
  uint16_t bomb_timer,
           explosion_radius,
           initial_blocks,
           game_length,
           port,
           size_x,
           size_y;
  uint8_t players_count;
  uint64_t turn_duration;
  uint32_t seed;  

  ServerOptions(int, char*[]);       
};

#endif // PROGRAM_OPTIONS_HPP