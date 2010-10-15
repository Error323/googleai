#include "PlanetWars.h"
#include "Simulator.h"
#include "Logger.h"

#include <iostream>
#include <algorithm>
#include <limits>
#include <cmath>


#define VERSION "12.0"

#define MAX_ROUNDS 200

PlanetWars* globalPW     = NULL;
int         globalTarget = 0;

bool SortOnGrowthRate(const int pidA, const int pidB) {
	const Planet& a = gAP[pidA];
	const Planet& b = gAP[pidB];
	return a.GrowthRate() > b.GrowthRate();
}

bool SortOnDistanceToTarget(const int pidA, const int pidB) {
	const Planet& t = gAP[globalTarget];
	const Planet& a = gAP[pidA];
	const Planet& b = gAP[pidB];
	const int distA = a.Distance(t);
	const int distB = b.Distance(t);
	return distA < distB;
}

bool SortOnGrowthShipRatio(const int pidA, const int pidB) {
	const Planet& a = gAP[pidA];
	const Planet& b = gAP[pidB];
	
	double growA = a.GrowthRate() / (1.0*a.NumShips() + 1.0);
	double growB = b.GrowthRate() / (1.0*b.NumShips() + 1.0);

	return growA > growB;
}

void AcceptOrRestore(bool success, Planet& target, std::vector<Fleet>& orders,
			std::vector<Planet>& AP, std::vector<Fleet>& AF) {
	if (success)
	{
		for (unsigned int j = 0, m = orders.size(); j < m; j++)
		{
			Fleet& order = orders[j];
			if (order.NumShips() > 0)
			{
				globalPW->IssueOrder(order.SourcePlanet(),
					order.DestinationPlanet(), order.NumShips());
			}
			else
			{
				LOG("ERROR: order has 0 ships - "<<order);
			}
		}
	}
	else // restore previous state
	{
		AP[target.PlanetID()].Restore();
		for (unsigned int j = 0, m = orders.size(); j < m; j++)
		{
			Fleet& order = orders[j];
			AP[order.SourcePlanet()].Restore();
		}
		AF.erase(AF.begin()+AF.size()-orders.size(), AF.end());
	}
}

void DoTurn(PlanetWars& pw, int turn) {
	std::vector<Planet> AP = pw.Planets();
	std::vector<Fleet>  AF = pw.Fleets();
	globalPW = &pw;
	gAP      = &AP;
	std::vector<int>    NPIDX;  // neutral planets
	std::vector<int>    EPIDX;  // enemy planets
	std::vector<int>    NMPIDX; // not my planets
	std::vector<int>    TPIDX;  // targetted planets belonging to us
	std::vector<int>    NTPIDX; // not targetted planets belonging to us
	std::vector<int>    EFIDX;  // enemy fleets
	std::vector<int>    MFIDX;  // my fleets

	Simulator s0;
	Simulator s1;
	s0.Start(MAX_ROUNDS-turn, AP, AF);
	int score = s0.GetScore();
	int myNumShips    = 0;
	int enemyNumShips = 0;

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
				myNumShips += p.NumShips();
			} break;

			default: {
				NMPIDX.push_back(i); 
				EPIDX.push_back(i);
				enemyNumShips += p.NumShips();
			} break;
		}
	}

	static int sDistance;
	if (turn == 0)
	{
		double x = (AP[NTPIDX[0]].X() - AP[EPIDX[0]].X());
		double y = (AP[NTPIDX[0]].Y() - AP[EPIDX[0]].Y());
		sDistance = int(round(sqrt(x*x + y*y)));
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

			const int distance = pw.Distance(tid, sid);
			s1.Start(distance, AP, AF);
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
				s0.Start(MAX_ROUNDS-turn, AP, AF);
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
				const int distance = pw.Distance(tid, sid);

				// if we are too close don't attack
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
				const int distance = pw.Distance(tid, sid);

				s1.Start(distance, AP, AF);
				const int fleetsRequired = s1.GetPlanet(tid).NumShips() + 1;
				const int fleetSize = std::min<int>(source.NumShips(), fleetsRequired);
				orders.push_back(Fleet(1, fleetSize, sid, tid, distance, distance));
				AF.push_back(orders.back());
				source.NumShips(source.NumShips() - fleetSize);

				// See if given all previous orders, this planet will be ours
				s1.Start(MAX_ROUNDS-turn, AP, AF);
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
}


// This is just the main game loop that takes care of communicating with the
// game engine for you. You don't have to understand or change the code below.
int main(int argc, char *argv[]) {
	Logger logger(std::string(argv[0]) + "-E323-" + VERSION + ".txt");
	Logger::SetLogger(&logger);

	LOG(argv[0]<<"-E323-v"<<VERSION<<" initialized");

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
