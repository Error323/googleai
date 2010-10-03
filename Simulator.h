#ifndef SIMULATE_H_
#define SIMULATE_H_

#include "PlanetWars.h"

#include <vector>

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

private:
	int myNumShips;
	int enemyNumShips;
	std::ofstream& file;
};

#endif
