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
