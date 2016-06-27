#include <cmath>
#include <cassert>
#include <iostream>

#include "bloomapfamily.h"


BloomapFamily::BloomapFamily(unsigned m, unsigned k)
	: m(m), k(k)
{
	ins_data.resize(m);
}

/* Convenience functions to create right families depending on the needs */
BloomapFamily* BloomapFamily::forElementsAndProb(unsigned n, double p) {
	unsigned m = ceil((n * log(p)) / log(1.0 / (pow(2.0, log(2.0)))));
	unsigned k = round(log(2.0) * m / n);
	assert(m);
	assert(k);

	return new BloomapFamily(m, k);
}

BloomapFamily* BloomapFamily::forSizeAndFunctions(unsigned m, unsigned k) {
	return new BloomapFamily(m, k);
}

/* Create and return a new map from this family */
Bloomap* BloomapFamily::newMap(void) {
	Bloomap* nm = new Bloomap(this, m, k);
	bloomaps.push_back(nm);
	return nm;
}

/* File the new element under index */
void BloomapFamily::newElement(unsigned ele, unsigned index) {
	ins_data[index].insert(ele);
}

void BloomapFamily::dumpCandidates(void) {
	for (unsigned i = 0; i < ins_data.size(); i++) {
		for (std::set<unsigned>::iterator it = ins_data[i].begin(); it != ins_data[i].end(); ++it) {
			unsigned e = *it;
			std::cerr << "Element (at " << i << "): " << e << std::endl;
		}
	}
}
