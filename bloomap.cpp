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

Bloomap::Bloomap(BloomapFamily* f, unsigned m, unsigned k, unsigned index_logsize)
	: f(f)
{
	_init(k, m/k, 1, index_logsize);
#ifdef DEBUG_STATS
	resetStats();
#endif
}

Bloomap::Bloomap(Bloomap *orig) {

	_init(orig->ncomp, orig->compsize, orig->nfunc, orig->index_logsize);

	specials = orig->specials;
	for (unsigned comp = 0; comp < ncomp; comp++) {
		for (unsigned pos = 0; pos < orig->compsize; pos++) {
			bits[comp*bits_segsize + pos] = orig->bits[comp*bits_segsize + pos];
		}
	}
}

void Bloomap::_init(unsigned _ncomp, unsigned _compsize, unsigned _nfunc, unsigned _index_logsize) {
	ncomp = _ncomp;
	compsize = _compsize;
	nfunc = _nfunc;
	index_logsize = _index_logsize;
	specials = 0x0;

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

	/* Generate compartments.*/
	//std::cerr << "Compsize is: " << compsize << std::endl;
	bits_segsize = (compsize / BITS_WORD);
	//std::cerr << "Segment is: " << bits_segsize << std::endl;
	bits_size = bits_segsize*ncomp;

	/* If we are in a family, append a few more bits for the side index. */
	if (f) {
		index_size = ((1 << index_logsize) / BITS_WORD)+1;
		bits_size += index_size;
	} else {
		side_index = NULL;
	}

	bits = new BITS_TYPE[bits_size];
	memset(bits, 0, bits_size*sizeof(BITS_TYPE));

	/* Make a pointer into the side index, just for convenience. */
	if (f) {
		side_index = bits + (bits_size - index_size);
	}

}

Bloomap::~Bloomap() {
	delete[] bits;
	/* Side index is actually inside bits, don't try to delete it! */
}

bool Bloomap::add(unsigned ele) {
#ifdef DEBUG_STATS
	real_contents.insert(ele);
#endif
	unsigned last_index_hash = 0;
	if (f) {
		last_index_hash = f->newElement(ele);
		assert(last_index_hash < (1U << index_logsize));
		unsigned side_i = last_index_hash / BITS_WORD;
		side_index[side_i] |= ((BITS_TYPE) 1) << (last_index_hash % BITS_WORD );
		//std::cerr << "side_index[" << side_i << "] |= " << (1 << (last_index_hash % (sizeof(BITS_TYPE)*8) )) << std::endl;
	}

	changed = false;
	if (ele < sizeof(specials)*CHAR_BIT) {
		SPECIALS_TYPE mask = 0x1 << ele;
		if (specials & mask) changed = false;
		else specials |= mask;
		return changed;
	}
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
	if (specials != map->specials) changed = true;
	specials |= map->specials;
	for (unsigned i = 0; i < bits_size; i++) {
		if ((bits[i] & map->bits[i]) != map->bits[i]) {
			changed = true;
			bits[i] |= map->bits[i];
		}
	}
	return changed;
}

bool Bloomap::contains(unsigned ele) {
	if (ele < sizeof(specials)*CHAR_BIT) {
		SPECIALS_TYPE mask = 0x1 << ele;
		return (specials & mask);
	}
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
	if (specials) return false;
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
	specials = 0;
	memset(bits, 0, bits_size*sizeof(BITS_TYPE));
}

Bloomap* Bloomap::intersect(Bloomap* map) {
	specials &= map->specials;
	for (unsigned i = 0; i < bits_size; i++) {
		bits[i] &= map->bits[i];
	}
	return this;
}

bool Bloomap::isIntersectionEmpty(Bloomap* map) {
	if (specials & map->specials) return false;
	for (unsigned comp = 0; comp < ncomp; comp++) {
		bool empty = true;
		for (unsigned i = 0; i < bits_segsize; i++) {
		    	unsigned index = comp*bits_segsize + i;
			if (bits[i] & map->bits[i]) {
				empty = false;
				break;
			}
		}
		if (empty) return true;
	}
	return false;
}

Bloomap* Bloomap::or_from(Bloomap *filter) {
	assert(this != filter);
	specials |= filter->specials;
	BITS_TYPE*__restrict from = filter->bits;
	BITS_TYPE*__restrict to = bits;
	changed = false;
	for (unsigned i = 0; i < bits_size; i++) {
		to[i] |= from[i];
	}
	return this;
}

void Bloomap::purge() {
	Bloomap orig(this);
	clear();
	for (BloomapIterator it = begin(this); !it.atEnd(); ++it) {
		if (orig.contains(*it))
			add(*it);
	}
}

void Bloomap::splitFamily(void) {
	f = NULL;
}

unsigned Bloomap::hash(unsigned ele, unsigned i) {
	//unsigned p = 767461883;
	uint32_t h = (ele*hashfn_a[i] + hashfn_b[i]);
	h >>= compsize_shiftbits;
	return h;
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
	cerr << "  specials=" << specials << endl;
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
	unsigned count = __builtin_popcount(specials);
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
	if (specials != rhs->specials) return false;

	/* If this passes, let's check the bits */
	if (ncomp != rhs->ncomp || compsize != rhs->compsize || nfunc != rhs->nfunc) return false;
	for (unsigned i = 0; i < bits_size; i++) {
		if (rhs->bits[i] != bits[i]) return false;
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
	current_hash = orig.current_hash;
	chi = orig.chi;
}

BloomapIterator::BloomapIterator(Bloomap *map, bool end) {
	_init(map,end);
}

void BloomapIterator::_init(Bloomap *_map, bool end) {
	map = _map;
	/* We are creating the "end" iterator */
	flagAtEnd = end;
	current_hash = 0;
	if (!flagAtEnd) {
		if (map->side_index[0] & 1)
			chi = map->family()->begin(current_hash);
		else findNextHash();
	}
}

BloomapIterator::BloomapIterator(Bloomap *map, unsigned& first)
{
	_init(map);
	first = operator*();
}

bool BloomapIterator::atEnd(void) {
	return flagAtEnd;
}

bool BloomapIterator::isValid(void) {
	return !chi.atEnd();
}

/* Advance the iterator and return true if it's dereferencable */
bool BloomapIterator::advanceHashIterator(void) {
	if (isValid())
		chi++;
	/* Return false if the iterator changed to past-the-end as well! */
	return isValid();
}

bool BloomapIterator::findNextHash(void) {
	unsigned side_i;
	BITS_TYPE side_mask;
	do {
		current_hash++;
		side_i = current_hash / BITS_WORD;
		//std::cerr << "Scanning hash: " << current_hash << ", index: " << side_i << std::endl;
		if (side_i >= map->index_size) {
			//std::cerr << "Ended: " << side_i << std::endl;
			flagAtEnd = true;
			return false; /* There is no next hash */
		}
		side_mask = (((BITS_TYPE) 1) << (current_hash % BITS_WORD));
		/* We are at the first bit, check if the byte is empty */
		if ((side_mask == 1) && (map->side_index[side_i] == 0)) {
			/* The -1 is here to compensate current_hash++ at the start of the loop. */
			current_hash += sizeof(BITS_TYPE)*8-1;
		//	std::cerr << "Skipping empty block." << std::endl;
		}
	} while ((map->side_index[side_i] & side_mask) == 0 );
	//std::cerr << "Found next hash: " << current_hash << std::endl;
	chi = map->f->begin(current_hash);
	return true;
}

BloomapIterator& BloomapIterator::operator++() {
	while (1) {
		/* Advance the iterator. If it's dereferenceable, check if it's in the map. Otherwise continue to the next element. */
		if (advanceHashIterator()) {
			if (map->contains(*chi)) return *this;
			else continue;
		}

		/* If chi is at end, find next hash. If even this fails, set flag and exit.  */
		if (chi.atEnd() && (!findNextHash())) {
			//std::cerr << "++ end." << std::endl;
			flagAtEnd = true;
			break;
		} else {
			/* The chi is either not at the end, or we have found next hash.
			 * Check the first element before we iterate further. */
			if (map->contains(*chi)) return *this;
		}
	}
	return *this;
}

BloomapIterator BloomapIterator::operator++(int/*unused: x*/) {
	BloomapIterator tmp(*this);
	operator++();
	return tmp;
}

bool BloomapIterator::operator==(const BloomapIterator& rhs) {
	if (flagAtEnd && rhs.flagAtEnd) return true;
	return (map == rhs.map && current_hash == rhs.current_hash && chi == rhs.chi);
}

bool BloomapIterator::operator!=(const BloomapIterator& rhs) {
	return !operator==(rhs);
	//return !(map == rhs.map && glob_pos == rhs.glob_pos && set_iterator == rhs.set_iterator);
}

unsigned BloomapIterator::operator*() {
	/* We need to make sure the iterator works even if not valid, for the foreach macros to work properly. */
	if (isValid()) return *chi;
	else return 6666;
}

