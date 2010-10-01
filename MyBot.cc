#include "PlanetWars.h"

#include <iostream>
#include <fstream>

#define LOG(msg) { file << msg; file.flush(); }

void DoTurn(const PlanetWars& pw, std::fstream& file) {
  /*
  // (1) If we currently have a fleet in flight, just do nothing.
  if (pw.MyFleets().size() >= 1) {
    return;
  }
  // (2) Find my strongest planet.
  int source = -1;
  double source_score = -999999.0;
  int source_num_ships = 0;
  std::vector<Planet> my_planets = pw.MyPlanets();
  for (int i = 0; i < my_planets.size(); ++i) {
    const Planet& p = my_planets[i];
    double score = (double)p.NumShips();
    if (score > source_score) {
      source_score = score;
      source = p.PlanetID();
      source_num_ships = p.NumShips();
    }
  }
  // (3) Find the weakest enemy or neutral planet.
  int dest = -1;
  double dest_score = -999999.0;
  std::vector<Planet> not_my_planets = pw.NotMyPlanets();
  for (int i = 0; i < not_my_planets.size(); ++i) {
    const Planet& p = not_my_planets[i];
    double score = 1.0 / (1 + p.NumShips());
    if (score > dest_score) {
      dest_score = score;
      dest = p.PlanetID();
    }
  }
  // (4) Send half the ships from my strongest planet to the weakest
  // planet that I do not own.
  if (source >= 0 && dest >= 0) {
    int num_ships = source_num_ships / 2;
    pw.IssueOrder(source, dest, num_ships);
  }
  */
}

// This is just the main game loop that takes care of communicating with the
// game engine for you. You don't have to understand or change the code below.
int main(int argc, char *argv[]) {
  std::fstream file;
  file.open("mybot.txt", std::ios::in|std::ios::trunc);
  std::string current_line;
  std::string map_data;
  while (true) {
    int c = std::cin.get();
    current_line += (char)c;
    if (c == '\n') {
      if (current_line.length() >= 2 && current_line.substr(0, 2) == "go") {
        PlanetWars pw(map_data);
        map_data = "";
        DoTurn(pw, file);
	pw.FinishTurn();
      } else {
        map_data += current_line;
      }
      current_line = "";
    }
  }
  file.close();
  return 0;
}
