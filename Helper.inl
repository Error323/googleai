const std::vector<Planet>* gAP     = NULL;
const std::vector<Fleet>*  gAF     = NULL;
int                        gTarget = 0;

inline bool SortOnGrowthRateAndOwner(const int pidA, const int pidB) {
	const Planet& a = gAP->at(pidA);
	const Planet& b = gAP->at(pidB);

	if (a.GrowthRate() == b.GrowthRate())
		return a.Owner() < b.Owner();
	else
		return a.GrowthRate() > b.GrowthRate();
}

inline bool SortOnDistanceToTarget(const int pidA, const int pidB) {
	const Planet& t = gAP->at(gTarget);
	const Planet& a = gAP->at(pidA);
	const Planet& b = gAP->at(pidB);
	const int distA = a.Distance(t);
	const int distB = b.Distance(t);
	return distA < distB;
}

inline bool SortOnGrowthShipRatio(const int pidA, const int pidB) {
	const Planet& a = gAP->at(pidA);
	const Planet& b = gAP->at(pidB);
	const double growA = a.GrowthRate() / (1.0*a.NumShips() + 1.0);
	const double growB = b.GrowthRate() / (1.0*b.NumShips() + 1.0);
	return growA > growB;
}

inline bool SortOnPlanetAndTurnsLeft(const Fleet& a, const Fleet& b) {
	if (a.DestinationPlanet() == b.DestinationPlanet())
		return a.TurnsRemaining() < b.TurnsRemaining();
	else
		return a.DestinationPlanet() < b.DestinationPlanet();
}

void Erase(std::vector<int>& subject, std::vector<int>& eraser) {
	ASSERT(subject.size() >= eraser.size());
	sort(eraser.begin(), eraser.end());

	for (int i = eraser.size()-1; i >= 0; i--)
		subject.erase(subject.begin()+eraser[i]);

	eraser.clear();
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

// Determine the "goodness" of a neutral planet
double GetValue(Planet& p, int dist) {
	#define EPS 1.0e-10
	return pow(p.GrowthRate(), 2.0) / (p.NumShips() * dist + EPS);
}

// Get the incomming fleets of a certain planet
int GetIncommingFleets(const int sid, std::vector<int>& FIDX, int remaining = 1000) {
	int numFleets = 0;
	for (unsigned int j = 0, m = FIDX.size(); j < m; j++)
	{
		const Fleet& f = gAF->at(FIDX[j]);
		if (f.DestinationPlanet() == sid && f.TurnsRemaining() <= remaining)
		{
			numFleets += f.NumShips();
		}
	}
	return numFleets;
}

// Compute the potential strength if all planets in the neighbourhood will feed this planet
int GetStrength(const int tid, const int dist, std::vector<int>& PIDX, std::vector<int>& FIDX) {
	int strength = 0;
	const Planet& target = gAP->at(tid);
	for (unsigned int i = 0, n = PIDX.size(); i < n; i++)
	{
		const Planet& p = gAP->at(PIDX[i]);
		const int pid = p.PlanetID();
		int distance = target.Distance(p);
		if (distance < dist && pid != tid)
		{
			int time = dist - distance;
			strength += p.NumShips() + time*p.GrowthRate() + GetIncommingFleets(pid, FIDX, time);
		}
	}
	return strength;
}

// Determine the closest-to-source in between lying hub for rerouting
int GetHub(const int sid, const int tid) {
	#define BETWEEN(v, a, b) ((v > a && v < b) || (v < a && v > b))
	static const double MAX_DETOUR = 1.0;

	int hid = tid;
	const Planet& source = gAP->at(sid);
	const Planet& target = gAP->at(tid);
	vec3<double> vecTarget = target.Loc() - source.Loc();
	double distance = vecTarget.len2D();
	double s2t = distance;
	for (unsigned int i = 0, n = gAP->size(); i < n; i++)
	{
		const Planet& hub = gAP->at(i);
		if (hub.Owner() == source.Owner() && hub.PlanetID() != tid && hub.PlanetID() != sid)
		{
			vec3<double> vecHub = hub.Loc() - source.Loc();
			vec3<double> vecProjectHub = vecHub.project(vecTarget);
			if (BETWEEN(vecProjectHub.x, 0.0, vecTarget.x) && BETWEEN(vecProjectHub.z, 0.0, vecTarget.z))
			{
				const double s2h = vecHub.len2D();
				const double h2t = (hub.Loc() - target.Loc()).len2D();
				const double pLength = vecProjectHub.len2D();
				const double detour  = (s2h + h2t) - s2t;
				if (pLength < distance && detour <= MAX_DETOUR)
				{
					hid = hub.PlanetID();
					distance = pLength;
				}
			}
		}
	}
	return hid;
}

