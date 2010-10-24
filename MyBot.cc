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

#define VERSION "13.0"

PlanetWars* gPW      = NULL;
int turn             = 0;
int MAX_ROUNDS       = 200;

namespace bot {
	#include "Helper.inl"
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
	const Planet& target = bot::gAP->at(tid);
	for (unsigned int i = 0, n = EPIDX.size(); i < n; i++)
	{
		const Planet& candidate = bot::gAP->at(EPIDX[i]);
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
		ASSERT_MSG(numships <= bot::gAP->at(sid).NumShips(), order);
		ASSERT_MSG(bot::gAP->at(sid).Owner() == 1, order);
		ASSERT_MSG(tid >= 0 && tid != sid, order);
		LOG(order);
		gPW->IssueOrder(sid, tid, numships);
	}
	orders.clear();
}

void DoTurn(PlanetWars& pw) {
	std::vector<Planet> AP = pw.Planets();
	std::vector<Fleet>  AF = pw.Fleets();
	gPW                    = &pw;
	bot::gAP               = &AP;
	std::vector<int>    NPIDX;  // neutral planets
	std::vector<int>    CNPIDX; // neutral planets that we can capture
	std::vector<int>    EPIDX;  // enemy planets
	std::vector<int>    NMPIDX; // not my planets
	std::vector<int>    MPIDX;  // all planets belonging to us
	std::vector<int>    SPIDX;  // planets sniped by the enemy
	std::vector<int>    TPIDX;  // targetted planets belonging to us
	std::vector<int>    NTPIDX; // not targetted planets belonging to us
	std::vector<int>    EFIDX;  // enemy fleets
	std::vector<int>    MFIDX;  // my fleets

	Simulator end;
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
				if (end.GetPlanet(pid).Owner() == 0)
					CNPIDX.push_back(pid);

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
	std::vector<Fleet> orders;

	Map map(AP);
	map.GetOrdersForCaptureableNeutrals(AF, orders, CNPIDX);
	IssueOrders(orders);
	std::vector<int>& FLPIDX = map.GetFrontLine();
}

// This is just the main game loop that takes care of communicating with the
// game engine for you. You don't have to understand or change the code below.
int main(int argc, char *argv[]) {
	(void) argc;
	(void) argv;

	Logger logger(std::string(argv[0]) + "-E323-" + VERSION + ".txt");
	Logger::SetLogger(&logger);

	LOG(argv[0]<<"-E323-"<<VERSION<<" initialized");

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
				DoTurn(pw);
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
