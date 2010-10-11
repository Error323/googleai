#include "Simulator.h"
#include "Logger.h"

#include <algorithm>
#include <list>

bool SortOnPlanetAndTurnsLeft(const Fleet& a, const Fleet& b) {
	if (a.DestinationPlanet() == b.DestinationPlanet())
		return a.TurnsRemaining() < b.TurnsRemaining();
	else
		return a.DestinationPlanet() < b.DestinationPlanet();
}

void Simulator::Start(int totalTurns, 
					std::vector<Planet>& ap, 
					std::vector<Fleet>& af,
					bool passOn) {

	myNumShips = enemyNumShips = 0;
	ownershipHistory.clear();
	std::vector<int> skipPlanets;
	std::list<unsigned int> remove;

	std::vector<Planet>* AP = &ap;
	std::vector<Fleet>*	 AF = &af;
	if (!passOn)
	{
		_AP.resize(ap.size());
		_AF.resize(af.size());
		copy(ap.begin(), ap.end(), _AP.begin());
		copy(af.begin(), af.end(), _AF.begin());
		AP = &_AP;
		AF = &_AF;
	}

	sort(AF->begin(), AF->end(), SortOnPlanetAndTurnsLeft);

	// Begin simulation
	int turnsTaken = 0;
	for (unsigned int i = 0, n = AF->size(); i < n; i++)
	{
		Fleet& f = AF->at(i);
		unsigned int index = i;

		if (passOn) ASSERT(f.TurnsRemaining() > 0);

		int turnsRemaining = f.TurnsRemaining() - turnsTaken;
		if ((turnsTaken + turnsRemaining) > totalTurns)
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
			AF->at(i+1).DestinationPlanet() == f.DestinationPlanet() &&
			AF->at(i+1).TurnsRemaining() == f.TurnsRemaining()
		)
		{
			i++;
			fleetsWithSameDestAndTurns.push_back(i);
			if (forces.find(AF->at(i).Owner()) == forces.end())
			{
				forces[AF->at(i).Owner()] = 0;
			}
			forces[AF->at(i).Owner()] += AF->at(i).NumShips();
		}
		for (unsigned int j = 0, m = fleetsWithSameDestAndTurns.size(); j < m; j++)
		{
			Fleet& ff = AF->at(fleetsWithSameDestAndTurns[j]);
			ff.TurnsRemaining(ff.TurnsRemaining()-turnsTaken);
			if (ff.TurnsRemaining() <= 0)
			{
				remove.push_front(index + j);
			}
		}
		
		// Add ship growth for non-neutral planets
		Planet& p = AP->at(f.DestinationPlanet());
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
			int force = 0;
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
					ChangeOwner(p, owner, turnsTaken, force);
				}
				p.NumShips(abs(p.NumShips() - force));
			}
		}

		// This planet has no more fleets, add additional simulation growth and
		// reset turns taken
		if (i == n-1 || AF->at(i+1).DestinationPlanet() != f.DestinationPlanet())
		{
			if (turnsTaken < totalTurns)
			{
				p.AddShips((totalTurns-turnsTaken)*p.GrowthRate());
			}
			turnsTaken = 0;
			skipPlanets.push_back(p.PlanetID());
		}
	}

	// TODO: Optimize
	if (passOn)
	{
		typedef std::list<unsigned int>::iterator Iter;
		for (Iter i = remove.begin(); i != remove.end(); i++)
		{
			AF->erase(AF->begin()+(*i));
		}
	}

	for (unsigned int i = 0, n = AP->size(); i < n; i++)
	{
		Planet& p = AP->at(i);
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

	for (unsigned int i = 0, n = AF->size(); i < n; i++)
	{
		Fleet& f = AF->at(i);

		if (passOn) ASSERT(f.TurnsRemaining() > 0);

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

std::vector<Simulator::PlanetOwner>& Simulator::GetOwnershipHistory(int i) { 
	ASSERT(ownershipHistory.find(i) != ownershipHistory.end());
	return ownershipHistory[i]; 
}

void Simulator::ChangeOwner(Planet& p, int owner, int time, int force) {
	if (ownershipHistory.find(p.PlanetID()) == ownershipHistory.end())
	{
		ownershipHistory[p.PlanetID()] = std::vector<PlanetOwner>();
	}
	ownershipHistory[p.PlanetID()].push_back(PlanetOwner(owner,time,force));
	p.Owner(owner);
}

Simulator::PlanetOwner& Simulator::GetFirstEnemyOwner(int i) {
	std::vector<Simulator::PlanetOwner>& H = GetOwnershipHistory(i);
	for (unsigned int j = 0, n = H.size(); j < n; j++)
	{
		if (H[j].owner > 1)
		{
			return H[j];
		}
	}
	return H[0];
}
