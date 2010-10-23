#include "Map.h"
#include "Logger.h"
#include "KnapSack.h"

#include <algorithm>
#include <limits>
#include <map>

Map::Map(std::vector<Planet>& ap): AP(ap) {
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

void Map::Init(const int mpid, const int epid, std::vector<Fleet>& orders) {
	const Planet& myPlanet = AP[mpid];
	const Planet& enemyPlanet = AP[epid];
	int distance = myPlanet.Distance(enemyPlanet);

	std::vector<int>    w;
	std::vector<double> v;

	for (unsigned int i = 0, n = NPIDX.size(); i < n; i++)
	{
		const Planet& p = AP[NPIDX[i]];

		w.push_back(p.NumShips() + 1);
		v.push_back(p.GrowthRate() / (myPlanet.Loc() - p.Loc()).len2D());
	}

	KnapSack ks(w, v, myPlanet.NumShips());
	std::vector<int>& I = ks.Indices();
	for (unsigned int i = 0, n = I.size(); i < n; i++)
	{
		const Planet& t = AP[NPIDX[I[i]]];
		const int d = myPlanet.Distance(t);
		Fleet f(1, t.NumShips()+1, myPlanet.PlanetID(), t.PlanetID(), d, d);
		orders.push_back(f);
	}

/*
	// Get the subset of myHalf that has the maximal growth
	// rate such that we can support the start planet
	bool covered = false;
	int numShips = myPlanet.NumShips();
	sort(myHalf.begin(), myHalf.end(), SortOnGrowShipRatio);

	for (unsigned int i = 0, n = myHalf.size(); i < n; i++)
	{
		PlanetSpecs& ps = myHalf[i];
		int numShipsRequired = ps.numShips + 1;

		if (numShips - numShipsRequired <= 0)
			break;

		numShips -= numShipsRequired;

		// can we provide backup to our startplanet?
		covered = covered ||              // already covered
			(distance - ps.eDist) >= 0 || // in between us
			(distance - ps.mDist*2) > 0;  // return trip closer

		Fleet order(1, numShipsRequired, myPlanet.PlanetID(), ps.pID, ps.mDist,
			ps.mDist);
		orders.push_back(order);
	}
*/
}
