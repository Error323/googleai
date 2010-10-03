#include "Simulator.h"

#include <vector>
#include <algorithm>
#include <fstream>
#include <cassert>

bool SortOnTurnsLeft(const Fleet& a, const Fleet& b) {
	return a.TurnsRemaining() < b.TurnsRemaining();
}

void Simulator::Start(unsigned int totalTurns, std::ofstream& file) {
	std::vector<Planet> AP = pw->Planets();
	std::vector<Fleet>  AF = pw->Fleets();

	sort(AF.begin(), AF.end(), SortOnTurnsLeft);

	unsigned int turnsTaken    = 0;
	unsigned int enemyNumShips = 0;
	unsigned int myNumShips    = 0;
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

		LOG("\t"<<f);

		int turnsRemaining = f.TurnsRemaining() - turnsTaken;

		assert(turnsRemaining >= 0);

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
				p.Owner(f.Owner());
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
					p.Owner(f.Owner());
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

	winning = myNumShips > enemyNumShips;
	LOG("myNumShips: " << myNumShips << "\tenemyNumShips: " << enemyNumShips << "\twinning: " << winning);
}
