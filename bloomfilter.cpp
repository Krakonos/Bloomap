#include <iostream>
#include <bitset>
#include <map>
#include <vector>
#include <cassert>
#include <cstdlib>
#include <cstring>

#include "murmur.h"
#include "bloomfilter.h"

/* This is static vector of hash functions. There are in fact seeds for the
 * murmur hash function, and are generated in constructor randomly, as needed */
static std::vector<uint32_t> hash_functions;

/*
 * Create a simple instance, with k compartments, each with it's own hash
 * function, and split m bits into all components.
 */
BloomFilter::BloomFilter(unsigned m, unsigned k) {
	_init(k, m/k, 1);
}

BloomFilter::BloomFilter(BloomFilter *orig) {

	_init(orig->ncomp, orig->compsize, orig->nfunc);

	for (unsigned comp = 0; comp < ncomp; comp++) {
		for (unsigned pos = 0; pos < orig->compsize; pos++) {
			bits[comp*bits_segsize + pos] = orig->bits[comp*bits_segsize + pos];
		}
	}
}

/*
 * Create rather convoluted instance, parametrized with number of components,
 * component size, and number of hash functions for EACH component.
 */
BloomFilter::BloomFilter(unsigned ncomp, unsigned compsize, unsigned nfunc) {
	_init(ncomp, compsize, nfunc);
}

void BloomFilter::_init(unsigned _ncomp, unsigned _compsize, unsigned _nfunc) {
	ncomp = _ncomp;
	compsize = _compsize;
	nfunc = _nfunc;

	assert(ncomp);
	assert(compsize);
	assert(nfunc);

	/* Generate seeds for all the hash functions */
	while (hash_functions.size() < nfunc*ncomp) {
		/* TODO: Possibly ensure different functions */
		hash_functions.push_back(rand());
	}

	/* Generate compartments */
	bits_segsize = (compsize / sizeof(BITS_TYPE))+1;
	bits_size = bits_segsize*ncomp;
	bits = new BITS_TYPE[bits_size];
	memset(bits, 0, bits_size*sizeof(BITS_TYPE));
}

BloomFilter::~BloomFilter() {

}

void BloomFilter::clear(void) {
	memset(bits, 0, bits_size*sizeof(BITS_TYPE));
}

bool BloomFilter::add(unsigned ele) {
	changed = false;
	/* Set appropriate bits in each container */
	unsigned fn = 0;
	for (unsigned comp = 0; comp < ncomp; comp++) {
		for (unsigned i = 0; i < nfunc; i++) {
			uint32_t h = hash(ele, fn++);
			set(comp,h);
		}
	}
	return changed;
}

bool BloomFilter::contains(unsigned ele) {
	unsigned fn = 0;
	for (unsigned comp = 0; comp < ncomp; comp++) {
		for (unsigned i = 0; i < nfunc; i++) {
			uint32_t h = hash(ele, fn++);
			if (!get(comp,h)) return false;
		}
	}
	return true;
}

unsigned BloomFilter::hash(unsigned ele, unsigned i) {
	return MurmurHash1(ele, hash_functions[i]) % compsize;
}

void BloomFilter::dump(void) {
	using namespace std;
	cerr << "=> Bloom filter dump (" << ncomp << " compartments, " << compsize << " bits in each)" << endl;
	for (unsigned comp = 0; comp < ncomp; comp++) {
		for (unsigned i = 0; i < compsize; i++) {
			if (get(comp,i)) cerr << "1";
			else cerr << ".";
			if (i % 80 == 79) cerr << endl;
		}
		cerr << endl << "=======================" << endl;
	}
}

unsigned BloomFilter::popcount(void) {
	unsigned count = 0;
	for (unsigned comp = 0; comp < ncomp; comp++) {
		for (unsigned i = 0; i < compsize; i++) {
			if (get(comp,i)) count++;
		}
	}
	return count;
}

unsigned BloomFilter::mapsize(void) {
	return bits_size*sizeof(BITS_TYPE);
}

BloomFilter* BloomFilter::intersect(BloomFilter *filter) {
	for (unsigned comp = 0; comp < ncomp; comp++) {
		for (unsigned i = 0; i < bits_segsize; i++) {
			bits[comp*bits_segsize + i] &= filter->bits[comp*bits_segsize + i];
		}
	}
	return this;
}

BloomFilter* BloomFilter::or_from(BloomFilter *filter) {
	changed = false;
	for (unsigned comp = 0; comp < ncomp; comp++) {
		for (unsigned i = 0; i < bits_segsize; i++) {
			bits[comp*bits_segsize + i] |= filter->bits[comp*bits_segsize + i];
		}
	}
	return this;
}

bool BloomFilter::add(BloomFilter *filter) {
	changed = false;
	for (unsigned comp = 0; comp < ncomp; comp++) {
		for (unsigned i = 0; i < bits_segsize; i++) {
			unsigned index = comp*bits_segsize + i;
			if ((bits[index] & filter->bits[index]) != filter->bits[index]) {
				changed = true;
				bits[index] |= filter->bits[index];
			}
		}
	}
	return changed;
}

bool BloomFilter::isEmpty(void) {
	for (unsigned comp = 0; comp < ncomp; comp++) {
		bool empty = true;
		for (unsigned i = 0; i < bits_segsize; i++) {
			if (bits[comp*bits_segsize + i]) {
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
	for (unsigned comp = 0; comp < ncomp; comp++) {
		for (unsigned i = 0; i < bits_segsize; i++) {
			if (rhs->bits[comp*bits_segsize + i] != bits[comp*bits_segsize + i]) return false;
		}
	}
	return true;
}

bool BloomFilter::operator!=(const BloomFilter* rhs) {
	return !operator==(rhs);
}
