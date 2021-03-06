#include "PlanetWars.h"
#include "Simulator.h"
#include "Logger.h"
#include "vec3.h"
#include "Map.h"
#include "KnapSack.h"
#include "Timer.h"

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

PlanetWars* gPW = NULL;
int turn        = 0;
int MAX_TURNS   = 200;

namespace bot {
	#include "Helper.inl"
}

bool Defend(int tid, std::vector<Planet>& AP, std::vector<Fleet>& AF,
				std::vector<int>& NTPIDX, std::vector<int>& EFIDX,
				std::vector<Fleet>& orders, bool restore) {

	Simulator end, sim;
	end.Start(MAX_TURNS-turn, AP, AF, false, true);
	Simulator::PlanetOwner& enemy = end.GetFirstEnemyOwner(tid);
	Planet& target = AP[tid];
	bot::gTarget = tid;
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
		numShips = std::min<int>(numShips, source.NumShips()-bot::GetIncommingFleets(sid, EFIDX));

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
	if (!success || restore)
	{
		for (unsigned int j = 0, m = orders.size(); j < m; j++)
			AP[orders[j].SourcePlanet()].Restore();

		AF.erase(AF.begin() + AF.size() - orders.size(), AF.end());
		orders.clear();
	}
	return success;
}

bool Attack(Map& map, std::vector<int>& EPIDX, int sid, int tid, std::vector<Planet>& AP, std::vector<Fleet>& AF,
				std::vector<int>& EFIDX, std::vector<Fleet>& orders, bool restore) {

	Simulator sim;
	bool canAttack = false;
	Planet& source = AP[sid];
	Planet& target = AP[tid];
	const int dist = source.Distance(target);
	sim.Start(dist, AP, AF, false, true);
	int numShipsRequired = sim.GetPlanet(tid).NumShips();
	int numShips = source.NumShips()-bot::GetIncommingFleets(sid, EFIDX);
	canAttack = numShips > numShipsRequired;

	if (canAttack)
	{
		int eid = map.GetClosestPlanetIdx(source.Loc(), EPIDX);
		if (eid != tid)
		{
			const int time = source.Distance(AP[eid]);
			numShips -= AP[eid].NumShips() - time*source.GrowthRate();
			numShips = std::min<int>(source.NumShips()-bot::GetIncommingFleets(sid, EFIDX), numShips);
			canAttack = numShips > numShipsRequired;
		}
	}

	if (canAttack)
	{
		Fleet order(1, numShips, sid, tid, dist, dist);
		source.Backup();
		source.RemoveShips(numShips);
		orders.push_back(order);
		AF.push_back(order);
	}

	if (canAttack && restore)
	{
		source.Restore();
		AF.pop_back();
		orders.pop_back();
	}

	return canAttack;
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
	bot::gAP               = &AP; // all planets
	bot::gAF               = &AF; // all fleets
	std::vector<int> NPIDX;  // neutral planets
	std::vector<int> EPIDX;  // enemy planets
	std::vector<int> TPIDX;  // targetted planets belonging to us
	std::vector<int> NTPIDX; // not targetted planets belonging to us
	std::vector<int> EFIDX;  // enemy fleets
	std::vector<int> MFIDX;  // my fleets

	Simulator end, sim;
#ifdef DEBUG
	sim.Start(0, AP, AF, false, true);
	LOG("SCORE: "<<sim.GetScore());
#endif
	end.Start(MAX_TURNS-turn, AP, AF, false, true);

	vec3<double> mAvgLoc(0.0, 0.0, 0.0);
	vec3<double> eAvgLoc(0.0, 0.0, 0.0);
	for (unsigned int i = 0, n = AP.size(); i < n; i++)
	{
		Planet& p = AP[i];
		const int pid = p.PlanetID();
		switch (p.Owner())
		{
			case 0: 
			{
				if (!end.IsMyPlanet(pid) && p.GrowthRate() > 0)
					NPIDX.push_back(pid);
			} break;

			case 1: 
			{
				if (end.IsEnemyPlanet(pid))
					TPIDX.push_back(pid);
				else
					NTPIDX.push_back(pid);
				mAvgLoc += p.Loc();
			} break;

			default: 
			{
				EPIDX.push_back(pid);
				eAvgLoc += p.Loc();
			} break;
		}
	}
	mAvgLoc /= (TPIDX.size() + NTPIDX.size());
	eAvgLoc /= EPIDX.size();

	for (unsigned int i = 0, n = AF.size(); i < n; i++)
	{
		Fleet& f = AF[i];
		if (f.Owner() == 1)
			MFIDX.push_back(i);
		else
			EFIDX.push_back(i);
	}
	Map map(AP);
	std::vector<int>& FLPIDX = map.GetFrontLine();
	std::vector<Fleet> orders;

	// ---------------------------------------------------------------------------
	LOG("SNIPE"); // overtake neutral planets captured by the enemy
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

				sim.Start(dist, AP, AF, false, true);
				int numShips =
					std::min<int>(source.NumShips()-bot::GetIncommingFleets(sid,
					EFIDX), sim.GetPlanet(tid).NumShips() + 1);


				// we don't wanna be sniped, just wait
				if (dist <= enemy.time)
				{
					break;
				}

				// only snipe when we are locally stronger or equal and the timedelay = 1
				if (dist > enemy.time+1 && bot::GetStrength(tid, dist, NTPIDX, MFIDX) < bot::GetStrength(tid, dist, EPIDX, EFIDX))
					continue;

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
	LOG("DEFEND AND ATTACK"); // sort planets on growthrate and perform attack
	// ---------------------------------------------------------------------------
	// gather all planets that are under attack and we can defend
	std::vector<int> DAPIDX;
	for (unsigned int i = 0, n = TPIDX.size(); i < n; i++)
	{
		Planet& target = AP[TPIDX[i]];
		const int tid = target.PlanetID();
		if (Defend(tid, AP, AF, NTPIDX, EFIDX, orders, true))
		{
			DAPIDX.push_back(tid);
		}
	}

	// gather all enemy planets that we can attack and that are weak, each
	// frontline ship gets assigned an enemy planet (they may overlap)
	std::map<int,int> targets;
	if (!EPIDX.empty())
	{
		for (unsigned int i = 0, n = FLPIDX.size(); i < n; i++)
		{
			Planet& source = AP[FLPIDX[i]];
			const int sid = source.PlanetID();
			int weakest = std::numeric_limits<int>::max();
			int bestTarget = -1;
			for (unsigned int j = 0, m = EPIDX.size(); j < m; j++)
			{
				Planet& target = AP[EPIDX[j]];
				const int tid = target.PlanetID();
				if (find(DAPIDX.begin(), DAPIDX.end(), tid) != DAPIDX.end())
					continue;

				const int dist = target.Distance(source);
				int strength = bot::GetStrength(tid, dist, EPIDX, EFIDX) + target.NumShips();
				if (strength < weakest)
				{
					weakest = strength;
					bestTarget = tid;
				}
			}
			if (bestTarget != -1 && Attack(map, EPIDX, sid, bestTarget, AP, AF, EFIDX, orders, true))
			{
				targets[bestTarget] = sid;
				DAPIDX.push_back(bestTarget);
			}
		}
	}

	// sort both attack/defend on growthrate and apply the orders
	sort(DAPIDX.begin(), DAPIDX.end(), bot::SortOnGrowthRateAndOwner);
	for (unsigned int i = 0, n = DAPIDX.size(); i < n; i++)
	{
		Planet& target = AP[DAPIDX[i]];
		const int tid = target.PlanetID();
		if (target.Owner() <= 1)
		{
			if (Defend(tid, AP, AF, NTPIDX, EFIDX, orders, false))
				IssueOrders(orders);
			else
				break;
		}
		else
		{
			if (Attack(map, EPIDX, targets[tid], tid, AP, AF, EFIDX, orders, false))
				IssueOrders(orders);
			else
				break;
		}
	}

	// ---------------------------------------------------------------------------
	LOG("EXPAND"); // capture neutrals when we are losing or drawing
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
				bot::GetIncommingFleets(pid, EFIDX) + dist*p.GrowthRate();

			numShips = std::min<int>(numShips, p.NumShips());
			if (numShips > 0)
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
				bool closer = false;
				if (turn == 0)
					closer = mdist2target <= edist2target;
				else
					closer = mdist2target < edist2target;
				const bool defend = source.NumShips() - bot::GetIncommingFleets(source.PlanetID(), EFIDX) > 0;
				if (closer && defend)
				{
					candidates.push_back(target.PlanetID());
					break;
				}
			}
		}

		std::vector<int> w; std::vector<double> v;
		std::priority_queue<bot::NPV> PQ;
		for (unsigned int i = 0, n = candidates.size(); i < n; i++)
		{
			Planet& candidate = AP[candidates[i]];
			w.push_back(candidate.NumShips() + 1);
			const int dist = (avgLoc - candidate.Loc()).len2D();
			double value = bot::GetValue(candidate, dist);
			v.push_back(value);
			PQ.push(bot::NPV(candidate.PlanetID(), value));
		}

		if (!candidates.empty() && !MHPIDX.empty())
		{
			// 3. Apply binary knapsack algorithm to select the best candidates on turn 0
			// skipping planets here which have the same distance to us and the enemy, this
			// way we can snipe those planets if the enemy captures them.
			if (turn == 0)
			{
				KnapSack ks(w, v, totalNumShipsToSpare);
				std::vector<int> I = ks.Indices();
				std::vector<int> skip;
				for (unsigned int i = 0, n = I.size(); i < n; i++)
				{
					Planet& target = AP[candidates[I[i]]];
					const int tid = target.PlanetID();
					const int sid = MHPIDX[0];
					const int eid = EPIDX[0];
					Planet& source = AP[sid];
					Planet& enemy = AP[eid];
					const int mdist = target.Distance(source);
					const int edist = target.Distance(enemy);
					if (mdist == edist)
						skip.push_back(tid);
				}

				for (unsigned int i = 0, n = I.size(); i < n; i++)
				{
					Planet& target = AP[candidates[I[i]]];
					const int tid = target.PlanetID();
					if (find(skip.begin(), skip.end(), tid) != skip.end())
						continue;

					bool success = true;
					bot::gTarget = tid;
					sort(MHPIDX.begin(), MHPIDX.end(), bot::SortOnDistanceToTarget);
					for (unsigned int j = 0, m = MHPIDX.size(); j < m; j++)
					{
						Planet& source = AP[MHPIDX[j]];
						const int sid = source.PlanetID();
						const int dist = target.Distance(source);
						int numShips = std::min<int>(target.NumShips() + 1, numShipsToSpare[sid]);
						numShips = std::min<int>(numShips, source.NumShips() - bot::GetIncommingFleets(sid, EFIDX));
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
					numShips = std::min<int>(numShips, source.NumShips() - bot::GetIncommingFleets(sid, EFIDX));
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

					// "Lock" the closest planet to the best neutral
					int sid = map.GetClosestPlanetIdx(target.Loc(), MHPIDX);
					AP[sid].NumShips(0);
				}
			}
		}
	}

	// ---------------------------------------------------------------------------
	LOG("FEED"); // support the frontline through routing
	// ---------------------------------------------------------------------------
	// compute the future frontline and use all non target planets for feeding this
	// frontline
	std::vector<Planet> AFP(AP); // all future planets
	std::vector<Fleet>  AFF(AF); // all future fleets
	end.Start(MAX_TURNS-turn, AFP, AFF);
	Map fmap(AFP); // future map, to compute future frontline
	std::vector<int>& FFLPIDX = fmap.GetFrontLine();

	for (unsigned int i = 0, n = NTPIDX.size(); i < n; i++)
	{
		Planet& source = AP[NTPIDX[i]];
		const int sid = source.PlanetID();
		if (find(FFLPIDX.begin(), FFLPIDX.end(), sid) != FFLPIDX.end())
			continue;

		const int tid = map.GetClosestPlanetIdx(source.Loc(), FFLPIDX);
		if (tid == -1)
			continue;

		Planet& target = AP[tid];
		const int dist = target.Distance(source);
		const int numShips = source.NumShips()-bot::GetIncommingFleets(sid, EFIDX);
		if (numShips <= 0)
			continue;

		const int hid = bot::GetHub(sid, tid);
		Fleet order(1, numShips, sid, hid, dist, dist);
		AF.push_back(order);
		orders.push_back(order);
		source.RemoveShips(numShips);
	}
	IssueOrders(orders);
}

#ifdef DEBUG
void SigHandler(int signum) {
	Logger logger("crash.txt");
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
	char buf[1024] = {0};
	snprintf(
		buf,
		1024 - 1,
		"%s.txt",
		argv[0]
	);
	signal(SIGSEGV, SigHandler);
	Logger logger(buf);
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
				#ifdef DEBUG
				Timer t;
				t.Tick();
				#endif
				DoTurn(pw);
				#ifdef DEBUG
				t.Tock();
				LOG("TIME: "<<t.Time()<<"s");
				#endif
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
