#include "PlanetWars.h"
#include "Simulator.h"
#include "Logger.h"
#include "vec3.h"
#include "Map.h"

#include <iostream>
#include <algorithm>
#include <limits>
#include <cmath>
#include <string>

#define VERSION "12.0"

#define MAX_ROUNDS 200

PlanetWars*          gPW     = NULL;
std::vector<Planet>* gAP     = NULL;
int                  gTarget = 0;

bool SortOnGrowthRate(const int pidA, const int pidB) {
	const Planet& a = gAP->at(pidA);
	const Planet& b = gAP->at(pidB);
	return a.GrowthRate() > b.GrowthRate();
}

bool SortOnDistanceToTarget(const int pidA, const int pidB) {
	const Planet& t = gAP->at(gTarget);
	const Planet& a = gAP->at(pidA);
	const Planet& b = gAP->at(pidB);
	const int distA = a.Distance(t);
	const int distB = b.Distance(t);
	return distA < distB;
}

bool SortOnGrowthShipRatio(const int pidA, const int pidB) {
	const Planet& a = gAP->at(pidA);
	const Planet& b = gAP->at(pidB);
	const double growA = a.GrowthRate() / (1.0*a.NumShips() + 1.0);
	const double growB = b.GrowthRate() / (1.0*b.NumShips() + 1.0);
	return growA > growB;
}

int GetRequiredShips(const int sid, std::vector<Fleet>& AF, std::vector<int>& EFIDX) {
	int numShipsRequired = 0;
	for (unsigned int j = 0, m = EFIDX.size(); j < m; j++)
	{
		Fleet& enemy = AF[EFIDX[j]];
		if (enemy.DestinationPlanet() == sid)
		{
			numShipsRequired += enemy.NumShips();
		}
	}
	return numShipsRequired;
}

void IssueOrders(std::vector<Fleet>& orders) {
	for (unsigned int i = 0, n = orders.size(); i < n; i++)
	{
		Fleet& order = orders[i];

		int numships = order.NumShips();
		ASSERT_MSG(numships > 0, order);

		int sid = order.SourcePlanet();
		ASSERT_MSG(gAP->at(sid).Owner() == 1, order);
		ASSERT_MSG(gAP->at(sid).NumShips() >= numships, order);

		int tid = order.DestinationPlanet();
		ASSERT_MSG(tid >= 0 && tid != sid, order);
		gPW->IssueOrder(sid, tid, numships);
	}
	orders.clear();
}

void DoTurn(PlanetWars& pw, int turn) {
	std::vector<Planet> AP = pw.Planets();
	std::vector<Fleet>  AF = pw.Fleets();
	gPW                    = &pw;
	gAP                    = &AP;
	std::vector<int>    NPIDX;  // neutral planets
	std::vector<int>    EPIDX;  // enemy planets
	std::vector<int>    NMPIDX; // not my planets
	std::vector<int>    MPIDX;  // all planets belonging to us
	std::vector<int>    TPIDX;  // targetted planets belonging to us
	std::vector<int>    NTPIDX; // not targetted planets belonging to us
	std::vector<int>    EFIDX;  // enemy fleets
	std::vector<int>    MFIDX;  // my fleets

	Simulator end, sim;
	end.Start(MAX_ROUNDS-turn, AP, AF, false, true);
	int myNumShips    = 0;
	int enemyNumShips = 0;

	for (unsigned int i = 0, n = AP.size(); i < n; i++)
	{
		Planet& p = AP[i];
		const int pid = p.PlanetID();
		switch (p.Owner())
		{
			case 0: 
			{
				NPIDX.push_back(pid);
				NMPIDX.push_back(pid); 
			} break;

			case 1: 
			{
				MPIDX.push_back(pid);
				if (p.Owner() == 1 && end.IsEnemyPlanet(pid))
					TPIDX.push_back(pid);
				else
					NTPIDX.push_back(pid);
				myNumShips += p.NumShips();
			} break;

			default: 
			{
				NMPIDX.push_back(pid); 
				EPIDX.push_back(pid);
				enemyNumShips += p.NumShips();
			} break;
		}
	}

	for (unsigned int i = 0, n = AF.size(); i < n; i++)
	{
		Fleet& f = AF[i];
		if (f.Owner() == 1)
		{
			MFIDX.push_back(i);
			myNumShips += f.NumShips();
		}
		else
		{
			EFIDX.push_back(i);
			enemyNumShips += f.NumShips();
		}
	}

	Map map(AP);
	if (turn == 0)
	{
		std::vector<Fleet> orders;
		ASSERT(!MPIDX.empty() && !EPIDX.empty());
		map.Init(MPIDX.back(), EPIDX.back(), orders);
		IssueOrders(orders);
		return;
	}
	std::vector<int>& FLPIDX = map.GetFrontLine();
	std::vector<Fleet> orders;

	LOG("(1) DEFEND");
	sort(TPIDX.begin(), TPIDX.end(), SortOnGrowthRate);
	for (unsigned int i = 0, n = TPIDX.size(); i < n; i++)
	{
		Planet& target = AP[TPIDX[i]];
		const int tid = target.PlanetID();
		
		gTarget = tid;
		bool success = false;
		sort(NTPIDX.begin(), NTPIDX.end(), SortOnDistanceToTarget);
		for (unsigned int j = 0, m = NTPIDX.size(); j < m; j++)
		{
			Planet& source = AP[NTPIDX[j]];
			int sid = source.PlanetID();

			const int distance = source.Distance(target);
			sim.Start(distance, AP, AF, false, true);
			Planet& simP = sim.GetPlanet(tid);
			source.Backup();
			while (end.IsEnemyPlanet(tid) && source.NumShips() > 0)
			{
				Simulator::PlanetOwner& enemy = end.GetFirstEnemyOwner(tid);
				int numShips;

				if (distance < enemy.time)
				{
					numShips = enemy.force - (simP.NumShips() + (enemy.time-distance)*target.GrowthRate());
				}
				else
				{
					break;
				}
				numShips = std::min<int>(source.NumShips(), numShips);

				ASSERT(numShips <= 0);
				if (numShips <= 0)
					break;

				orders.push_back(Fleet(1, numShips, sid, tid, distance, distance));
				AF.push_back(orders.back());
				source.NumShips(source.NumShips() - numShips);
				end.Start(MAX_ROUNDS-turn, AP, AF, false, true);
			}
			if (end.IsMyPlanet(tid))
			{
				success = true;
				break;
			}
		}
		if (success)
		{
			IssueOrders(orders);
		}
		else
		{
			for (unsigned int j = 0, m = orders.size(); j < m; j++)
			{
				AP[orders[j].SourcePlanet()].Restore();
			}
			AF.erase(AF.begin()+AF.size()-orders.size(), AF.end());
			orders.clear();
		}
	}

	LOG("(2) SNIPE");
	for (unsigned int i = 0, n = NPIDX.size(); i < n; i++)
	{
		Planet& target = AP[NPIDX[i]];
		const int tid = target.PlanetID();
		gTarget = tid;

		// This neutral planet will be captured by the enemy
		if (end.IsEnemyPlanet(tid))
		{
			Simulator::PlanetOwner& enemy = end.GetFirstEnemyOwner(tid);
			std::vector<Fleet> orders;
			sort(FLPIDX.begin(), FLPIDX.end(), SortOnDistanceToTarget);
			for (unsigned int j = 0, m = FLPIDX.size(); j < m; j++)
			{
				Planet& source = AP[FLPIDX[j]];
				const int sid = source.PlanetID();
				const int distance = source.Distance(target);
				if (distance == enemy.time+1)
				{
					sim.Start(distance, AP, AF, false, true);
					const int fleetsRequired = sim.GetPlanet(tid).NumShips() + 1;
					const int fleetSize = std::min<int>(source.NumShips(), fleetsRequired);
					orders.push_back(Fleet(1, fleetSize, sid, tid, distance, distance));
					AF.push_back(orders.back());
					source.NumShips(source.NumShips() - fleetSize);
				}
			}
			IssueOrders(orders);
		}
	}

	LOG("(3) ENFORCE");
	for (unsigned int i = 0, n = MPIDX.size(); i < n; i++)
	{

		Planet& source = AP[MPIDX[i]];
		const int sid = source.PlanetID();

		if (find(FLPIDX.begin(), FLPIDX.end(), sid) != FLPIDX.end())
			continue;

		const int numShips = source.NumShips() - GetRequiredShips(sid, AF, EFIDX);
		
		const int tid = map.GetClosestFrontLinePlanetIdx(source);
		if (tid != -1 && numShips > 0)
		{
			orders.push_back(Fleet(1, numShips, sid, tid, 0, 0));
		}
	}
	IssueOrders(orders);
	
	LOG("(4) ATTACK");
	for (unsigned int i = 0, n = FLPIDX.size(); i < n; i++)
	{
		Planet& source = AP[FLPIDX[i]];
		const int sid = source.PlanetID();

		int closestDist = std::numeric_limits<int>::max();
		int tid = -1;
		for (unsigned int j = 0, m = EPIDX.size(); j < m; j++)
		{
			Planet& target = AP[EPIDX[j]];
			int dist = target.Distance(source);
			if (dist < closestDist)
			{
				closestDist = dist;
				tid = target.PlanetID();
			}
		}

		const int numShips = source.NumShips() - GetRequiredShips(sid, AF, EFIDX);

		sim.Start(closestDist, AP, AF, false, true);
		if (tid != -1 && sim.GetPlanet(tid).NumShips() < numShips)
		{
			orders.push_back(Fleet(1, numShips, sid, tid, 0, 0));
		}
	}
	IssueOrders(orders);
}

// This is just the main game loop that takes care of communicating with the
// game engine for you. You don't have to understand or change the code below.
int main(int argc, char *argv[]) {
	(void) argc;
	(void) argv;

	Logger logger(std::string(argv[0]) + "-E323-" + VERSION + ".txt");
	Logger::SetLogger(&logger);

	LOG(argv[0]<<"-E323-"<<VERSION<<" initialized");

	unsigned int turn = 0;
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
				LOG("turn: " << turn);
				DoTurn(pw, turn);
				LOG("\n--------------------------------------------------------------------------------\n");
				turn++;
				pw.FinishTurn();
			} 
			else 
			{
				map_data += current_line;
			}
			current_line = "";
		}
	}
	return 0;
}
