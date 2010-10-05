#ifndef SIMULATE_H_
#define SIMULATE_H_

#include "PlanetWars.h"

#include <vector>
#include <map>
#include <utility>

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
	std::vector<std::pair<int,int> >& GetOwnershipHistory(int i);

	bool Winning()					{ return myNumShips > enemyNumShips; }
	bool IsMyPlanet(int i) 			{ return AP[i].Owner() == 1; }
	bool IsEnemyPlanet(int i) 		{ return AP[i].Owner() > 1; }
	int MyScore()					{ return myNumShips; }
	int EnemyScore()				{ return enemyNumShips; }
	std::vector<Planet>& Planets()	{ return AP; }
	std::vector<Fleet>&  Fleets()	{ return AF; }
	Planet& GetPlanet(int i)		{ return AP[i]; }

private:
	int myNumShips;
	int enemyNumShips;
	std::ofstream& file;
	std::vector<Planet> AP;
	std::vector<Fleet>  AF;
	// <planet, vec<owner, time> >
	std::map<int, std::vector<std::pair<int, int> > > ownershipHistory;
	void ChangeOwner(Planet& p, Fleet& f, int time);
};

#endif
