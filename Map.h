#ifndef MAP_
#define MAP_

#include "PlanetWars.h"
#include "vec3.h"

#include <vector>

class Map {
public:
	Map(std::vector<Planet>&);

	int GetClosestFrontLinePlanetIdx(const Planet&);
	int GetClosestPlanetIdx(const vec3<double>&, const std::vector<int>&);

	std::vector<int>  GetPlanetIDsInRadius(const vec3<double>&, const std::vector<int>&, const int);
	std::vector<int>& GetFrontLine(){ return FLPIDX; }

private:
	const std::vector<Planet>& AP; // hard copy of all the planets

	std::vector<int>  FLPIDX;  // planets on the front line
	std::vector<int>  NPIDX;   // neutral planets
	std::vector<int>  EPIDX;   // enemy planets
	std::vector<int>  MPIDX;   // all planets belonging to us
};

#endif
