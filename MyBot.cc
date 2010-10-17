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

void IssueOrders(std::vector<Fleet>& orders) {
	for (unsigned int i = 0, n = orders.size(); i < n; i++)
	{
		Fleet& order = orders[i];

		int numships = order.NumShips();
		ASSERT_MSG(numships > 0, order);

		int sid = order.SourcePlanet();
		ASSERT_MSG(gAP->at(sid).Owner() == 1, order);

		int tid = order.DestinationPlanet();
		gPW->IssueOrder(sid, tid, numships);
	}
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
	int score = myNumShips - enemyNumShips;

	Map map(AP);
	if (turn == 0)
	{
		std::vector<Fleet> orders;
		ASSERT(!MPIDX.empty() && !EPIDX.empty());
		map.Init(MPIDX.back(), EPIDX.back(), orders);
		IssueOrders(orders);
		return;
	}

	LOG("(1) DEFENSE");

	LOG("(2) SNIPE");
	/*
	std::vector<int>& FLPIDX = map.GetFrontLine();
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
				if (source.NumShips() <= 0)
					continue;

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
	*/

	if (score < 0)
	{
		LOG("(3.1) LOSING");
	}
	else
	if (score > 0)
	{
		LOG("(3.2) WINNING");
	}
	else
	{
		LOG("(3.3) DRAWING");
	}
}

/*
	// (1) defend our planets which are to be captured in the future
	LOG("(1) DEFENSE");
	sort(TPIDX.begin(), TPIDX.end(), SortOnGrowthRate);
	for (unsigned int i = 0, n = TPIDX.size(); i < n; i++)
	{
		Planet& target = AP[TPIDX[i]];
		const int tid = target.PlanetID();
		std::vector<Fleet>  orders;
		
		globalTarget = tid;
		bool successfullAttack = false;
		sort(NTPIDX.begin(), NTPIDX.end(), SortOnDistanceToTarget);
		for (unsigned int j = 0, m = NTPIDX.size(); j < m; j++)
		{
			Planet& source = AP[NTPIDX[j]];
			if (source.NumShips() <= 0)
				continue;
			source.Backup();
			int sid = source.PlanetID();

			const int distance = source.Distance(target);
			s1.Start(distance, AP, AF, false, true);
			Planet& s1P = s1.GetPlanet(tid);
			while (s0.IsEnemyPlanet(tid) && source.NumShips() > 0)
			{
				Simulator::PlanetOwner& enemy = s0.GetFirstEnemyOwner(tid);
				int numShips;

				if (distance < enemy.time)
				{
					numShips = enemy.force - (target.NumShips() + enemy.time*target.GrowthRate());
					LOG("distance < enemy.time, ships: "<<numShips);
				}
				else
				if (distance > enemy.time)
				{
					numShips = s1P.NumShips() + 1;
					LOG("distance > enemy.time, ships: "<<numShips);
				}
				else
				{
					numShips = enemy.force;
					LOG("distance = enemy.time, ships: "<<numShips);
				}

				// FIXME: This should not happen...
				if (numShips <= 0) break;
				numShips = std::min<int>(source.NumShips(), numShips);
				orders.push_back(Fleet(1, numShips, sid, tid, distance, distance));
				AF.push_back(orders.back());
				source.NumShips(source.NumShips() - numShips);
				s0.Start(MAX_ROUNDS-turn, AP, AF, false, true);
			}
			if (s0.IsMyPlanet(tid))
			{
				successfullAttack = true;
				break;
			}
		}
		AcceptOrRestore(successfullAttack, target, orders, AP, AF);
	}
	
	// (2) overtake neutral planets captured by enemy in the future
	LOG("(2) ANNOY");
	for (unsigned int i = 0, n = NPIDX.size(); i < n; i++)
	{
		Planet& target = AP[NPIDX[i]];
		int tid = target.PlanetID();

		// This neutral planet will be captured by the enemy
		if (s0.IsEnemyPlanet(tid))
		{
			std::vector<Fleet>  orders;
			
			globalTarget = tid;
			Simulator::PlanetOwner& enemy = s0.GetFirstEnemyOwner(tid);
			bool successfullAttack = false;
			sort(NTPIDX.begin(), NTPIDX.end(), SortOnDistanceToTarget);
			for (unsigned int j = 0, m = NTPIDX.size(); j < m; j++)
			{
				Planet& source = AP[NTPIDX[j]];
				if (source.NumShips() <= 0)
					continue;

				source.Backup();
				int sid = source.PlanetID();
				const int distance = source.Distance(target);

				// if we are too close don't attack
				if (distance <= enemy.time)
					break;

				s1.Start(distance, AP, AF, false, true);
				const int fleetsRequired = s1.GetPlanet(tid).NumShips() + 1;
				const int fleetSize = std::min<int>(source.NumShips(), fleetsRequired);
				orders.push_back(Fleet(1, fleetSize, sid, tid, distance, distance));
				AF.push_back(orders.back());
				source.NumShips(source.NumShips() - fleetSize);

				// See if given all previous orders, this planet will be ours
				s1.Start(distance, AP, AF, false, true);
				if (s1.IsMyPlanet(tid))
				{
					successfullAttack = true;
					break;
				}
			}
			AcceptOrRestore(successfullAttack, target, orders, AP, AF);
		}
	}

	// (3) annihilate when we are definitly winning
	LOG("(3) ANNIHILATE");
	if (myNumShips > enemyNumShips*2)
	{
		sort(NMPIDX.begin(), NMPIDX.end(), SortOnGrowthShipRatio);
		for (unsigned int i = 0, n = NMPIDX.size(); i < n; i++)
		{
			Planet& target = AP[NMPIDX[i]];
			int tid = target.PlanetID();

			std::vector<Fleet> orders;
			
			globalTarget = tid;
			bool successfullAttack = false;
			sort(NTPIDX.begin(), NTPIDX.end(), SortOnDistanceToTarget);
			for (unsigned int j = 0, m = NTPIDX.size(); j < m; j++)
			{
				Planet& source = AP[NTPIDX[j]];
				if (source.NumShips() <= 0)
					continue;
				source.Backup();
				int sid = source.PlanetID();
				const int distance = source.Distance(target);

				s1.Start(distance, AP, AF, false, true);
				const int fleetsRequired = s1.GetPlanet(tid).NumShips() + 1;
				const int fleetSize = std::min<int>(source.NumShips(), fleetsRequired);
				orders.push_back(Fleet(1, fleetSize, sid, tid, distance, distance));
				AF.push_back(orders.back());
				source.NumShips(source.NumShips() - fleetSize);

				// See if given all previous orders, this planet will be ours
				s1.Start(MAX_ROUNDS-turn, AP, AF, false, true);
				if (s1.GetScore() >= score)
				{
					score = s1.GetScore();
					successfullAttack = true;
					break;
				}
			}
			AcceptOrRestore(successfullAttack, target, orders, AP, AF);
		}
	}
	*/


// This is just the main game loop that takes care of communicating with the
// game engine for you. You don't have to understand or change the code below.
int main(int argc, char *argv[]) {
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
