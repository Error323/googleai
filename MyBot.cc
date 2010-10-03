#include "PlanetWars.h"
#include "Simulator.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <limits>
#include <cassert>
#include <utility>

#define VERSION "7.0"

#define MAX_ROUNDS 200

void DoTurn(PlanetWars& pw, int round, std::ofstream& file) {
	std::vector<Fleet>  orders;
	std::vector<Fleet>  AF = pw.Fleets();
	std::vector<Planet> AP = pw.Planets();

	int maxTurnsRemaining = 0;
	for (unsigned int i = 0, n = AF.size(); i < n; i++)
	{
		Fleet& f = AF[i];
		maxTurnsRemaining = std::max<int>(maxTurnsRemaining, f.TurnsRemaining());
	}

	// (1) Simulate ahead such that all fleets are consumed
	Simulator s(file);
	s.Start(maxTurnsRemaining, AP, AF);
	std::vector<Planet> MP = pw.MyPlanets();

	// (2) Filter out those planets that used to be neutral and attack them
	std::vector<Planet> simAP = s.Planets();
	for (unsigned int i = 0, n = simAP.size(); i < n; i++)
	{
		Planet& simTarget = simAP[i]; // planet with new properties
		Planet& target    = AP[i];    // planet with current properties

		if (simTarget.Owner() > 1 && target.Owner() == 0)
		{
			int timeOfOwnerShipChange = s.GetTimeOfOwnerShipChange(i);
			for (unsigned int j = 0, m = MP.size(); j < m; j++)
			{
				Planet& source = MP[j];
				const int distance = pw.Distance(source.PlanetID(), target.PlanetID());

				// if we don't have enough ships anymore, skip
				if (source.NumShips() <= 1)
				{
					continue;
				}

				const int targetGrowth = std::max<int>(0, distance-timeOfOwnerShipChange)*target.GrowthRate();
				const int fleetSize = std::min<int>(source.NumShips() - 1, target.NumShips() + targetGrowth + 1);

				orders.push_back(Fleet(1, fleetSize, source.PlanetID(), i, distance, distance));
				source.NumShips(source.NumShips() - fleetSize);
				target.NumShips(target.NumShips() - fleetSize);

				// This planet is ours, proceed to next target
				if (target.NumShips() < 0)
				{
					break;
				}
				
			}
		}
	}

	for (unsigned int i = 0, n = orders.size(); i < n; i++)
	{
		Fleet& order = orders[i];
		pw.IssueOrder(order.SourcePlanet(), order.DestinationPlanet(), order.NumShips());
	}

	/*
	// (3) engage neutral planets as soon as an enemyfleet is underway and is
	// closer, this allows us to use less fleets
	for (unsigned int i = 0, n = neutralPlanets.size(); i < n; i++)
	{
		Planet& target = neutralPlanets[i];

		int numShipsRequired = target.NumShips();
		int myMinimalDistance = std::numeric_limits<int>::max();
		bool owned = false;
		for (unsigned int j = 0, m = notMyFleets.size(); j < m; j++)
		{
			Fleet& f = notMyFleets[j];
			if (f.DestinationPlanet() == target.PlanetID())
			{
				if (owned)
				{
					numShipsRequired += f.NumShips();
				}
				else
				{
					numShipsRequired -= f.NumShips();
				}

				owned = owned || numShipsRequired < 0;

				if (owned)
				{
					myMinimalDistance = std::min<int>(myMinimalDistance, f.TurnsRemaining());
					numShipsRequired = abs(numShipsRequired);
				}
			}
		}

		for (unsigned int j = 0, m = myFleets.size(); j < m; j++)
		{
			Fleet& f = myFleets[j];
			if (f.DestinationPlanet() == target.PlanetID())
			{
				numShipsRequired -= f.NumShips();
			}
		}

		// Already on it
		if (numShipsRequired < 0)
		{
			continue;
		}

		// If the enemy fleet isn't underway, neither will we
		if (!owned)
		{
			continue;
		}

		// if the required number is greater than we can handle, don't
		if ((numShipsRequired+myMinimalDistance*enemyGrowthRate) >= (myNumShips+myMinimalDistance*myGrowthRate))
		{
			LOG("Enemy fleets too big " << numShipsRequired);
			continue;
		}

		globalTarget = &target;
		target.NumShips(numShipsRequired);
		sort(myPlanets.begin(), myPlanets.end(), cmp2);

		// send fleets until we've captured the planet
		for (unsigned int j = 0, m = myPlanets.size(); j < m; j++)
		{
			Planet& source = myPlanets[j];
			const int distance = pw.Distance(source.PlanetID(), target.PlanetID());

			// if we don't have enough ships anymore, or myMinimalDistance isn't satisfied, skip
			if (source.NumShips() <= 1 || distance <= myMinimalDistance)
			{
				continue;
			}

			const int targetGrowth = (distance-myMinimalDistance)*target.GrowthRate();
			const int fleetSize = std::min<int>(source.NumShips() - 1, target.NumShips() + targetGrowth + 1);

			myNumShips -= fleetSize;
			pw.IssueOrder(source.PlanetID(), target.PlanetID(), fleetSize);
			source.NumShips(source.NumShips() - fleetSize);
			target.NumShips(target.NumShips() - fleetSize);

			// This planet is ours, proceed to next target
			if (target.NumShips() < 0)
			{
				break;
			}
		}
	}
	

	// (4) attack enemy planets
	for (unsigned int i = 0, n = enemyPlanets.size(); i < n; i++)
	{
		Planet& target = enemyPlanets[i];
		const bool isNeutral = target.Owner() == 0;

		int targetNumShips = target.NumShips();

		// subtract our incomming fleets
		for (unsigned int j = 0, m = myFleets.size(); j < m; j++)
		{
			Fleet& f = myFleets[j];
			if (f.DestinationPlanet() == target.PlanetID())
			{
				targetNumShips -= f.NumShips();
			}
		}

		// add their incomming fleets
		for (unsigned int j = 0, m = notMyFleets.size(); j < m; j++)
		{
			Fleet& f = notMyFleets[j];
			if (f.DestinationPlanet() == target.PlanetID())
			{
				targetNumShips += f.NumShips();
			}
		}

		// if this target will be ours, we don't need to attack it, skip target
		if (targetNumShips < 0)
		{
			continue;
		}

		// if we don't have the amount of ships to deal with this, skip target
		if ((myNumShips - targetNumShips) < 0)
		{
			continue;
		}

		target.NumShips(targetNumShips);

		// sort our sourceplanets on distance from target, creating an outwards
		// spiral
		globalTarget = &target;
		sort(myPlanets.begin(), myPlanets.end(), cmp2);

		// send fleets until we've captured the planet
		for (unsigned int j = 0, m = myPlanets.size(); j < m; j++)
		{
			Planet& source = myPlanets[j];

			if (source.NumShips() <= 1)
			{
				continue;
			}

			const int distance = pw.Distance(source.PlanetID(), target.PlanetID());
			const int targetGrowth = isNeutral ? 0 : distance*target.GrowthRate();
			const int fleetSize = std::min<int>(source.NumShips() - 1, target.NumShips() + targetGrowth + 1);

			myNumShips -= fleetSize;
			pw.IssueOrder(source.PlanetID(), target.PlanetID(), fleetSize);
			source.NumShips(source.NumShips() - fleetSize);
			target.NumShips(target.NumShips() - fleetSize);

			// This planet is ours, proceed to next target
			if (target.NumShips() < 0)
			{
				break;
			}
		}
	}
	*/
}

// This is just the main game loop that takes care of communicating with the
// game engine for you. You don't have to understand or change the code below.
int main(int argc, char *argv[]) {
	unsigned int round = 0;
	std::ofstream file;
	std::string filename("-Error323-v");
	filename = argv[0] + filename + VERSION + ".txt";
	file.open(filename.c_str(), std::ios::in|std::ios::trunc);
	std::string current_line;
	std::string map_data;
	while (true) {
		int c = std::cin.get();
		current_line += (char)c;
		if (c == '\n') 
		{
			if (current_line.length() >= 2 && current_line.substr(0, 2) == "go") 
			{
				PlanetWars pw(map_data);
				map_data = "";
				LOG("round: " << round);
				DoTurn(pw, round, file);
				LOG("\n--------------------------------------------------------------------------------\n");
				round++;
				pw.FinishTurn();
			} 
			else 
			{
				map_data += current_line;
			}
			current_line = "";
		}
	}
	if (file.good())
	{
		file.close();
	}
	return 0;
}
