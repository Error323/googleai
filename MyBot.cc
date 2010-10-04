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
int         globalRemain = 0;

// sorts like this: [1 2 ... 0 -1 -2 ...]
bool SortOnDistanceToFleet(const int pidA, const int pidB) {
	int distA = globalPW->Distance(pidA, globalTarget) - globalRemain;
	int distB = globalPW->Distance(pidB, globalTarget) - globalRemain;
	if (distA > 0 && distB > 0)
		return distA < distB;
	else
		return distA > distB;
}

bool SortOnDistanceToTarget(const int pidA, const int pidB) {
	int distA = globalPW->Distance(pidA, globalTarget);
	int distB = globalPW->Distance(pidB, globalTarget);
	return distA < distB;
}

bool SortOnGrowthRate(const int pidA, const int pidB) {
	int growA = globalPW->GetPlanet(pidA).GrowthRate();
	int growB = globalPW->GetPlanet(pidB).GrowthRate();
	return growA > growB;
}

void DoTurn(PlanetWars& pw, int round, std::ofstream& file) {
	globalPW = &pw;
	std::vector<Fleet>  AF = pw.Fleets();
	std::vector<Planet> AP = pw.Planets();
	std::vector<int>    MPIDX;
	std::vector<int>    NPIDX;
	std::vector<int>    EPIDX;
	std::vector<int>    NMPIDX;

	for (unsigned int i = 0, n = AP.size(); i < n; i++)
	{
		Planet& p = AP[i];
		switch (p.Owner())
		{
			case 0:  NPIDX.push_back(i);                        break;
			case 1:  NMPIDX.push_back(i); MPIDX.push_back(i); break;
			default: NMPIDX.push_back(i); EPIDX.push_back(i); break;
		}
	}

	Simulator s(file);
	s.Start(MAX_ROUNDS-round, AP, AF);

	// (1) defend our planets
	std::vector<int> MPIDX_(MPIDX); 
	sort(MPIDX_.begin(), MPIDX_.end(), SortOnGrowthRate);
	for (unsigned int i = 0, n = MPIDX_.size(); i < n; i++)
	{
		Planet& curTarget = AP[MPIDX_[i]];
		int tid = curTarget.PlanetID();

		// This planet will be captured by the enemy
		if (s.GetPlanet(tid).Owner() > 1)
		{
			std::vector<std::pair<int,int> >& history = s.GetOwnershipHistory(tid);

			std::vector<Fleet>  orders;
			std::vector<Planet> backupAP(AP);
			std::vector<Fleet>  backupAF(AF);
			
			globalTarget = tid;
			globalRemain = history.back().second;

			Simulator s1(file);
			s1.Start(globalRemain, AP, AF);

			bool successfullAttack = false;

			sort(MPIDX.begin(), MPIDX.end(), SortOnDistanceToTarget);
			for (unsigned int j = 0, m = MPIDX.size(); j < m; j++)
			{
				Planet& source = AP[MPIDX[j]];
				int sid = source.PlanetID();

				// this is the planet in question
				if (sid == tid || source.NumShips() <= 1)
				{
					continue;
				}
				const int distance = pw.Distance(tid, sid);

				s1.Start(distance, AP, AF);
				int fleetsRequired = s1.GetPlanet(tid).NumShips() + 1;

				const int fleetSize = std::min<int>(source.NumShips() - 1, fleetsRequired);

				Fleet order(1, fleetSize, sid, tid, distance, distance);
				orders.push_back(order);
				AF.push_back(order);
				source.NumShips(source.NumShips() - fleetSize);

				// See if given all previous orders, this planet will be ours
				s1.Start(MAX_ROUNDS-round, AP, AF);
				if (s1.CapturedPlanet(tid))
				{
					successfullAttack = true;
					break;
				}
			}
			if (successfullAttack)
			{
				for (unsigned int j = 0, m = orders.size(); j < m; j++)
				{
					Fleet& order = orders[j];
					pw.IssueOrder(order.SourcePlanet(), order.DestinationPlanet(), order.NumShips());
				}
			}
			else
			{
				AP = backupAP;
				AF = backupAF;
			}
		}
	}

	// (3) overtake neutral planets captured by enemy in the future
	for (unsigned int i = 0, n = NPIDX.size(); i < n; i++)
	{
		Planet& curTarget = AP[NPIDX[i]];
		int tid = curTarget.PlanetID();

		// This neutral planet will be captured by the enemy
		if (s.GetPlanet(tid).Owner() > 1)
		{
			std::vector<std::pair<int,int> >& history = s.GetOwnershipHistory(tid);

			std::vector<Fleet>  orders;
			std::vector<Planet> backupAP(AP);
			std::vector<Fleet>  backupAF(AF);
			
			globalTarget = tid;
			globalRemain = history.back().second;

			Simulator s1(file);
			s1.Start(globalRemain, AP, AF);

			bool successfullAttack = false;

			sort(MPIDX.begin(), MPIDX.end(), SortOnDistanceToFleet);
			for (unsigned int j = 0, m = MPIDX.size(); j < m; j++)
			{
				Planet& source = AP[MPIDX[j]];
				int sid = source.PlanetID();
				const int distance = pw.Distance(tid, sid);

				// if the first planet is still too close don't attack yet
				if (j == 0 && distance <= globalRemain)
				{
					break;
				}

				// if we don't have enough ships anymore
				if (source.NumShips() <= 1)
				{
					continue;
				}

				s1.Start(distance, AP, AF);
				int fleetsRequired = s1.GetPlanet(tid).NumShips() + 1;

				const int fleetSize = std::min<int>(source.NumShips() - 1, fleetsRequired);

				Fleet order(1, fleetSize, sid, tid, distance, distance);
				orders.push_back(order);
				AF.push_back(order);
				source.NumShips(source.NumShips() - fleetSize);

				// See if given all previous orders, this planet will be ours
				s1.Start(MAX_ROUNDS-round, AP, AF);
				if (s1.CapturedPlanet(tid))
				{
					successfullAttack = true;
					break;
				}
			}
			if (successfullAttack)
			{
				for (unsigned int j = 0, m = orders.size(); j < m; j++)
				{
					Fleet& order = orders[j];
					pw.IssueOrder(order.SourcePlanet(), order.DestinationPlanet(), order.NumShips());
				}
			}
			else
			{
				AP = backupAP;
				AF = backupAF;
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
				LOG(pw.ToString());
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
