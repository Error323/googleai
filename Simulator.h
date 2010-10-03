#ifndef SIMULATE_H_
#define SIMULATE_H_

#include "PlanetWars.h"

#include <vector>
#include <map>

#define LOG(msg)                  \
	if (file.good())              \
	{                             \
		file << msg << std::endl; \
	}

class Simulator {
public:
	Simulator(std::ofstream& filestream):
		myNumShips(0),
		enemyNumShips(0),
		file(filestream)
	{
	}

	void Start(unsigned int, std::vector<Planet>&, std::vector<Fleet>&);
	bool Winning()    { return myNumShips > enemyNumShips; }
	int MyScore()     { return myNumShips; }
	int EnemyScore()  { return enemyNumShips; }
	std::vector<Planet>& Planets() { return AP; }
	std::vector<Fleet>&  Fleets()  { return AF; }
	int GetTimeOfOwnerShipChange(int i) { return time.find(i) == time.end() ? 0 : time[i]; }
	Planet& GetPlanet(int i) { return AP[i]; }

private:
	int myNumShips;
	int enemyNumShips;
	std::ofstream& file;
	std::vector<Planet> AP;
	std::vector<Fleet>  AF;
	std::map<int, int>  time; ///< time of _first_ planet ownership change <planetid,time>
};

#endif
