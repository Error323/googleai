#include "KnapSack.h"
#include "Logger.h"

#define IDX(i,j) ((i)*(W+1)+(j))

KnapSack::KnapSack(std::vector<int>& w_, std::vector<double>& v_, int W_) {
	Init(w_, v_, W_);
}

void KnapSack::Init(std::vector<int>& w_, std::vector<double>& v_, int W_) {
	w = w_;
	v = v_;
	W = W_;
	N = w.size();
	v.insert(v.begin(),0.0);
	w.insert(w.begin(),0);
	ASSERT(w.size() == v.size() && !w.empty());
	C.resize((N+1)*(W+1), 0.0);

	for (int i = 0; i <= N; i++)
	{
		for (int j = 0; j <= W; j++)
		{
			if (i == 0 || j == 0)
				C[IDX(i, j)] =  0.0;
			else
				C[IDX(i, j)] = -1.0;
		}
	}
	I.clear();
}

std::vector<int>& KnapSack::Indices() {
	ASSERT(!v.empty() && !w.empty());
	if (!I.empty())
	{
		return I;
	}

	ZeroOne(N, W);
	int aux = W;
	for (int i = N; i > 0; i--)
	{
		if (C[IDX(i, aux)] != C[IDX(i - 1, aux)])
		{
			I.push_back(i-1);
			aux -= w[i];
		}
	}
	return I;
}


double KnapSack::ZeroOne(int n, int c) {
	double val, temp, temp1;
	if (C[IDX(n, c)] < 0.0)
	{
		temp = ZeroOne(n - 1, c);
		if (c < w[n])
		{
			val = temp;
		}
		else
		{
			temp1 = v[n] + ZeroOne(n - 1, c - w[n]);
			if (temp1 > temp)
				val = temp1;
			else
				val = temp;
		}
		C[IDX(n, c)] = val;
	}
	return C[IDX(n, c)];
}
