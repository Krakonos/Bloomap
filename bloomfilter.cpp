#include <iostream>
#include <bitset>
#include <map>
#include <vector>
#include <cassert>
#include <cstdlib>

#include "murmur.h"
#include "bloomfilter.h"

std::map<unsigned, bool> m;


/* This is static vector of hash functions. There are in fact seeds for the
 * murmur hash function, and are generated in constructor randomly, as needed */
static std::vector<uint32_t> hash_functions;

/*
 * Create a simple instance, with k compartments, each with it's own hash
 * function, and split m bits into all components.
 */
BloomFilter::BloomFilter(unsigned m, unsigned k)
	: BloomFilter(k, m/k, 1)
{
}

BloomFilter::BloomFilter(BloomFilter *orig)
	: BloomFilter(orig->ncomp, orig->compsize, orig->nfunc)
{
	for (unsigned comp = 0; comp < ncomp; comp++) {
		for (unsigned pos = 0; pos < orig->compsize; pos++) {
			bits[comp][pos] = orig->bits[comp][pos];
		}
	}
}

/*
 * Create rather convoluted instance, parametrized with number of components,
 * component size, and number of hash functions for EACH component.
 */
BloomFilter::BloomFilter(unsigned ncomp, unsigned compsize, unsigned nfunc) :
	nfunc(nfunc), compsize(compsize), ncomp(ncomp)
{
	assert(ncomp);
	assert(compsize);
	assert(nfunc);

	/* Generate seeds for all the hash functions */
	while (hash_functions.size() < nfunc*ncomp) {
		/* TODO: Possibly ensure different functions */
		hash_functions.push_back(rand());
	}

	/* Generate compartments */
	bits.resize(ncomp);
	for (std::vector<bool> &comp : bits) {
		comp.resize(compsize, false); /* Component size is in bits, take care to have empty bitmap */
	}
}

BloomFilter::~BloomFilter() {

}

bool BloomFilter::add(unsigned ele) {
	bool changed = false;
	/* Set appropriate bits in each container */
	unsigned fn = 0;
	for (std::vector<bool> &comp : bits) {
		for (unsigned i = 0; i < nfunc; i++) {
			uint32_t h = hash(ele, fn++);
			if (!comp[h]) changed = true;
			comp[h] = true;
		}
	}
	return changed;
}

bool BloomFilter::contains(unsigned ele) {
	unsigned fn = 0;
	for (std::vector<bool> &comp : bits) {
		for (unsigned i = 0; i < nfunc; i++) {
			uint32_t h = hash(ele, fn++);
			if (!comp[h]) return false;
		}
	}
	return true;
}

unsigned BloomFilter::hash(unsigned ele, unsigned i) {
	return MurmurHash1(ele, hash_functions[i]) % compsize;
}

void BloomFilter::dump(void) {
	using namespace std;
	cerr << "=> Bloom filter dump (" << bits.size() << " compartments, " << compsize << " bits in each)" << endl;
	for (std::vector<bool> &comp : bits) {
		for (unsigned i = 0; i < compsize; i++) {
			if (comp[i]) cerr << "1";
			else cerr << ".";
			if (i % 80 == 79) cerr << endl;
		}
		cerr << endl << "=======================" << endl;
	}
}

unsigned BloomFilter::popcount(void) {
	unsigned count = 0;
	for (std::vector<bool> &comp : bits) {
		for (unsigned i = 0; i < compsize; i++) {
			if (comp[i]) count++;
		}
	}
	return count;
}

unsigned BloomFilter::mapsize(void) {
	return compsize*bits.size();
}

BloomFilter* BloomFilter::intersect(BloomFilter *filter) {
	for (unsigned comp = 0; comp < bits.size(); comp++) {
		for (unsigned i = 0; i < compsize; i++) {
			if (!filter->bits[comp][i])
			bits[comp][i] = false;
		}
	}
	return this;
}

BloomFilter* BloomFilter::or_from(BloomFilter *filter) {
	for (unsigned comp = 0; comp < bits.size(); comp++) {
		for (unsigned i = 0; i < compsize; i++) {
			if (filter->bits[comp][i])
			bits[comp][i] = true;
		}
	}
	return this;
}

bool BloomFilter::isEmpty(void) {
	for (std::vector<bool> &comp : bits) {
		bool empty = true;
		for (unsigned i = 0; i < compsize; i++) {
			if (comp[i]) {
				empty = false;
				break;
			}
		}
		if (empty) return true;
	}
	return false;
}

bool BloomFilter::operator==(const BloomFilter* rhs) {
	if (ncomp != rhs->ncomp || compsize != rhs->compsize || nfunc != rhs->nfunc) return false;
	for (unsigned comp = 0; comp < bits.size(); comp++) {
		for (unsigned i = 0; i < compsize; i++) {
			if (rhs->bits[comp][i] != bits[comp][i]) return false;
		}
	}
	return true;
}

bool BloomFilter::operator!=(const BloomFilter* rhs) {
	return !operator==(rhs);
}
