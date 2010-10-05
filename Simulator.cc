#include "Simulator.h"

#include <algorithm>
#include <fstream>
#include <cassert>

bool SortOnPlanetAndTurnsLeft(const Fleet& a, const Fleet& b) {
	if (a.DestinationPlanet() == b.DestinationPlanet())
		return a.TurnsRemaining() < b.TurnsRemaining();
	else
		return a.DestinationPlanet() < b.DestinationPlanet();
}

void Simulator::Start(unsigned int totalTurns, 
					std::vector<Planet>& planets, 
					std::vector<Fleet>& fleets) {

	AP = planets;
	AF = fleets;
	myNumShips = enemyNumShips = 0;
	ownershipHistory.clear();
	std::vector<int> skipPlanets;

	sort(AF.begin(), AF.end(), SortOnPlanetAndTurnsLeft);

	// Begin simulation
	int turnsTaken = 0;
	for (unsigned int i = 0, n = AF.size(); i < n; i++)
	{
		Fleet& f = AF[i];

		int turnsRemaining = f.TurnsRemaining() - turnsTaken;
		if (turnsTaken + turnsRemaining > totalTurns)
		{
			turnsRemaining = totalTurns - turnsTaken;
		}
		turnsTaken += turnsRemaining;

		std::map<int, int> forces;
		forces[f.Owner()] = f.NumShips();
		std::vector<int> fleetsWithSameDestAndTurns;
		fleetsWithSameDestAndTurns.push_back(i);

		// if the next fleet has the same destination planet AND the same
		// amount of turns remaining, gather all the forces
		while (
			i < n-1 && 
			AF[i+1].DestinationPlanet() == f.DestinationPlanet() &&
			AF[i+1].TurnsRemaining() == f.TurnsRemaining()
		)
		{
			i++;
			fleetsWithSameDestAndTurns.push_back(i);
			if (forces.find(AF[i].Owner()) == forces.end())
			{
				forces[AF[i].Owner()] = 0;
			}
			forces[AF[i].Owner()] += AF[i].NumShips();
		}
		for (unsigned int j = 0, m = fleetsWithSameDestAndTurns.size(); j < m; j++)
		{
			Fleet& ff = AF[fleetsWithSameDestAndTurns[j]];
			ff.TurnsRemaining(ff.TurnsRemaining()-turnsTaken);
		}
		
		// Add ship growth for non-neutral planets
		Planet& p = AP[f.DestinationPlanet()];
		if (p.Owner() > 0)
		{
			p.AddShips(turnsRemaining*p.GrowthRate());
		}

		// If there are more then one force and if all forces are the same
		// size, the winner is the original planet owner and the new
		// shipcount is zero
		int forceBegin = forces.begin()->second;
		int forceEnd   = (--forces.end())->second;
		if (forces.size() > 1 && forceBegin >= p.NumShips() && forceBegin == forceEnd)
		{
			p.NumShips(0);
		}
		else
		{
			// Determine biggest force
			int owner = p.Owner();
			int force = p.NumShips();
			for (std::map<int,int>::iterator j = forces.begin(); j != forces.end(); j++)
			{
				if (j->second >= force)
				{
					owner = j->first;
					force = j->second;
				}
			}

			// Change the planet owner to the biggest force and add it
			if (p.Owner() == owner)
			{
				p.AddShips(force);
			}
			else
			{
				if (force > p.NumShips())
				{
					ChangeOwner(p, owner, turnsTaken);
				}
				p.NumShips(abs(p.NumShips() - force));
			}
		}

		// This planet has no more fleets, add additional simulation growth and
		// reset turns taken
		if (i == n-1 || AF[i+1].DestinationPlanet() != f.DestinationPlanet())
		{
			if (turnsTaken < totalTurns)
			{
				p.AddShips((totalTurns-turnsTaken)*p.GrowthRate());
			}
			turnsTaken = 0;
		}
		skipPlanets.push_back(p.PlanetID());
	}

	for (unsigned int i = 0, n = AP.size(); i < n; i++)
	{
		Planet& p = AP[i];
		if (p.Owner() != 0 && find(skipPlanets.begin(), skipPlanets.end(), p.PlanetID()) == skipPlanets.end())
		{
			p.AddShips(p.GrowthRate()*totalTurns);
		}

		if (p.Owner() == 1)
		{
			myNumShips += p.NumShips();
		}
		else if (p.Owner() > 1)
		{
			enemyNumShips += p.NumShips();
		}
	}

	for (unsigned int i = 0, n = AF.size(); i < n; i++)
	{
		Fleet& f = AF[i];

		// Fleet landed on planet...
		if (f.TurnsRemaining() <= 0)
		{
			continue;
		}

		if (f.Owner() == 1)
		{
			myNumShips += f.NumShips();
		}
		else
		{
			enemyNumShips += f.NumShips();
		}
	}
}

std::vector<std::pair<int,int> >& Simulator::GetOwnershipHistory(int i) { 
	if (ownershipHistory.find(i) == ownershipHistory.end())
	{
		ownershipHistory[i] = std::vector<std::pair<int, int> >();
	}

	return ownershipHistory[i]; 
}

void Simulator::ChangeOwner(Planet& p, int owner, int time) {
	if (ownershipHistory.find(p.PlanetID()) == ownershipHistory.end())
	{
		ownershipHistory[p.PlanetID()] = std::vector<std::pair<int, int> >();
		ownershipHistory[p.PlanetID()].push_back(std::make_pair(p.Owner(), 0));
	}
	ownershipHistory[p.PlanetID()].push_back(std::make_pair(owner, time));

	p.Owner(owner);
}
