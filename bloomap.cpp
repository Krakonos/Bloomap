#include <iostream>
#include <map>
#include <vector>
#include <cassert>
#include <cstdlib>
#include <cstring>

#include "murmur.h"
#include "bloomap.h"
#include "bloomapfamily.h"

/* This is static vector of hash functions. There are in fact seeds for the
 * murmur hash function, and are generated in constructor randomly, as needed */
static std::vector<uint32_t> hashfn_a;
static std::vector<uint32_t> hashfn_b;

Bloomap::Bloomap(BloomapFamily* f, unsigned m, unsigned k) : f(f)
{
	_init(k, m/k, 1);
#ifdef DEBUG_STATS
	resetStats();
#endif
}

Bloomap::Bloomap(Bloomap *orig) {

	_init(orig->ncomp, orig->compsize, orig->nfunc);

	for (unsigned comp = 0; comp < ncomp; comp++) {
		for (unsigned pos = 0; pos < orig->compsize; pos++) {
			bits[comp*bits_segsize + pos] = orig->bits[comp*bits_segsize + pos];
		}
	}
}

void Bloomap::_init(unsigned _ncomp, unsigned _compsize, unsigned _nfunc) {
	ncomp = _ncomp;
	compsize = _compsize;
	nfunc = _nfunc;

	assert(ncomp);
	assert(compsize);
	assert(nfunc);

	/* Round the compsize to the next power of two */
	for (unsigned i = 0; i < 32; i++) {
		if (compsize < (1U << i)) {
			compsize = 1 << i;
			compsize_shiftbits = 32-i;
			break;
		}
	}

	/* Generate seeds for all the hash functions */
	while (hashfn_a.size() < nfunc*ncomp) {
		/* TODO: Possibly ensure different functions */
		unsigned a = rand();
		while (a == 0) a = rand();
		hashfn_a.push_back(a);
		hashfn_b.push_back(rand());
	}

	/* Generate compartments */
	bits_segsize = (compsize / sizeof(BITS_TYPE))+1;
	bits_size = bits_segsize*ncomp;
	bits = new BITS_TYPE[bits_size];
	memset(bits, 0, bits_size*sizeof(BITS_TYPE));
}

bool Bloomap::add(unsigned ele) {
#ifdef DEBUG_STATS
	real_contents.insert(ele);
#endif
	if (f)
		f->newElement(ele, hash(ele, 0)); /* This can't be conditional, as we need even collided elements! */

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

bool Bloomap::add(Bloomap *map) {
	changed = false;
	for (unsigned comp = 0; comp < ncomp; comp++) {
		for (unsigned i = 0; i < bits_segsize; i++) {
			unsigned index = comp*bits_segsize + i;
			if ((bits[index] & map->bits[index]) != map->bits[index]) {
				changed = true;
				bits[index] |= map->bits[index];
			}
		}
	}
	return changed;
}

bool Bloomap::contains(unsigned ele) {
	bool ret = true;
	unsigned fn = 0;
	for (unsigned comp = 0; comp < ncomp; comp++) {
		for (unsigned i = 0; i < nfunc; i++) {
			uint32_t h = hash(ele, fn++);
			if (!get(comp,h)) {
				ret = false;
				goto stat_and_ret;
			}
		}
	}
	stat_and_ret:
#ifdef DEBUG_STATS
	counter_query++;
	if (ret && !real_contents.count(ele))
		counter_fp++;
#endif
	return ret;
}

bool Bloomap::isEmpty(void) {
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

void Bloomap::clear(void) {
	memset(bits, 0, bits_size*sizeof(BITS_TYPE));
}

Bloomap* Bloomap::intersect(Bloomap* map) {
	for (unsigned comp = 0; comp < ncomp; comp++) {
		for (unsigned i = 0; i < bits_segsize; i++) {
			bits[comp*bits_segsize + i] &= map->bits[comp*bits_segsize + i];
		}
	}
	return this;
}

Bloomap* Bloomap::or_from(Bloomap *filter) {
	changed = false;
	for (unsigned comp = 0; comp < ncomp; comp++) {
		for (unsigned i = 0; i < bits_segsize; i++) {
			bits[comp*bits_segsize + i] |= filter->bits[comp*bits_segsize + i];
		}
	}
	return this;
}

void Bloomap::purge() {
	Bloomap orig(this);
	clear();
	for (unsigned glob_pos = 0; glob_pos < compsize; glob_pos++) {
		if (!orig.get(0,glob_pos)) continue;
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

void Bloomap::splitFamily(void) {
	f = NULL;
}

unsigned Bloomap::hash(unsigned ele, unsigned i) {
	unsigned p = 767461883;
	//return (ele*hashfn_a[i] + hashfn_b[i]) >> compsize_shiftbits;
	return ((ele*hashfn_a[i] + hashfn_b[i]) %p) % compsize;
	//return MurmurHash1(ele, hash_functions[i]) % compsize;
}

#ifdef DEBUG_STATS
void Bloomap::resetStats(void) {
	counter_fp = 0;
	counter_query = 0;
}
#endif

void Bloomap::dump(void) {
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

void Bloomap::dumpStats(void) {
	std::cerr << "  Map compartments:       " << ncomp << std::endl;
	std::cerr << "  Map size (in bits):     " << mapsize() << std::endl;
	std::cerr << "  Map popcount (in bits): " << popcount()<< std::endl;
	std::cerr << "  Map popcount (ratio):   " << popcount()*1.0/mapsize() << std::endl;
	std::cerr << "  Empty:                  " << (isEmpty() ? "yes" : "no") << std::endl;

}

unsigned Bloomap::popcount(void) {
	unsigned count = 0;
	for (unsigned comp = 0; comp < ncomp; comp++) {
		for (unsigned i = 0; i < compsize; i++) {
			if (get(comp,i)) count++;
		}
	}
	return count;
}

unsigned Bloomap::mapsize(void) {
	return bits_size*sizeof(BITS_TYPE);
}

/* Bloomap operators */

bool Bloomap::operator==(const Bloomap* rhs) {
	/* Trivial cases */
	if (rhs == NULL) return false;
	if (this == rhs) return true;
	if (f != rhs->f) return false;

	/* If this passes, let's check the bits */
	if (ncomp != rhs->ncomp || compsize != rhs->compsize || nfunc != rhs->nfunc) return false;
	for (unsigned comp = 0; comp < ncomp; comp++) {
		for (unsigned i = 0; i < bits_segsize; i++) {
			if (rhs->bits[comp*bits_segsize + i] != bits[comp*bits_segsize + i]) return false;
		}
	}
	return true;
}

bool Bloomap::operator!=(const Bloomap* rhs) {
	return !operator==(rhs);
}

/* Bloomap iterators */

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

BloomapIterator BloomapIterator::operator++(int x) {
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

