#include "Map.h"
#include "Logger.h"
#include "KnapSack.h"
#include "Simulator.h"

#include <algorithm>
#include <limits>
#include <map>

namespace map {
	#include "Helper.inl"
}

Map::Map(std::vector<Planet>& ap): AP(ap) {
	map::gAP = &AP;
	// compute our planets, enemy planets etc
	for (unsigned int i = 0, n = AP.size(); i < n; i++)
	{
		const Planet& p = AP[i];
		const int pid = p.PlanetID();
		switch (p.Owner())
		{
			case 0:  NPIDX.push_back(pid); break;
			case 1:  MPIDX.push_back(pid); break;
			default: EPIDX.push_back(pid); break;
		}
	}

	// compute frontline, for each enemyplanet find our closest planet
	std::map<int, int> tmp;
	for (unsigned int i = 0, n = EPIDX.size(); i < n; i++)
	{
		const Planet& eP = AP[EPIDX[i]];
		const int eid = eP.PlanetID();
		for (unsigned int j = 0, m = MPIDX.size(); j < m; j++)
		{
			const Planet& mP = AP[MPIDX[j]];
			const int mid = mP.PlanetID();
			if (tmp.find(eid) == tmp.end())
			{
				tmp[eid] = mid;
			}
			else
			{
				const int d1 = eP.Distance(mP);
				const int d2 = eP.Distance(AP[tmp[eid]]);
				if (d1 < d2)
				{
					tmp[eid] = mid;
				}
			}
		}
	}

	typedef std::map<int, int>::iterator MIter;
	for (MIter i = tmp.begin(); i != tmp.end(); i++)
	{
		if (find(FLPIDX.begin(), FLPIDX.end(), i->second) == FLPIDX.end())
		{
			FLPIDX.push_back(i->second);
		}
	}
}

int Map::GetClosestFrontLinePlanetIdx(const Planet& p) {
	int closestDist = std::numeric_limits<int>::max();
	int closestPid  = -1;
	for (unsigned int i = 0, n = FLPIDX.size(); i < n; i++)
	{
		const Planet& t = AP[FLPIDX[i]];
		int dist = t.Distance(p);
		if (dist < closestDist)
		{
			closestDist = dist;
			closestPid = t.PlanetID();
		}
	}
	return closestPid;
}

std::vector<int> Map::GetPlanetIDsInRadius(const vec3<double>& pos,
					const std::vector<int>& candidates, const int r) {
	std::vector<int> PIRIDX;
	for (unsigned int i = 0, n = candidates.size(); i < n; i++)
	{
		const Planet& p = AP[candidates[i]];

		int dist = (pos - p.Loc()).len2D();
		if (dist <= r)
			PIRIDX.push_back(p.PlanetID());
	}
	return PIRIDX;
}

int Map::GetClosestPlanetIdx(const vec3<double>& pos, const std::vector<int>& candidates) {
	int closestDist = std::numeric_limits<int>::max();
	int pid = -1;
	for (unsigned int i = 0, n = candidates.size(); i < n; i++)
	{
		const Planet& p = AP[candidates[i]];
		const int dist = (pos - p.Loc()).len2D();
		if (dist < closestDist)
		{
			closestDist = dist;
			pid = p.PlanetID();
		}
	}
	return pid;
}

void Map::GetOrdersForCaptureableNeutrals(const std::vector<Fleet>& AF, std::vector<Fleet>& orders, std::vector<int> CNPIDX) {
	extern const int turn;
	extern const int MAX_ROUNDS;
	std::vector<int> MHPIDX(MPIDX); // planets that have spare ships
	std::vector<int> eraser;

	// while there are captureable neutrals left and planets that are able
	// to safely capture them...
	while (true)
	{
		if (CNPIDX.empty() || MHPIDX.empty() || EPIDX.empty())
			break;

		orders.clear();

		int totalShipsToSpare = 0;
		vec3<double> avgLoc(0.0,0.0,0.0);
		std::map<int,int> shipsToSpare;

		// compute average location of planets that can help in capturing one or
		// more neutral planets and their ships to spare
		std::vector<int> marked;
		for (unsigned int i = 0, n = MHPIDX.size(); i < n; i++)
		{
			const Planet& p  = AP[MHPIDX[i]];
			const int eid    = GetClosestPlanetIdx(p.Loc(), EPIDX);
			const Planet& e  = AP[eid];
			const int radius = p.Distance(e);
			const std::vector<int> DPIDX = GetPlanetIDsInRadius(p.Loc(), MPIDX, radius);

			// compute the number of ships we can use
			int numShips = p.NumShips() - e.NumShips();
			for (unsigned int j = 0, m = DPIDX.size(); j < m; j++)
			{
				const Planet& d = AP[DPIDX[j]];
				if (find(marked.begin(), marked.end(), d.PlanetID()) != marked.end())
					continue;

				const int dist = p.Distance(d);
				numShips += (radius-dist)*d.GrowthRate();
				marked.push_back(d.PlanetID());
			}

			numShips = std::min<int>(numShips, p.NumShips());
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
		Erase(MHPIDX, eraser);

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

		// make sure all planets that are captured are safe
		std::vector<Fleet>  AF_(AF);
		std::vector<Planet> AP_(AP);
		Simulator sim;
		for (unsigned int i = 0, n = I.size(); i < n; i++)
		{
			const Planet& target = AP_[CNPIDX[I[i]]];

			map::gTarget = target.PlanetID();
			sort(MHPIDX.begin(), MHPIDX.end(), map::SortOnDistanceToTarget);
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
		Erase(CNPIDX, eraser);
	}
}

bool Map::IsSafe(const Planet& target, std::vector<Planet>& ap, std::vector<Fleet>& af) {
	ASSERT(target.Owner() == 0);
	std::vector<Planet> AP(ap);
	std::vector<Fleet>  AF(af);
	extern const int turn;
	extern const int MAX_ROUNDS;
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
	}
	return sim.IsMyPlanet(target.PlanetID());
}

void Map::Erase(std::vector<int>& subject, std::vector<int>& eraser) {
	ASSERT(subject.size() >= eraser.size());
	sort(eraser.begin(), eraser.end());

	for (int i = eraser.size()-1; i >= 0; i--)
		subject.erase(subject.begin()+eraser[i]);

	eraser.clear();
}
