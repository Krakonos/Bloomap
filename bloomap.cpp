#include <iostream>
#include <cstdint>
#include <bitset>
#include <map>
#include <vector>

#include "murmur.h"
#include "bloomap.h"
#include "bloomapfamily.h"

//std::map<unsigned, bool> m;

/* This is static vector of hash functions. There are in fact seeds for the
 * murmur hash function, and are generated in constructor randomly, as needed */
static std::vector<uint32_t> hash_functions;


Bloomap::Bloomap(BloomapFamily* f, unsigned m, unsigned k) :
	BloomFilter(m, k), f(f)
{
}

Bloomap::Bloomap(Bloomap *orig)
	: BloomFilter(orig)
{
}

bool Bloomap::add(unsigned ele) {
#ifdef DEBUG_STATS
	real_contents.insert(ele);
#endif
	if (BloomFilter::add(ele)) {
		f->newElement(ele, hash(ele, 0));
		return true;
	} else return false;
}

bool Bloomap::contains(unsigned ele) {
	bool ret = BloomFilter::contains(ele);
#ifdef DEBUG_STATS
	counter_query++;
	if (ret && !real_contents.count(ele))
		counter_fp++;
#endif
	return ret;
}

void Bloomap::dump(void) {
	BloomFilter::dump();
}

Bloomap* Bloomap::intersect(Bloomap* map) {
	BloomFilter::intersect(map);
	return this;
}

//Bloomap* unite(Bloomap* map);

void Bloomap::clear() {
	for (unsigned comp = 0; comp < ncomp; comp++) {
		for (unsigned i = 0; i < compsize; i++) {
			bits[comp][i] = false;
		}
	}
}

void Bloomap::purge() {
	Bloomap orig(this);
	clear();
	for (unsigned pos = 0; pos < compsize; pos++) {
		if (!orig.bits[0][pos]) continue;
		std::unordered_set<int> &set = family()->ins_data[pos];
		for (const auto entry : set) {
			if (orig.contains(entry))
				add(entry);
			else {
				/* TODO: Count some statistics about purged elements */
			}
		}
	}
}

void Bloomap::dumpStats(void) {
	std::cerr << "  Map compartments:       " << ncomp << std::endl;
	std::cerr << "  Map size (in bits):     " << mapsize() << std::endl;
	std::cerr << "  Map popcount (in bits): " << popcount()<< std::endl;
	std::cerr << "  Map popcount (ratio):   " << popcount()*1.0/mapsize() << std::endl;
	std::cerr << "  Empty:                  " << (isEmpty() ? "yes" : "no") << std::endl;

}
