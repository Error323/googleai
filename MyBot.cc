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

PlanetWars* globalPW     = NULL;
int         globalTarget = 0;

bool SortOnDestAndDist(const Fleet& a, const Fleet& b) {
	if (a.DestinationPlanet() == b.DestinationPlanet())
		return a.TurnsRemaining() > b.TurnsRemaining();
	else
		return a.DestinationPlanet() < b.DestinationPlanet();
}

bool SortOnDistanceToTarget(const Planet& a, const Planet& b) {
	int distA = globalPW->Distance(a.PlanetID(), globalTarget);
	int distB = globalPW->Distance(b.PlanetID(), globalTarget);
	return distA < distB;
}

void DoTurn(PlanetWars& pw, int round, std::ofstream& file) {
	globalPW = &pw;
	std::vector<Fleet>  AF = pw.Fleets();
	std::vector<Planet> AP = pw.Planets();
	std::vector<Planet> MP = pw.MyPlanets();

	sort(AF.begin(), AF.end(), SortOnDestAndDist);
	for (unsigned int i = 0, n = AF.size(); i < n; i++)
	{
		Fleet& f = AF[i];
		Simulator s(file);
		s.Start(f.TurnsRemaining(), AP, AF);

		// skip all other incomming fleets for this planet, they are already in
		// the simulation, as we sort on dist descending
		while (i < n-1 && f.DestinationPlanet() == AF[i+1].DestinationPlanet())
		{
			i++;
		}

		Planet& simTarget = s.GetPlanet(f.DestinationPlanet());
		Planet& nowTarget = AP[f.DestinationPlanet()];

		if (simTarget.Owner() > 1 && nowTarget.Owner() <= 1)
		{
			std::vector<Fleet> orders;
			bool successfullAttack = false;
			int timeOfOwnerShipChange = s.GetTimeOfOwnerShipChange(f.DestinationPlanet());

			// sort my planets on distance to current target
			globalTarget = nowTarget.PlanetID();
			sort(MP.begin(), MP.end(), SortOnDistanceToTarget);
			for (unsigned int j = 0, m = MP.size(); j < m; j++)
			{
				Planet& source = MP[j];
				const int distance = pw.Distance(source.PlanetID(), nowTarget.PlanetID());

				// if we don't have enough ships anymore, skip
				if (source.NumShips() <= 1)
				{
					continue;
				}

				const int targetGrowth = std::max<int>(1, distance-timeOfOwnerShipChange)*nowTarget.GrowthRate();
				const int fleetSize = std::min<int>(source.NumShips() - 1, simTarget.NumShips() + targetGrowth + 1);

				orders.push_back(Fleet(1, fleetSize, source.PlanetID(), f.DestinationPlanet(), 0, 0));
				source.NumShips(source.NumShips() - fleetSize);
				simTarget.NumShips(simTarget.NumShips() - fleetSize);

				// This planet is ours, proceed to next target
				if (simTarget.NumShips() < 0)
				{
					successfullAttack = true;
					break;
				}
			}

			// only issue orders that definitly will conquer the target
			if (successfullAttack)
			{
				for (unsigned int j = 0, m = orders.size(); j < m; j++)
				{
					Fleet& order = orders[j];
					pw.IssueOrder(order.SourcePlanet(), order.DestinationPlanet(), order.NumShips());
				}
			}
		}
	}
	
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
