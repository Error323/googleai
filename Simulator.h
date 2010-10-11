#ifndef SIMULATE_H_
#define SIMULATE_H_

#include "PlanetWars.h"

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
		PlanetOwner(int o, int t, int f):
			owner(o),
			time(t),
			force(f)
		{}
		int owner;
		int time;
		int force;
	};

	void Start(int, std::vector<Planet>&, std::vector<Fleet>&, bool removeFleets = true, bool makeCopy = false);
	std::vector<PlanetOwner>& GetOwnershipHistory(int i);
	PlanetOwner& GetFirstEnemyOwner(int i);

	bool Winning()					{ return myNumShips > enemyNumShips; }
	bool IsMyPlanet(int i) 			{ return AP->at(i).Owner() == 1; }
	bool IsEnemyPlanet(int i) 		{ return AP->at(i).Owner() > 1; }
	int MyNumShips()				{ return myNumShips; }
	int EnemyNumShips()				{ return enemyNumShips; }
	int GetScore()					{ return myNumShips - enemyNumShips; }
	Planet& GetPlanet(int i)		{ return AP->at(i); }

private:
	int myNumShips;
	int enemyNumShips;
	std::vector<Planet> _AP;
	std::vector<Fleet> _AF;
	std::vector<Planet>* AP;
	std::vector<Fleet>* AF;
	// <planet, vec<owner, time> >
	std::map<int, std::vector<PlanetOwner> > ownershipHistory;
	void ChangeOwner(Planet& p, int owner, int time, int force);
};

#endif
