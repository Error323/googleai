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
