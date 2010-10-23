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

#define VERSION "12.2"

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

int GetStrength(const int tid, const int dist, std::vector<int>& EPIDX) {
	int strength = 0;
	Planet& target = gAP->at(tid);
	for (unsigned int i = 0, n = EPIDX.size(); i < n; i++)
	{
		Planet& candidate = gAP->at(EPIDX[i]);
		int distance = target.Distance(candidate);
		if (distance < dist)
		{
			int time = dist - distance;
			strength += candidate.NumShips() + time*candidate.GrowthRate();
		}
	}
	return strength;
}

void IssueOrders(std::vector<Fleet>& orders) {
	for (unsigned int i = 0, n = orders.size(); i < n; i++)
	{
		Fleet& order = orders[i];
		const int sid = order.SourcePlanet();
		const int numships = order.NumShips();
		const int tid = order.DestinationPlanet();

		ASSERT_MSG(numships > 0, order);
		ASSERT_MSG(numships <= gAP->at(sid).NumShips(), order);
		ASSERT_MSG(gAP->at(sid).Owner() == 1, order);
		ASSERT_MSG(tid >= 0 && tid != sid, order);
		LOG(order);
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
	std::vector<int>    SPIDX;  // planets sniped by the enemy
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
				if (end.IsSniped(pid))
					SPIDX.push_back(pid);
				else
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
	std::map<int, int> defenders; // <src, dst>
	for (unsigned int i = 0, n = TPIDX.size(); i < n; i++)
	{
		Planet& target = AP[TPIDX[i]];
		const int tid = target.PlanetID();
		
		Simulator::PlanetOwner& enemy = end.GetFirstEnemyOwner(tid);
		int shipsRequired = enemy.force - enemy.numships;

		gTarget = tid;
		bool success = false;
		sort(NTPIDX.begin(), NTPIDX.end(), SortOnDistanceToTarget);
		// See if we can prevent the enemy takeover by redirecting the feeders
		// to the targetted planet
		std::vector<int> FIDX; // feeder indices
		for (unsigned int j = 0, m = NTPIDX.size(); j < m; j++)
		{
			Planet& source = AP[NTPIDX[j]];
			const int sid = source.PlanetID();
			const int distance = source.Distance(target);

			// Too late
			if (distance > enemy.time)
			{
				continue;
			}

			// Don't use feeders that have a defending role already
			if (defenders.find(sid) != defenders.end())
			{
				continue;
			}

			// Don't use the frontline initially
			if (find(FLPIDX.begin(), FLPIDX.end(), sid) != FLPIDX.end())
			{
				continue;
			}

			FIDX.push_back(sid);
		}

		// Determine how long the feeders will need to be active
		for (unsigned int t = 1; t <= enemy.time; t++)
		{
			for (unsigned int j = 0, m = FIDX.size(); j < m; j++)
			{
				Planet& source = AP[FIDX[j]];
				const int sid = source.PlanetID();
				if (t == 1)
					shipsRequired -= std::min<int>(shipsRequired, source.NumShips());
				else
					shipsRequired -= std::min<int>(shipsRequired, source.GrowthRate());
				defenders[sid] = tid;
			}
			// We are able to defend this planet
			if (shipsRequired <= 0)
			{
				success = true;
				break;
			}
		}

		if (success)
		{
			continue;
		}
		else // The feeders aren't enough, let the frontline assist
		{
			sort(FLPIDX.begin(), FLPIDX.end(), SortOnDistanceToTarget);
			for (unsigned int j = 0, m = FLPIDX.size(); j < m; j++)
			{
				//TODO: Determine whether this frontline planet can and should spare the fleets
				Planet& source = AP[FLPIDX[j]];
				const int sid = source.PlanetID();
				const int distance = source.Distance(target);
				if (distance > enemy.time)
				{
					continue;
				}
				if (sid == tid)
				{
					continue;
				}
				if (find(TPIDX.begin(), TPIDX.end(), sid) != TPIDX.end())
				{
					continue;
				}

				const int numships = std::min<int>(shipsRequired, source.NumShips());
				source.Backup();
				source.NumShips(source.NumShips() - numships);
				orders.push_back(Fleet(1, numships, sid, tid, distance, distance));
				shipsRequired -= numships;
				if (shipsRequired <= 0)
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
				orders.clear();
				std::vector<int> erase;
				for (std::map<int,int>::iterator j = defenders.begin(); j != defenders.end(); j++)
				{
					if (j->second == tid)
					{
						erase.push_back(j->first);
					}
				}
				for (unsigned int j = 0, m = erase.size(); j < m; j++)
				{
					defenders.erase(erase[j]);
				}
			}
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

		// Don't enforce if we are the frontline
		if (find(FLPIDX.begin(), FLPIDX.end(), sid) != FLPIDX.end())
		{
			continue;
		}

		// If we are a target, keep our ships
		if (find(TPIDX.begin(), TPIDX.end(), sid) != TPIDX.end())
		{
			continue;
		}

		// Don't enforce frontline if we are defending
		if (defenders.find(sid) != defenders.end())
		{
			const int numShips = source.NumShips();
			const int tid = defenders[sid];
			orders.push_back(Fleet(1, numShips, sid, tid, 0, 0));
		}
		else
		{
			const int requiredShips = GetRequiredShips(sid, AF, EFIDX);
			const int numShips = source.NumShips() - requiredShips;
			const int tid = map.GetClosestFrontLinePlanetIdx(source);
			if (tid != -1 && numShips > 0)
			{
				orders.push_back(Fleet(1, numShips, sid, tid, 0, 0));
			}
		}
	}
	IssueOrders(orders);
	
	LOG("(4) CAPTURE");
	sort(NPIDX.begin(), NPIDX.end(), SortOnGrowthShipRatio);
	for (unsigned int i = 0, n = NPIDX.size(); i < n; i++)
	{
		Planet& target = AP[NPIDX[i]];
		const int tid = target.PlanetID();
		gTarget = tid;
		sort(FLPIDX.begin(), FLPIDX.end(), SortOnDistanceToTarget);
		for (unsigned int j = 0, m = FLPIDX.size(); j < m; j++)
		{
			Planet& source = AP[FLPIDX[j]];
			const int sid = source.PlanetID();
		}
	}

	LOG("(5) ATTACK");
	if (!EPIDX.empty())
	{
		for (unsigned int i = 0, n = FLPIDX.size(); i < n; i++)
		{
			Planet& source = AP[FLPIDX[i]];
			const int sid = source.PlanetID();

			gTarget = sid;
			sort(EPIDX.begin(), EPIDX.end(), SortOnDistanceToTarget);
			int tid = 0;
			int dist = 0;
			int numShipsRequired = std::numeric_limits<int>::max();
			for (unsigned int j = 0, m = EPIDX.size(); j < m; j++)
			{
				int d = source.Distance(AP[EPIDX[j]]);
				sim.Start(d, AP, AF, false, true);
				int s = GetStrength(EPIDX[j], d, EPIDX) + sim.GetPlanet(tid).NumShips();
				if (s < numShipsRequired)
				{
					tid = EPIDX[j];
					numShipsRequired = s;
					dist = d;
				}
			}

			// If we are a target, and our growthrate is better, keep the ships
			if (find(TPIDX.begin(), TPIDX.end(), sid) != TPIDX.end())
			{
				if (AP[tid].GrowthRate() < source.GrowthRate())
				{
					continue;
				}
			}

			end.Start(MAX_ROUNDS-turn, AP, AF, false, true);
			if (end.IsMyPlanet(tid))
			{
				continue;
			}

			const int numShips = source.NumShips() - GetRequiredShips(sid, AF, EFIDX);

			if (numShipsRequired < numShips)
			{
				orders.push_back(Fleet(1, numShipsRequired, sid, tid, dist, dist));
			}
		}
		IssueOrders(orders);
	}
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
				LOG(pw.ToString());
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
