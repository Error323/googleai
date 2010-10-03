#ifndef SIMULATE_H_
#define SIMULATE_H_

#include "PlanetWars.h"

#define LOG(msg)                  \
	if (file.good())              \
	{                             \
		file << msg << std::endl; \
	}

class Simulator {
public:
	Simulator(PlanetWars* pw_):
		pw(pw_),
		winning(false)
	{
	}

	void Start(unsigned int, std::ofstream&);
	bool Winning() { return winning; }

private:
	bool        winning;
	PlanetWars* pw;
};

#endif
