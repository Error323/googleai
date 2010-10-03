#include "Simulator.h"

#include <algorithm>
#include <fstream>

bool SortOnTurnsLeft(const Fleet& a, const Fleet& b) {
	return a.TurnsRemaining() < b.TurnsRemaining();
}

void Simulator::Start(unsigned int totalTurns, 
					std::vector<Planet>& planets, 
					std::vector<Fleet>& fleets) {

	AP = planets;
	AF = fleets;
	myNumShips = enemyNumShips = 0;

	sort(AF.begin(), AF.end(), SortOnTurnsLeft);

	unsigned int turnsTaken    = 0;
	unsigned int i             = 0;
	unsigned int n             = AF.size();
	// Begin simulation
	for (; i < n; i++)
	{
		Fleet& f = AF[i];
		if (f.TurnsRemaining() > totalTurns)
		{
			break;
		}

		int turnsRemaining = f.TurnsRemaining() - turnsTaken;

		// Don't exceed max amount of simulation turns
		if ((turnsRemaining+turnsTaken) > totalTurns)
		{
			turnsRemaining = totalTurns - turnsTaken;
		}

		// increase ships on all non-neutral planets
		if (turnsRemaining > 0)
		{
			for (unsigned int j = 0, m = AP.size(); j < m; j++)
			{
				Planet& p = AP[j];
				if (p.Owner() != 0)
				{
					p.AddShips(p.GrowthRate()*turnsRemaining);
				}
			}
		}

		// See if a planet gets a new owner
		// and add additional ships
		Planet& p = AP[f.DestinationPlanet()];

		if (p.Owner() == 0)
		{
			if (p.NumShips() < f.NumShips())
			{
				ChangeOwner(p, f, turnsTaken);
			}
			p.NumShips(abs(p.NumShips() - f.NumShips()));
		}
		else
		{
			if (p.Owner() == f.Owner())
			{
				p.NumShips(p.NumShips() + f.NumShips());
			}
			else
			{
				if (p.NumShips() < f.NumShips())
				{
					ChangeOwner(p, f, turnsTaken);
				}
				p.NumShips(abs(p.NumShips() - f.NumShips()));
			}
		}

		turnsTaken += turnsRemaining;
	}

	int turnsRemaining = totalTurns - turnsTaken;
	for (unsigned int j = 0, m = AP.size(); j < m; j++)
	{
		Planet& p = AP[j];
		if (p.Owner() != 0)
		{
			p.AddShips(p.GrowthRate()*turnsRemaining);
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

	for (; i < n; i++)
	{
		Fleet& f = AF[i];
		if (f.TurnsRemaining() <= totalTurns)
		{
			continue;
		}

		if (f.Owner() == 1)
		{
			myNumShips += f.NumShips();
		}
		else if (f.Owner() > 1)
		{
			enemyNumShips += f.NumShips();
		}
	}
	LOG("myNumShips: " << myNumShips << "\tenemyNumShips: " << enemyNumShips << "\twinning: " << Winning());
}

std::vector<std::pair<int,int> >& Simulator::GetOwnershipHistory(int i) { 
	if (ownershipHistory.find(i) == ownershipHistory.end())
	{
		ownershipHistory[i] = std::vector<std::pair<int, int> >();
	}

	return ownershipHistory[i]; 
}

void Simulator::ChangeOwner(Planet& p, Fleet& f, int time) {
	if (ownershipHistory.find(p.PlanetID()) == ownershipHistory.end())
	{
		ownershipHistory[p.PlanetID()] = std::vector<std::pair<int, int> >();
		ownershipHistory[p.PlanetID()].push_back(std::make_pair(p.Owner(), 0));
	}
	ownershipHistory[p.PlanetID()].push_back(std::make_pair(f.Owner(), time));

	p.Owner(f.Owner());
}
