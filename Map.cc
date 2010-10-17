#include "Map.h"
#include "Logger.h"

#include <algorithm>

bool SortOnGrowShipRatio(const Map::PlanetSpecs& a, const Map::PlanetSpecs& b) {
	const double growA = a.growRate / (1.0*a.numShips + 1.0);
	const double growB = b.growRate / (1.0*b.numShips + 1.0);
	return growA > growB;
}

bool SortOnDistance(const Map::PlanetSpecs& a, const Map::PlanetSpecs& b) {
	return a.mDist < b.mDist;
}

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
		const int numShips = AP[i->first].NumShips();
		if (find(FLIDX.begin(), FLIDX.end(), i->second) == FLIDX.end())
		{
			FLIDX.push_back(i->second);
			FL[i->second] = numShips;
		}
		else
		{
			FL[i->second] += numShips;
		}
	}
}

void Map::Init(const int mpid, const int epid, std::vector<Fleet>& orders) {
	const Planet& myPlanet = AP[mpid];
	const Planet& enemyPlanet = AP[epid];
	int distance = myPlanet.Distance(enemyPlanet);

	// filter out planets closer to us or equally far
	// from the enemy
	for (unsigned int i = 0, n = AP.size(); i < n; i++)
	{
		const Planet& p = AP[i];
		int pid   = p.PlanetID();
		int mdist = p.Distance(myPlanet);
		int edist = p.Distance(enemyPlanet);

		if (mdist > edist || pid == mpid)
			continue;

		HIDX.push_back(pid);
		mHalf.push_back(PlanetSpecs(
			pid,
			mdist,
			edist,
			p.GrowthRate(),
			p.NumShips()
		));
	}

	// Get the subset of mHalf that has the maximal growth
	// rate such that we can support the start planet
	bool covered = false;
	int numShips = myPlanet.NumShips();
	sort(mHalf.begin(), mHalf.end(), SortOnGrowShipRatio);
	for (unsigned int i = 0, n = mHalf.size(); i < n; i++)
	{
		PlanetSpecs& ps = mHalf[i];
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

	// if not, we need a different set of start planets to capture
	if (!covered)
	{
		double best = -1.0;
		Fleet order;
		sort(mHalf.begin(), mHalf.end(), SortOnDistance);
		for (unsigned int i = 0, n = mHalf.size(); i < n; i++)
		{
			PlanetSpecs& ps = mHalf[i];
			if ((distance - ps.eDist) >= 0 || (distance - ps.mDist*2) > 0)
			{
				double r = ps.growRate / (ps.numShips + 1.0);
				if (r > best)
				{
					best = r;
					order = Fleet(1, ps.numShips+1, myPlanet.PlanetID(),
						ps.pID, ps.mDist, ps.mDist);
				}
			}
		}
		// there is nothing we can do
		if (best == -1.0)
			return;

		// if we still have enough ships...
		if (numShips >= order.NumShips() || orders.empty())
		{
			orders.push_back(order);
		}

		// otherwise remove the last order from the queue until we do
		// have enough ships
		else
		{
			while (numShips < order.NumShips())
			{
				Fleet& last = orders.back();
				numShips += last.NumShips();
				orders.pop_back();
			}
			orders.push_back(order);
		}
	}
}
