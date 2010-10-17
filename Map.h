#ifndef MAP_
#define MAP_

#include "PlanetWars.h"

#include <vector>
#include <map>

class Map {
public:
	Map(std::vector<Planet>&);

	struct PlanetSpecs {
		PlanetSpecs(){}
		PlanetSpecs(int pid, int mdist, int edist, int gr, int ns):
			pID(pid),
			mDist(mdist),
			eDist(edist),
			growRate(gr),
			numShips(ns)
		{}

		int pID;
		int mDist;
		int eDist;
		int growRate;
		int numShips;
	};

	void Init(const int, const int, std::vector<Fleet>& orders);
	std::vector<int>& GetFrontLine() { return FLIDX; }
	int GetEnemyStrength(const int pid) { return FL[pid]; }

private:
	const std::vector<Planet>& AP;
	std::vector<PlanetSpecs> mHalf;

	std::map<int, int> FL;    // frontline and the strength they need
	std::vector<int>  HIDX;   // planets on my initial half
	std::vector<int>  FLIDX;  // planets on the front line
	std::vector<int>  NPIDX;  // neutral planets
	std::vector<int>  EPIDX;  // enemy planets
	std::vector<int>  MPIDX;  // all planets belonging to us
};

#endif
