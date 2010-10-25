#include "PlanetWars.h"
#include "Simulator.h"
#include "Logger.h"
#include "vec3.h"
#include "Map.h"
#include "KnapSack.h"

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

bool IsSafe(const Planet& target, std::vector<Planet>& ap, std::vector<Fleet>& af) {
	ASSERT(target.Owner() == 0);
	std::vector<Planet> AP(ap);
	std::vector<Fleet>  AF(af);
	Simulator sim;
	// start simulation that all ships available on all planets to this target
	for (int i = 0; i < MAX_ROUNDS-turn; i++)
	{
		for (unsigned int j = 0, m = AP.size(); j < m; j++)
		{
			Planet& source = AP[j];
			if (source.PlanetID() == target.PlanetID())
				continue;

			if (source.NumShips() <= 0)
				continue;

			if (source.Owner() == 0)
				continue;

			const int dist = target.Distance(source);
			AF.push_back(Fleet(source.Owner(), source.NumShips(),
				source.PlanetID(), target.PlanetID(), dist, dist));
			source.RemoveShips(source.NumShips());
		}
		sim.Start(1, AP, AF);
		if (sim.IsEnemyPlanet(target.PlanetID()))
			return false;
	}
	return true;
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
		ASSERT_MSG(bot::gAP->at(sid).Owner() == 1, order);
		ASSERT_MSG(tid >= 0 && tid != sid, order);
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
				if (end.IsEnemyPlanet(pid))
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
	std::vector<int>& FLPIDX = map.GetFrontLine();
	std::vector<Fleet> orders;

	// ---------------------------------------------------------------------------
	LOG("DEFEND");
	// ---------------------------------------------------------------------------
	for (unsigned int i = 0, n = TPIDX.size(); i < n; i++)
	{
		Planet& target = AP[TPIDX[i]];
		const int tid = target.PlanetID();
		bot::gTarget = tid;

		Simulator::PlanetOwner& enemy = end.GetFirstEnemyOwner(tid);
		sort(NTPIDX.begin(), NTPIDX.end(), bot::SortOnDistanceToTarget);
		bool success = false;
		for (unsigned int j = 0, m = NTPIDX.size(); j < m; j++)
		{
			Planet& source = AP[NTPIDX[j]];
			const int sid = source.PlanetID();
			if (source.NumShips() <= 0)
				continue;

			const int dist = target.Distance(source);
			if (dist > enemy.time)
				continue;

			source.Backup();
			sim.Start(enemy.time, AP, AF, false, true);
			int numShips = sim.GetPlanet(tid).NumShips();
			numShips = std::min<int>(numShips, source.NumShips()-GetRequiredShips(sid, AF, EFIDX));

			if (numShips <= 0)
				continue;

			source.RemoveShips(numShips);
			Fleet order(1, numShips, sid, tid, dist, dist);
			AF.push_back(order);
			orders.push_back(order);
			sim.Start(enemy.time, AP, AF, false, true);
			if (sim.IsMyPlanet(tid))
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
				AP[orders[j].SourcePlanet()].Restore();

			AF.erase(AF.begin() + AF.size() - orders.size(), AF.end());
			orders.clear();
		}
	}

	// ---------------------------------------------------------------------------
	LOG("SNIPE");
	// ---------------------------------------------------------------------------
	for (unsigned int i = 0, n = NPIDX.size(); i < n; i++)
	{
		Planet& target = AP[NPIDX[i]];
		const int tid = target.PlanetID();
		if (end.IsEnemyPlanet(tid))
		{
			Simulator::PlanetOwner& enemy = end.GetFirstEnemyOwner(tid);
			bot::gTarget = tid;
			sort(NTPIDX.begin(), NTPIDX.end(), bot::SortOnDistanceToTarget);
			bool success = false;
			for (unsigned int j = 0, m = NTPIDX.size(); j < m; j++)
			{
				Planet& source = AP[NTPIDX[j]];
				const int sid = source.PlanetID();
				const int dist = target.Distance(source);

				// not enough ships
				if (source.NumShips() <= 0)
					continue;

				// we don't wanna be sniped
				if (dist <= enemy.time)
					continue;

				// compute the timeframe in which we would benefit
				const int timeFrame = target.NumShips() / target.GrowthRate();
				if (dist-enemy.time >= timeFrame)
					continue;

				sim.Start(dist, AP, AF, false, true);
				int numShips =
					std::min<int>(source.NumShips()-GetRequiredShips(sid, AF,
					EFIDX), sim.GetPlanet(tid).NumShips()+1);

				if (numShips > 0)
				{
					Fleet order(1, numShips, sid, tid, dist, dist);
					orders.push_back(order);
					AF.push_back(order);
					source.Backup();
					source.RemoveShips(numShips);

					sim.Start(dist, AP, AF, false, true);
					if (sim.IsMyPlanet(tid))
					{
						success = true;
						break;
					}
				}
			}
			if (success)
			{
				IssueOrders(orders);
			}
			else
			{
				for (unsigned int j = 0, m = orders.size(); j < m; j++)
					AP[orders[j].SourcePlanet()].Restore();

				AF.erase(AF.begin() + AF.size() - orders.size(), AF.end());
				orders.clear();
			}
		}
	}

	// ---------------------------------------------------------------------------
	LOG("ATTACK");
	// ---------------------------------------------------------------------------
	// Select weakest target

	for (unsigned int i = 0, n = FLPIDX.size(); i < n; i++)
	{
		Planet& source = AP[FLPIDX[i]];
		const int sid = source.PlanetID();

		end.Start(MAX_ROUNDS-turn, AP, AF, false, true);
		int weakness = std::numeric_limits<int>::max();
		int bestTarget = -1;
		int bestDist = -1;
		for (unsigned int j = 0, m = EPIDX.size(); j < m; j++)
		{
			Planet& target = AP[EPIDX[j]];
			const int tid = target.PlanetID();

			if (end.IsMyPlanet(tid))
				continue;

			const int dist = target.Distance(source);
			int strength = GetStrength(tid, dist, EPIDX);
			if (strength < weakness)
			{
				weakness = strength;
				bestTarget = tid;
				bestDist = dist;
			}
		}
		int numShips = source.NumShips()-GetRequiredShips(sid, AF, EFIDX);
		if (numShips >= weakness)
		{
			Fleet order(1, numShips, sid, bestTarget, bestDist, bestDist);
			orders.push_back(order);
		}
	}
	IssueOrders(orders);

	// ---------------------------------------------------------------------------
	LOG("EXPAND");
	// ---------------------------------------------------------------------------
	std::vector<int> MHPIDX(NTPIDX); // planets that have spare ships
	std::vector<int> eraser;

	// while there are captureable neutrals left and planets that are able
	// to safely capture them...
	while (end.GetScore() <= 0)
	{
		orders.clear();

		if (CNPIDX.empty() || MHPIDX.empty() || EPIDX.empty())
			break;

		int totalShipsToSpare = 0;
		vec3<double> avgLoc(0.0,0.0,0.0);
		std::map<int,int> shipsToSpare;

		// compute average location of planets that can help in capturing one or
		// more neutral planets and their ships to spare
		std::vector<int> marked;
		for (unsigned int i = 0, n = MHPIDX.size(); i < n; i++)
		{
			const Planet& p  = AP[MHPIDX[i]];
			const int eid    = map.GetClosestPlanetIdx(p.Loc(), EPIDX);
			const Planet& e  = AP[eid];
			const int radius = p.Distance(e);
			const std::vector<int> DPIDX = map.GetPlanetIDsInRadius(p.Loc(), MPIDX, radius);

			// compute the number of ships we can use
			int numShips = p.NumShips() - e.NumShips();
			for (unsigned int j = 0, m = DPIDX.size(); j < m; j++)
			{
				const Planet& d = AP[DPIDX[j]];
				if (find(marked.begin(), marked.end(), d.PlanetID()) != marked.end())
					continue;

				const int dist = p.Distance(d);
				numShips += (radius-dist)*d.GrowthRate()-GetRequiredShips(d.PlanetID(), AF, EFIDX);
				marked.push_back(d.PlanetID());
			}

			numShips = std::min<int>(numShips, p.NumShips()-GetRequiredShips(p.PlanetID(), AF, EFIDX));
			if (numShips > 0)
			{
				avgLoc += p.Loc();
				totalShipsToSpare += numShips;
				shipsToSpare[p.PlanetID()] = numShips;
			}
			else
			{
				eraser.push_back(i);
			}
		}

		// erase planets that are not safe to use for capturing
		bot::Erase(MHPIDX, eraser);

		avgLoc /= MHPIDX.size();
		
		// compute weights and values of capturable neutral planets
		std::vector<int> w; std::vector<double> v;
		for (unsigned int i = 0, n = CNPIDX.size(); i < n; i++)
		{
			const Planet& p = AP[CNPIDX[i]];
			w.push_back(p.NumShips() + 1);
			v.push_back(p.GrowthRate() / (avgLoc - p.Loc()).len2D());
		}

		// compute optimal set of planets given ships to spare
		KnapSack ks(w, v, totalShipsToSpare);
		std::vector<int>& I = ks.Indices();

		// capture the planets from the knapsack
		std::vector<Fleet>  AF_(AF);
		std::vector<Planet> AP_(AP);
		Simulator sim;
		for (unsigned int i = 0, n = I.size(); i < n; i++)
		{
			const Planet& target = AP_[CNPIDX[I[i]]];

			bot::gTarget = target.PlanetID();
			sort(MHPIDX.begin(), MHPIDX.end(), bot::SortOnDistanceToTarget);
			for (unsigned int j = 0, m = MHPIDX.size(); j < m; j++)
			{
				sim.Start(MAX_ROUNDS-turn, AP_, AF_, false, true);
				const Planet& simTarget = sim.GetPlanet(target.PlanetID());

				if (sim.IsMyPlanet(target.PlanetID()) || sim.IsEnemyPlanet(target.PlanetID()))
					break;

				Planet& source = AP_[MHPIDX[j]];
				const int numShips = std::min<int>(simTarget.NumShips()+1, shipsToSpare[source.PlanetID()]);

				if (numShips <= 0)
					break;

				shipsToSpare[source.PlanetID()] -= numShips;

				const int dist = target.Distance(source);
				source.RemoveShips(numShips);
				Fleet order(1, numShips, source.PlanetID(), target.PlanetID(), dist, dist);
				orders.push_back(order);
				AF_.push_back(order);
			}
		}

		// make sure all planets that are captured are safe
		for (unsigned int i = 0, n = I.size(); i < n; i++)
		{
			const Planet& target = AP_[CNPIDX[I[i]]];
			if (!IsSafe(target, AP_, AF_))
			{
				eraser.push_back(I[i]);
			}
		}

		// optimal solution found wrt values, capture neutral planets in a greedy
		// manner wrt distance
		if (eraser.empty())
		{
			break;
		}

		// erase planets that are not safe to capture
		bot::Erase(CNPIDX, eraser);
	}
	IssueOrders(orders);

	// ---------------------------------------------------------------------------
	LOG("FEED");
	// ---------------------------------------------------------------------------
	for (unsigned int i = 0, n = NTPIDX.size(); i < n; i++)
	{
		Planet& source = AP[NTPIDX[i]];
		const int sid = source.PlanetID();
		if (find(FLPIDX.begin(), FLPIDX.end(), sid) != FLPIDX.end())
			continue;

		bot::gTarget = sid;
		sort(FLPIDX.begin(), FLPIDX.end(), bot::SortOnDistanceToTarget);
		for (unsigned int j = 0, m = FLPIDX.size(); j < m; j++)
		{
			Planet& target = AP[FLPIDX[j]];
			const int tid = target.PlanetID();
			if (sid == tid)
				continue;

			const int eid = map.GetClosestPlanetIdx(target.Loc(), EPIDX);
			Planet& enemy = AP[eid];
			const int edist = enemy.Distance(target);
			int numShips = GetStrength(eid, edist, EPIDX) - target.NumShips();
			numShips = std::min<int>(numShips, source.NumShips()-GetRequiredShips(sid, AF, EFIDX));

			if (numShips <= 0)
				continue;

			const int dist = source.Distance(target);
			
			Fleet order(1, numShips, sid, tid, dist, dist);
			AF.push_back(order);
			orders.push_back(order);
			source.RemoveShips(numShips);
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
