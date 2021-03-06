#ifndef SIMULATE_H_
#define SIMULATE_H_

#include "PlanetWars.h"
#include "Logger.h"

#include <vector>
#include <map>

class Simulator {
public:
	Simulator():
		myNumShips(0),
		enemyNumShips(0)
	{
	}

	struct PlanetOwner {
		PlanetOwner(){}
		PlanetOwner(int o, int t, int f, int n):
			owner(o),
			time(t),
			force(f),
			numships(n)
		{}
		int owner;    // owner of the incomming fleet
		int time;     // time of impact relative to current turn
		int force;    // force of incomming fleet
		int numships; // numships on planet before impact
	};

	void Start(int, std::vector<Planet>&, std::vector<Fleet>&, bool removeFleets = true, bool makeCopy = false);
	std::vector<PlanetOwner>& GetOwnershipHistory(int i);
	PlanetOwner& GetFirstEnemyOwner(int i);

	bool Winning()					{ return myNumShips > enemyNumShips; }
	bool IsNeutralPlanet(int i) 	{ return AP->at(i).Owner() == 0; }
	bool IsMyPlanet(int i) 			{ return AP->at(i).Owner() == 1; }
	bool IsEnemyPlanet(int i) 		{ return AP->at(i).Owner() > 1; }
	int MyNumShips()				{ return myNumShips; }
	int EnemyNumShips()				{ return enemyNumShips; }
	int GetScore()					{ return myNumShips - enemyNumShips; }
	Planet& GetPlanet(int i)		{ return AP->at(i); }

private:
	int myNumShips;
	int enemyNumShips;
	std::vector<Planet> copyAP; // local deepcopy of all planets
	std::vector<Fleet> copyAF;  // local deepcopy of all fleets
	std::vector<Planet>* AP; // active planets, either from the deepcopy or passed by reference from Start()
	std::vector<Fleet>* AF;  // active fleets, either from the deepcopy or passed by reference from Start()
	std::map<int, std::vector<PlanetOwner> > ownershipHistory; // history record of fleet impacts in a planet

	void ChangeOwner(Planet& p, int owner, int time, int force);
};

#endif
