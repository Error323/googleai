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
#include <queue>

#ifdef DEBUG
	#include <sys/types.h>
	#include <unistd.h>
	#include <signal.h>
#endif


#define VERSION "13.0"

PlanetWars* gPW = NULL;
int turn        = 0;
int MAX_TURNS   = 200;

namespace bot {
	#include "Helper.inl"
}

struct NPV {
	NPV(): id(-1), val(-1.0) {}
	NPV(int pid, double v): id(pid), val(v) {}
	int id;
	double val;
	bool operator< (const NPV& npv) const {
		return val < npv.val;
	}
};

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
	std::vector<int> NPIDX;  // neutral planets
	std::vector<int> EPIDX;  // enemy planets
	std::vector<int> TPIDX;  // targetted planets belonging to us
	std::vector<int> NTPIDX; // not targetted planets belonging to us
	std::vector<int> EFIDX;  // enemy fleets
	std::vector<int> MFIDX;  // my fleets

	Simulator end, sim;
	end.Start(MAX_TURNS-turn, AP, AF, false, true);
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
					TPIDX.push_back(pid);
				else if (!end.IsMyPlanet(pid))
					NPIDX.push_back(pid);
			} break;

			case 1: 
			{
				if (end.IsEnemyPlanet(pid))
					TPIDX.push_back(pid);
				else
					NTPIDX.push_back(pid);
				myNumShips += p.NumShips();
			} break;

			default: 
			{
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
	LOG("DEFEND"); // defend our planets when possible
	// ---------------------------------------------------------------------------
	sort(TPIDX.begin(), TPIDX.end(), bot::SortOnDistanceToEnemy);
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
	LOG("SNIPE"); // overtake neutral planets captured by the enemy RLLY CLOSE
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

				// only snipe when we are the closest planet around
				std::vector<int> EPIRIDX = map.GetPlanetIDsInRadius(target.Loc(), EPIDX, dist);
				if (end.GetScore() < 0 && !EPIRIDX.empty())
					continue;

				// compute the timeframe in which we would benefit
				const int timeFrame = target.NumShips() / target.GrowthRate();
				if (dist-enemy.time >= timeFrame)
					continue;

				sim.Start(dist, AP, AF, false, true);
				int numShips =
					std::min<int>(source.NumShips()-GetRequiredShips(sid, AF,
					EFIDX), sim.GetPlanet(tid).NumShips() + 1);

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
	LOG("ATTACK"); // rage upon our enemy
	// ---------------------------------------------------------------------------
	if (end.GetScore() >= 0 && !EPIDX.empty())
	{
		for (unsigned int i = 0, n = FLPIDX.size(); i < n; i++)
		{
			Planet& source = AP[FLPIDX[i]];
			const int sid = source.PlanetID();

			end.Start(MAX_TURNS-turn, AP, AF, false, true);
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
			if (bestTarget != -1)
			{
				int numShips = source.NumShips()-GetRequiredShips(sid, AF, EFIDX);
				sim.Start(bestDist, AP, AF, false, true);
				if (numShips > 0 && numShips > sim.GetPlanet(bestTarget).NumShips())
				{
					Fleet order(1, numShips, sid, bestTarget, bestDist, bestDist);
					source.RemoveShips(numShips);
					orders.push_back(order);
					AF.push_back(order);
				}
			}
		}
		IssueOrders(orders);
	}

	// ---------------------------------------------------------------------------
	LOG("EXPAND"); // when we are losing or drawing
	// ---------------------------------------------------------------------------
	end.Start(MAX_TURNS-turn, AP, AF, false, true);
	if (end.GetScore() <= 0)
	{
		// 1. Compute the ships to spare wrt closest enemy
		std::map<int, int> numShipsToSpare;
		std::vector<int> MHPIDX; // planets that have ships to spare
		vec3<double> avgLoc(0.0,0.0,0.0);
		int totalNumShipsToSpare = 0;
		for (unsigned int i = 0, n = NTPIDX.size(); i < n; i++)
		{
			Planet& p = AP[NTPIDX[i]];
			const int pid = p.PlanetID();
			const int eid = map.GetClosestPlanetIdx(p.Loc(), EPIDX);
			Planet& e = AP[eid];
			const int dist = p.Distance(e);
			int numShips = p.NumShips() - e.NumShips() -
				GetRequiredShips(pid, AF, EFIDX) + dist*p.GrowthRate();

			numShips = std::min<int>(numShips, p.NumShips());
			if (numShips > 0 && p.NumShips() > 0)
			{
				numShipsToSpare[pid] = numShips;
				totalNumShipsToSpare += numShips;
				MHPIDX.push_back(pid);
				avgLoc += p.Loc();
			}
		}
		avgLoc /= MHPIDX.size();

		// 2. Filter out candidates wrt numships and enemy
		std::vector<int> candidates;
		for (unsigned int i = 0, n = NPIDX.size(); i < n; i++)
		{
			Planet& target = AP[NPIDX[i]];
			if (target.NumShips() + 1 > totalNumShipsToSpare)
				continue;

			const int eid = map.GetClosestPlanetIdx(target.Loc(), EPIDX);
			Planet& e = AP[eid];
			const int edist2target = target.Distance(e);
			std::vector<int> PIRIDX = map.GetPlanetIDsInRadius(target.Loc(), MHPIDX, edist2target);
			bot::gTarget = target.PlanetID();
			sort(PIRIDX.begin(), PIRIDX.end(), bot::SortOnDistanceToTarget);
			for (unsigned int j = 0, m = PIRIDX.size(); j < m; j++)
			{
				Planet& source = AP[PIRIDX[j]];
				const int mdist2target = target.Distance(source);
				const bool closer = mdist2target <= edist2target;
				const bool defend = source.NumShips() - GetRequiredShips(source.PlanetID(), AF, EFIDX) > 0;
				if (closer && defend)
				{
					candidates.push_back(target.PlanetID());
					break;
				}
			}
		}

		// 3. Apply binary knapsack algorithm to select the best candidates
		std::vector<int> w; std::vector<double> v;
		std::priority_queue<NPV> PQ;
		for (unsigned int i = 0, n = candidates.size(); i < n; i++)
		{
			Planet& candidate = AP[candidates[i]];
			w.push_back(candidate.NumShips() + 1);
			//double value = candidate.GrowthRate() / (avgLoc - candidate.Loc()).len2D();
			const int dist = (avgLoc - candidate.Loc()).len2D();
			double value = std::numeric_limits<double>::max() / (candidate.NumShips() / (candidate.GrowthRate() + 1.0) + log(pow(2, dist)));
			v.push_back(value);
			PQ.push(NPV(candidate.PlanetID(), value));
		}

		if (!candidates.empty() && !MHPIDX.empty())
		{
			// use the knapsack algorithm on turn 0
			if (turn == 0)
			{
				KnapSack ks(w, v, totalNumShipsToSpare);
				std::vector<int> I = ks.Indices();
				for (unsigned int i = 0, n = I.size(); i < n; i++)
				{
					Planet& target = AP[candidates[I[i]]];
					const int tid = target.PlanetID();
					bot::gTarget = tid;
					bool success = true;
					sort(MHPIDX.begin(), MHPIDX.end(), bot::SortOnDistanceToTarget);
					for (unsigned int j = 0, m = MHPIDX.size(); j < m; j++)
					{
						Planet& source = AP[MHPIDX[j]];
						const int sid = source.PlanetID();
						const int dist = target.Distance(source);
						int numShips = std::min<int>(target.NumShips() + 1, numShipsToSpare[sid]);
						numShips = std::min<int>(numShips, source.NumShips() - GetRequiredShips(sid, AF, EFIDX));
						if (numShips <= 0 || source.NumShips() <= 0)
							continue;

						sim.Start(dist, AP, AF, false, true);
						numShips = std::min<int>(sim.GetPlanet(tid).NumShips() + 1, numShips);
						numShipsToSpare[sid] -= numShips;
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

			// otherwise cap the best neutral if possible
			else
			{
				Planet& target = AP[PQ.top().id];
				const int tid = target.PlanetID();
				if (totalNumShipsToSpare >= target.NumShips() + 1)
				{
					bool success = false;
					bot::gTarget = tid;
					sort(MHPIDX.begin(), MHPIDX.end(), bot::SortOnDistanceToTarget);
					for (unsigned int i = 0, n = MHPIDX.size(); i < n; i++)
					{
						Planet& source = AP[MHPIDX[i]];
						const int sid = source.PlanetID();
						const int dist = target.Distance(source);
						sim.Start(dist, AP, AF, false, true);
						int numShips = std::min<int>(sim.GetPlanet(tid).NumShips() + 1, numShipsToSpare[sid]);
						numShips = std::min<int>(numShips, source.NumShips() - GetRequiredShips(sid, AF, EFIDX));
						if (numShips <= 0)
							continue;

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
		}
	}

	// ---------------------------------------------------------------------------
	LOG("FEED"); // support the frontline
	// ---------------------------------------------------------------------------
	std::vector<Planet> _AP(AP);
	std::vector<Fleet>  _AF(AF);
	end.Start(MAX_TURNS-turn, _AP, _AF);
	Map _map(_AP);
	std::vector<int>& _FLPIDX = _map.GetFrontLine();
	for (unsigned int i = 0, n = NTPIDX.size(); i < n; i++)
	{
		Planet& source = AP[NTPIDX[i]];
		const int sid = source.PlanetID();
		if (find(_FLPIDX.begin(), _FLPIDX.end(), sid) != _FLPIDX.end())
			continue;

		bot::gTarget = sid;
		sort(_FLPIDX.begin(), _FLPIDX.end(), bot::SortOnDistanceToTarget);
		for (unsigned int j = 0, m = _FLPIDX.size(); j < m; j++)
		{
			Planet& target = AP[_FLPIDX[j]];
			const int tid = target.PlanetID();
			const int eid = map.GetClosestPlanetIdx(target.Loc(), EPIDX);
			Planet& enemy = AP[eid];
			const int edist = enemy.Distance(target);
			int numShips = GetStrength(eid, edist, EPIDX) - target.NumShips();
			numShips = std::min<int>(numShips, source.NumShips()-GetRequiredShips(sid, AF, EFIDX));

			if (numShips <= 0 || source.NumShips() <= 0)
				continue;

			const int dist = source.Distance(target);
			
			Fleet order(1, numShips, sid, tid, dist, dist);
			AF.push_back(order);
			orders.push_back(order);
			source.RemoveShips(numShips);
		}
	}
	IssueOrders(orders);

	for (unsigned int i = 0, n = NTPIDX.size(); i < n; i++)
	{
		Planet& source = AP[NTPIDX[i]];
		const int sid = source.PlanetID();
		if (find(_FLPIDX.begin(), _FLPIDX.end(), sid) != _FLPIDX.end())
			continue;

		const int tid = map.GetClosestPlanetIdx(source.Loc(), _FLPIDX);
		if (tid == -1)
			continue;

		Planet& target = AP[tid];
		const int dist = target.Distance(source);
		const int numShips = source.NumShips()-GetRequiredShips(sid, AF, EFIDX);
		if (numShips <= 0)
			continue;

		Fleet order(1, numShips, sid, tid, dist, dist);
		AF.push_back(order);
		orders.push_back(order);
		source.RemoveShips(numShips);
	}
	IssueOrders(orders);
}

#ifdef DEBUG
void SigHandler(int signum) {
	Logger logger(std::string(VERSION) + "-crash.txt");
	logger.Log(gPW->ToString());
	signal(signum, SIG_DFL);
	kill(getpid(), signum);
}
#endif

// This is just the main game loop that takes care of communicating with the
// game engine for you. You don't have to understand or change the code below.
int main(int argc, char *argv[]) {
	(void) argc;
	(void) argv;

#ifdef DEBUG
	signal(SIGSEGV, SigHandler);
	Logger logger(std::string(argv[0]) + ".txt");
	Logger::SetLogger(&logger);
	LOG(argv[0]<<" initialized");
#endif

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
