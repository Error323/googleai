#ifndef MAP_
#define MAP_

#include "PlanetWars.h"

#include <vector>

class Map {
public:
	Map(std::vector<Planet>&);

	void Init(const int, const int, std::vector<Fleet>& orders);
	int GetClosestFrontLinePlanetIdx(const Planet&);

	std::vector<int>& GetFrontLine()								{ return FLPIDX; }

private:
	const std::vector<Planet>& AP;

	std::vector<int>  FLPIDX;  // planets on the front line
	std::vector<int>  NPIDX;  // neutral planets
	std::vector<int>  EPIDX;  // enemy planets
	std::vector<int>  MPIDX;  // all planets belonging to us
};

#endif
