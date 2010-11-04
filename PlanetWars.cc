#include "PlanetWars.h"
#include "Logger.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#define MAX_STACK_SIZE 3

void StringUtil::Tokenize(const std::string& s,
                          const std::string& delimiters,
                          std::vector<std::string>& tokens) {
  std::string::size_type lastPos = s.find_first_not_of(delimiters, 0);
  std::string::size_type pos = s.find_first_of(delimiters, lastPos);
  while (std::string::npos != pos || std::string::npos != lastPos) {
    tokens.push_back(s.substr(lastPos, pos - lastPos));
    lastPos = s.find_first_not_of(delimiters, pos);
    pos = s.find_first_of(delimiters, lastPos);
  }
}

std::vector<std::string> StringUtil::Tokenize(const std::string& s,
                                              const std::string& delimiters) {
  std::vector<std::string> tokens;
  Tokenize(s, delimiters, tokens);
  return tokens;
}

Fleet::Fleet(int owner,
             int num_ships,
             int source_planet,
             int destination_planet,
             int total_trip_length,
             int turns_remaining) {
  owner_ = owner;
  num_ships_ = num_ships;
  source_planet_ = source_planet;
  destination_planet_ = destination_planet;
  total_trip_length_ = total_trip_length;
  turns_remaining_ = turns_remaining;
}

int Fleet::Owner() const {
  return owner_;
}

int Fleet::NumShips() const {
  return num_ships_;
}

int Fleet::SourcePlanet() const {
  return source_planet_;
}

int Fleet::DestinationPlanet() const {
  return destination_planet_;
}

int Fleet::TotalTripLength() const {
  return total_trip_length_;
}

int Fleet::TurnsRemaining() const {
  return turns_remaining_;
}

int Fleet::TurnsRemaining(int new_turns_remaining) {
  return turns_remaining_ = new_turns_remaining;
}

void Fleet::Backup() {
  bak_turns_remaining_ = turns_remaining_;
  bak_num_ships_       = num_ships_;
}

void Fleet::Restore() {
  turns_remaining_ = bak_turns_remaining_;
  num_ships_ = bak_num_ships_;
}

Planet::Planet(int planet_id,
               int owner,
               int num_ships,
               int growth_rate,
               double x,
               double y) {
  planet_id_ = planet_id;
  owner_ = owner;
  num_ships_ = num_ships;
  growth_rate_ = growth_rate;
  x_ = x;
  y_ = y;
  location_.x = x;
  location_.y = 0.0;
  location_.z = y;
}

int Planet::PlanetID() const {
  return planet_id_;
}

int Planet::Owner() const {
  return owner_;
}

int Planet::NumShips() const {
  return num_ships_;
}

vec3<double> Planet::Loc() const {
  return location_;
}

int Planet::Distance(const Planet& p) const {
  return int(ceil((p.Loc() - location_).len2D()));
}

int Planet::GrowthRate() const {
  return growth_rate_;
}

double Planet::X() const {
  return x_;
}

double Planet::Y() const {
  return y_;
}

void Planet::Owner(int new_owner) {
  owner_ = new_owner;
}

void Planet::NumShips(int new_num_ships) {
  num_ships_ = new_num_ships;
}

void Planet::AddShips(int amount) {
  num_ships_ += amount;
}

void Planet::RemoveShips(int amount) {
  num_ships_ -= amount;
  ASSERT(num_ships_ >= 0);
}

void Planet::Backup() {
  bak_owner_     = owner_;
  bak_num_ships_ = num_ships_;
}

void Planet::Restore() {
  owner_     = bak_owner_;
  num_ships_ = bak_num_ships_;
}

PlanetWars::PlanetWars(const std::string& gameState) {
  ParseGameState(gameState);
}

std::vector<Planet>& PlanetWars::Planets() {
  return planets_;
}

std::vector<Fleet>& PlanetWars::Fleets() {
  return fleets_;
}

std::string PlanetWars::ToString() const {
  std::stringstream s;
  for (unsigned int i = 0; i < planets_.size(); ++i) {
    const Planet& p = planets_[i];
    s << p << std::endl;
  }
  for (unsigned int i = 0; i < fleets_.size(); ++i) {
    const Fleet& f = fleets_[i];
    s << f << std::endl;
  }
  return s.str();
}

void PlanetWars::IssueOrder(int source_planet,
                            int destination_planet,
                            int num_ships) const {
  std::cout << source_planet << " "
            << destination_planet << " "
            << num_ships << std::endl;
  std::cout.flush();
}

int PlanetWars::ParseGameState(const std::string& s) {
  planets_.clear();
  fleets_.clear();
  std::vector<std::string> lines = StringUtil::Tokenize(s, "\n");
  int planet_id = 0;
  for (unsigned int i = 0; i < lines.size(); ++i) {
    std::string& line = lines[i];
    size_t comment_begin = line.find_first_of('#');
    if (comment_begin != std::string::npos) {
      line = line.substr(0, comment_begin);
    }
    std::vector<std::string> tokens = StringUtil::Tokenize(line);
    if (tokens.size() == 0) {
      continue;
    }
    if (tokens[0] == "P") {
      if (tokens.size() != 6) {
        return 0;
      }
      Planet p(planet_id++,              // The ID of this planet
	       atoi(tokens[3].c_str()),  // Owner
               atoi(tokens[4].c_str()),  // Num ships
               atoi(tokens[5].c_str()),  // Growth rate
               atof(tokens[1].c_str()),  // X
               atof(tokens[2].c_str())); // Y
      planets_.push_back(p);
    } else if (tokens[0] == "F") {
      if (tokens.size() != 7) {
        return 0;
      }
      Fleet f(atoi(tokens[1].c_str()),  // Owner
              atoi(tokens[2].c_str()),  // Num ships
              atoi(tokens[3].c_str()),  // Source
              atoi(tokens[4].c_str()),  // Destination
              atoi(tokens[5].c_str()),  // Total trip length
              atoi(tokens[6].c_str())); // Turns remaining
	  if (f.NumShips() > 0)
	  {
        fleets_.push_back(f);
	  }
    } else {
      return 0;
    }
  }
  return 1;
}

void PlanetWars::FinishTurn() const {
  std::cout << "go" << std::endl;
  std::cout.flush();
}

std::ostream& operator<<(std::ostream &out, const Planet& p) {
	char data[10];
	out << "Planet{id:";
	snprintf(data, 10, "%2.2d", p.PlanetID());
	out << data;
	out << ", owner:";
	if (p.Owner() == 0)
		out << "N";
	else
	if (p.Owner() == 1)
		out << "U";
	else
		out << "T";
	out << ", numships:";
	snprintf(data, 10, "%3.3d", p.NumShips());
	out << data;
	out << ", growthrate:";
	snprintf(data, 10, "%1.1d", p.GrowthRate());
	out << data;
	out << "}";

	return out;
}

std::ostream& operator<<(std::ostream& out, const Fleet& f) {
	char data[10];
	out << "Fleet{owner:";
	out << (f.Owner() == 1 ? "U" : "T");
	out << ", numships:";
	snprintf(data, 10, "%3.3d", f.NumShips());
	out << data;
	out << ", source:";
	out << f.SourcePlanet();
	out << ", dest:";
	out << f.DestinationPlanet();
	out << ", totaltriptime:";
	out << f.TotalTripLength();
	out << ", remaining:";
	out << f.TurnsRemaining();
	out << "}";

	return out;
}
