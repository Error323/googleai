#include "PlanetWars.h"
#include "Simulator.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <limits>
#include <cassert>
#include <cmath>
#include <utility>


#define VERSION "9.0"

#define MAX_ROUNDS 200

PlanetWars* globalPW     = NULL;
int         globalTarget = 0;
int         globalRemain = 0;

bool SortOnGrowthRate(const int pidA, const int pidB) {
	const Planet& a = globalPW->GetPlanet(pidA);
	const Planet& b = globalPW->GetPlanet(pidB);
	return a.GrowthRate() > b.GrowthRate();
}

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

bool SortOnGrowthShipRatio(const int pidA, const int pidB) {
	const Planet& a = globalPW->GetPlanet(pidA);
	const Planet& b = globalPW->GetPlanet(pidB);
	
	double growA = a.GrowthRate() / (1.0*a.NumShips() + 1.0);
	double growB = b.GrowthRate() / (1.0*b.NumShips() + 1.0);

	return growA > growB;
}

void DoTurn(PlanetWars& pw, int round, std::ofstream& file) {
	globalPW = &pw;
	std::vector<Planet> AP = pw.Planets();
	std::vector<Fleet>  AF = pw.Fleets();
	std::vector<int>    NPIDX;  // neutral planets
	std::vector<int>    EPIDX;  // enemy planets
	std::vector<int>    NMPIDX; // not my planets
	std::vector<int>    TPIDX;  // targetted planets belonging to us
	std::vector<int>    NTPIDX; // not targetted planets belonging to us
	std::vector<int>    EFIDX;  // enemy fleets
	std::vector<int>    MFIDX;  // my fleets

	Simulator s0(file, 0);
	Simulator s1(file, 1);
	s0.Start(MAX_ROUNDS-round, AP, AF);
	int score = s0.GetScore();

	for (unsigned int i = 0, n = AP.size(); i < n; i++)
	{
		Planet& p = AP[i];
		switch (p.Owner())
		{
			case 0: {
				NPIDX.push_back(i);
				NMPIDX.push_back(i); 
			} break;

			case 1: {
				if (p.Owner() == 1 && s0.IsEnemyPlanet(p.PlanetID()))
					TPIDX.push_back(p.PlanetID());
				else
					NTPIDX.push_back(p.PlanetID());
			} break;

			default: {
				NMPIDX.push_back(i); 
				EPIDX.push_back(i);
			} break;
		}
	}

	for (unsigned int i = 0, n = AF.size(); i < n; i++)
	{
		Fleet& f = AF[i];
		if (f.Owner() == 1)
			MFIDX.push_back(i);
		else
			EFIDX.push_back(i);
	}


	// (1) defend our planets which are to be captured in the future
	sort(TPIDX.begin(), TPIDX.end(), SortOnGrowthRate);
	for (unsigned int i = 0, n = TPIDX.size(); i < n; i++)
	{
		Planet& curTarget = AP[TPIDX[i]];
		const int tid = curTarget.PlanetID();
		std::vector<Fleet>  orders;
		curTarget.Backup();
		
		globalTarget = tid;
		bool successfullAttack = false;
		LOG(curTarget);
		sort(NTPIDX.begin(), NTPIDX.end(), SortOnDistanceToTarget);
		for (unsigned int j = 0, m = NTPIDX.size(); j < m; j++)
		{
			Planet& source = AP[NTPIDX[j]];
			source.Backup();
			int sid = source.PlanetID();

			// if we don't have enough ships anymore
			if (source.NumShips() <= 0)
			{
				continue;
			}

			const int distance = pw.Distance(tid, sid);
			s1.Start(distance, AP, AF);
			Planet& s1P = s1.GetPlanet(tid);
			while (s0.IsEnemyPlanet(tid) && source.NumShips() > 0)
			{
				Simulator::PlanetOwner& enemy = s0.GetFirstEnemyOwner(tid);
				int numShips;
				if (distance < enemy.time)
				{
					LOG("distance < enemy.time\t"<<enemy.time<<", "<<enemy.force);
					numShips = enemy.force - (curTarget.NumShips() + enemy.time*curTarget.GrowthRate());
				}
				else
				if (distance > enemy.time)
				{
					LOG("distance > enemy.time");
					numShips = s1P.NumShips() + 1;
				}
				else
				{
					LOG("distance = enemy.time");
					numShips = enemy.force;
				}
				numShips = std::min<int>(source.NumShips(), numShips);
				orders.push_back(Fleet(1, numShips, sid, tid, distance, distance));
				AF.push_back(orders.back());
				source.NumShips(source.NumShips() - numShips);
				s0.Start(MAX_ROUNDS-round, AP, AF);
			}
			if (s0.IsMyPlanet(tid))
			{
				successfullAttack = true;
				score = s0.GetScore();
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
		else // restore previous state
		{
			AP[curTarget.PlanetID()].Restore();
			for (unsigned int j = 0, m = orders.size(); j < m; j++)
			{
				Fleet& order = orders[j];
				AP[order.SourcePlanet()].Restore();
			}
			AF.erase(AF.begin()+AF.size()-orders.size(), AF.end());
		}
	}
	
	// (2) overtake neutral planets captured by enemy in the future
	for (unsigned int i = 0, n = NPIDX.size(); i < n; i++)
	{
		Planet& curTarget = AP[NPIDX[i]];
		int tid = curTarget.PlanetID();

		// This neutral planet will be captured by the enemy
		if (s0.IsEnemyPlanet(tid))
		{
			std::vector<Fleet>  orders;
			curTarget.Backup();
			
			globalTarget = tid;
			Simulator::PlanetOwner& enemy = s0.GetFirstEnemyOwner(tid);
			bool successfullAttack = false;
			sort(NTPIDX.begin(), NTPIDX.end(), SortOnDistanceToTarget);
			for (unsigned int j = 0, m = NTPIDX.size(); j < m; j++)
			{
				Planet& source = AP[NTPIDX[j]];
				source.Backup();
				int sid = source.PlanetID();
				const int distance = pw.Distance(tid, sid);
				LOG(distance<<" | "<<enemy.owner<<"\t"<<enemy.time<<"\t"<<enemy.force);

				// if we are still too close don't attack
				if (distance <= enemy.time)
					break;

				s1.Start(distance, AP, AF);
				const int fleetsRequired = s1.GetPlanet(tid).NumShips() + 1;
				const int fleetSize = std::min<int>(source.NumShips(), fleetsRequired);
				orders.push_back(Fleet(1, fleetSize, sid, tid, distance, distance));
				AF.push_back(orders.back());
				source.NumShips(source.NumShips() - fleetSize);

				// See if given all previous orders, this planet will be ours
				s1.Start(distance, AP, AF);
				if (s1.IsMyPlanet(tid))
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
			else // restore previous state
			{
				AP[curTarget.PlanetID()].Restore();
				for (unsigned int j = 0, m = orders.size(); j < m; j++)
				{
					Fleet& order = orders[j];
					AP[order.SourcePlanet()].Restore();
				}
				AF.erase(AF.begin()+AF.size()-orders.size(), AF.end());
			}
		}
	}

/*
	// (3) capture good planets
	s0.Start(MAX_ROUNDS-round, AP, AF);
	sort(NMPIDX.begin(), NMPIDX.end(), SortOnGrowthShipRatio);
	for (unsigned int i = 0, n = NMPIDX.size(); i < n; i++)
	{
		Planet& curTarget = AP[NMPIDX[i]];
		const int tid = curTarget.PlanetID();
		if (s0.IsMyPlanet(tid) || (curTarget.Owner() == 0 && s0.IsEnemyPlanet(tid)))
		{
			continue;
		}

		std::vector<Fleet>  orders;
		curTarget.Backup();
		
		globalTarget = tid;
		bool successfullAttack = false;

		sort(NTPIDX.begin(), NTPIDX.end(), SortOnDistanceToTarget);
		for (unsigned int j = 0, m = NTPIDX.size(); j < m; j++)
		{
			Planet& source = AP[NTPIDX[j]];
			source.Backup();
			const int sid = source.PlanetID();
			const int distance = pw.Distance(tid, sid);

			// if we don't have enough ships anymore
			if (source.NumShips() <= 0)
			{
				continue;
			}

			// if this planet has more value and is about to be captured, don't use it
			if (source.GrowthRate() >= curTarget.GrowthRate() && s0.IsEnemyPlanet(sid))
			{
				continue;
			}

			s1.Start(distance, AP, AF);
			int fleetsRequired = s1.GetPlanet(tid).NumShips() + 1;

			const int fleetSize = std::min<int>(source.NumShips(), fleetsRequired);

			Fleet order(1, fleetSize, sid, tid, distance, distance);
			orders.push_back(order);
			AF.push_back(order);
			source.NumShips(source.NumShips() - fleetSize);

			// See if given all previous orders, this planet will be ours
			s1.Start(MAX_ROUNDS-round, AP, AF);
			int tmpScore = s1.GetScore();
			if (s1.IsMyPlanet(tid))
			{
				successfullAttack = true;
				score = tmpScore;
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
		else // restore previous state
		{
			AP[curTarget.PlanetID()].Restore();
			for (unsigned int j = 0, m = orders.size(); j < m; j++)
			{
				Fleet& order = orders[j];
				AP[order.SourcePlanet()].Restore();
			}
			AF.erase(AF.begin()+AF.size()-orders.size(), AF.end());
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
