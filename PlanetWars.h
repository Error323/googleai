// This file contains helper code that does all the boring stuff for you.
// The code in this file takes care of storing lists of planets and fleets, as
// well as communicating with the game engine. You can get along just fine
// without ever looking at this file. However, you are welcome to modify it
// if you want to.
#ifndef PLANET_WARS_H_
#define PLANET_WARS_H_

#include <string>
#include <vector>

#include "vec3.h"

// This is a utility class that parses strings.
class StringUtil {
 public:
  // Tokenizes a string s into tokens. Tokens are delimited by any of the
  // characters in delimiters. Blank tokens are omitted.
  static void Tokenize(const std::string& s,
                       const std::string& delimiters,
                       std::vector<std::string>& tokens);

  // A more convenient way of calling the Tokenize() method.
  static std::vector<std::string> Tokenize(
                       const std::string& s,
                       const std::string& delimiters = std::string(" "));
};

// This class stores details about one fleet. There is one of these classes
// for each fleet that is in flight at any given time.
class Fleet {
 public:
  // Initializes a fleet.
  Fleet(int owner = -1,
        int num_ships = -1,
        int source_planet = -1,
        int destination_planet = -1,
        int total_trip_length = -1,
        int turns_remaining = -1);

  // Returns the playerID of the owner of the fleet. Your player ID is always
  // 1. So if the owner is 1, you own the fleet. If the owner is 2 or some
  // other number, then this fleet belongs to your enemy.
  int Owner() const;

  // Returns the number of ships that comprise this fleet.
  int NumShips() const;

  // Returns the ID of the planet where this fleet originated.
  int SourcePlanet() const;

  // Returns the ID of the planet where this fleet is headed.
  int DestinationPlanet() const;

  // Returns the total distance that is being traveled by this fleet. This
  // is the distance between the source planet and the destination planet,
  // rounded up to the nearest whole number.
  int TotalTripLength() const;

  // Returns the number of turns until this fleet reaches its destination. If
  // this value is 1, then the fleet will hit the destination planet next turn.
  int TurnsRemaining() const;

  // Set a new value for turns remaining
  int TurnsRemaining(int new_turns_remaining);

  void Owner(int owner) { owner_ = owner; }
  void Backup();
  void Restore();

  friend std::ostream& operator<<(std::ostream&, const Fleet&);

 private:
  int bak_num_ships_;
  int bak_turns_remaining_;

  int owner_;
  int num_ships_;
  int source_planet_;
  int destination_planet_;
  int total_trip_length_;
  int turns_remaining_;
};

// Stores information about one planet. There is one instance of this class
// for each planet on the map.
class Planet {
 public:
  Planet(){}
  // Initializes a planet.
  Planet(int planet_id,
         int owner,
         int num_ships,
         int growth_rate,
         double x,
         double y);

  friend std::ostream& operator<<(std::ostream&, const Planet&);

  // Returns the ID of this planets. Planets are numbered starting at zero.
  int PlanetID() const;

  // Returns the ID of the player that owns this planet. Your playerID is
  // always 1. If the owner is 1, this is your planet. If the owner is 0, then
  // the planet is neutral. If the owner is 2 or some other number, then this
  // planet belongs to the enemy.
  int Owner() const;

  // The number of ships on the planet. This is the "population" of the planet.
  int NumShips() const;

  // Returns the growth rate of the planet. Unless the planet is neutral, the
  // population of the planet grows by this amount each turn. The higher this
  // number is, the faster this planet produces ships.
  int GrowthRate() const;

  // The position of the planet in space.
  double X() const;
  double Y() const;

  vec3<double> Loc() const;

  int Distance(const Planet& p) const;


  // Use the following functions to set the properties of this planet. Note
  // that these functions only affect your program's copy of the game state.
  // You can't steal your opponent's planets just by changing the owner to 1
  // using the Owner(int) function! :-)
  void Owner(int new_owner);
  void NumShips(int new_num_ships);
  void AddShips(int amount);
  void RemoveShips(int amount);
  void Backup();
  void Restore();

 private:
  int bak_owner_;
  int bak_num_ships_;

  int planet_id_;
  int owner_;
  int num_ships_;
  int growth_rate_;
  double x_, y_;
  vec3<double> location_;
};

class PlanetWars {
 public:
  // Initializes the game state given a string containing game state data.
  PlanetWars(const std::string& game_state);
  PlanetWars(){}

  // Returns a list of all the planets.
  std::vector<Planet>& Planets();

  // Return a list of all the fleets.
  std::vector<Fleet>& Fleets();

  // Writes a string which represents the current game state. This string
  // conforms to the Point-in-Time format from the project Wiki.
  std::string ToString() const;

  // Sends an order to the game engine. The order is to send num_ships ships
  // from source_planet to destination_planet. The order must be valid, or
  // else your bot will get kicked and lose the game. For example, you must own
  // source_planet, and you can't send more ships than you actually have on
  // that planet.
  void IssueOrder(int source_planet,
		  int destination_planet,
		  int num_ships) const;

  // Sends a message to the game engine letting it know that you're done
  // issuing orders for now.
  void FinishTurn() const;

 private:
  // Parses a game state from a string. On success, returns 1. On failure,
  // returns 0.
  int ParseGameState(const std::string& s);

  // Store all the planets and fleets. OMG we wouldn't wanna lose all the
  // planets and fleets, would we!?
  std::vector<Planet> planets_;
  std::vector<Fleet> fleets_;
};

#endif
