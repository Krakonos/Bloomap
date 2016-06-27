#include <iostream>
#include <bitset>
#include <map>
#include <vector>
#include <cassert>

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
	resetStats();
}

Bloomap::Bloomap(Bloomap *orig)
	: BloomFilter(orig)
{
}

void Bloomap::resetStats(void) {
	counter_fp = 0;
	counter_query = 0;
}

bool Bloomap::add(unsigned ele) {
#ifdef DEBUG_STATS
	real_contents.insert(ele);
#endif
	f->newElement(ele, hash(ele, 0)); /* This can't be conditional, as we need even collided elements! */
	if (BloomFilter::add(ele)) {
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
	for (unsigned glob_pos = 0; glob_pos < compsize; glob_pos++) {
		if (!orig.bits[0][glob_pos]) continue;
		std::set<unsigned> &set = family()->ins_data[glob_pos];
		for (std::set<unsigned>::const_iterator it = set.begin(); it != set.end(); it++) {
			unsigned entry = *it;
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


/* Bloomap operators */

bool Bloomap::operator==(const Bloomap* rhs) {
	/* Trivial cases */
	if (this == rhs) return true;
	if (f != rhs->f) return false;

	/* We have to check the bits.. */
	return BloomFilter::operator==(rhs);
}

bool Bloomap::operator!=(const Bloomap* rhs) {
	return !operator==(rhs);
}

/* Iterator stuff */

BloomapIterator begin(Bloomap *map) {
	return BloomapIterator(map, false);
}

BloomapIterator end(Bloomap *map) {
	return BloomapIterator(map, true);
}

BloomapIterator::BloomapIterator(const BloomapIterator& orig) {
	map = orig.map;
	glob_pos = orig.glob_pos;
	set_iterator = orig.set_iterator;
}

BloomapIterator::BloomapIterator(Bloomap *map, bool end) {
	_init(map,end);
}

void BloomapIterator::_init(Bloomap *_map, bool end) {
	map = _map;
	/* We are creating the "end" iterator */
	if (end) {
		glob_pos = map->compsize-1;
		set_iterator = map->family()->ins_data[map->compsize-1].end();
	} else {
		glob_pos = 0;
		set_iterator = map->family()->ins_data[0].begin();

		/* Check if the first element is in the map, otherwise call ++ to find one. */
		if (set_iterator == map->family()->ins_data[0].end() || !map->contains(*set_iterator))
			operator++();
	}
}

BloomapIterator::BloomapIterator(Bloomap *map, unsigned& first)
{
	_init(map);
	first = operator*();
}

bool BloomapIterator::isValid(void) {
	return !(set_iterator == map->family()->ins_data[glob_pos].end());
}

/* Advance the iterator and return true if it's dereferencable */
bool BloomapIterator::advanceSetIterator(void) {
	if (isValid())
		set_iterator++;
	/* Return false if the iterator changed to past-the-end as well! */
	return isValid();
}

BloomapIterator& BloomapIterator::operator++() {
	while (1) {
		/* Advance the iterator. If it's dereferenceable, check if it's in the map. Otherwise continue to the next element. */
		if (advanceSetIterator()) {
			if (map->contains(*set_iterator)) return *this;
			else continue;
		}

		/* Now the iterator is invalid. If it's in the last set, abort now. */
		if (glob_pos == (map->compsize-1)) return *this;

		/* Allright, there is at least one set to explore. Jum to it. */
		glob_pos++;
		set_iterator = map->family()->ins_data[glob_pos].begin();
		if (isValid() && map->contains(*set_iterator)) return *this;
	}
	return *this;
}

BloomapIterator BloomapIterator::operator++(int count) {
	BloomapIterator tmp(*this);
	operator++();
	return tmp;
}

bool BloomapIterator::operator==(const BloomapIterator& rhs) {
	return (map == rhs.map && glob_pos == rhs.glob_pos && set_iterator == rhs.set_iterator);
}

bool BloomapIterator::operator!=(const BloomapIterator& rhs) {
	return !operator==(rhs);
	//return !(map == rhs.map && glob_pos == rhs.glob_pos && set_iterator == rhs.set_iterator);
}

unsigned BloomapIterator::operator*() {
	/* We need to make sure the iterator works even if not valid, for the foreach macros to work properly. */
	if (isValid()) return *set_iterator;
	else return 0;
}
