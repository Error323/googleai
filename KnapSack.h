#include <vector>

class KnapSack {
public:
	KnapSack(std::vector<int>&, std::vector<double>&, int);
	std::vector<int>& Indices();

private:
	std::vector<int>    w; // w_i,...,w_n
	std::vector<double> v; // v_i,...,v_n
	int W;                 // Max weight possible
	int N;                 // Max items possible
	std::vector<double> C; // Cost matrix
	std::vector<int>    I; // Solution indices

	double ZeroOne(int, int);
};
