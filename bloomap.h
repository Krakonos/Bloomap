/******************************************************************************
 * Filename: bloomap.h
 *
 * Created: 2016/05/04 20:46
 * Author: Ladislav Láska
 * e-mail: laska@kam.mff.cuni.cz
 *
 ******************************************************************************/

#ifndef __BLOOMAP_H__
#define __BLOOMAP_H__

#include <stdint.h>
#include <bitset>
#include <vector>
#include <set>
#include <iterator>

#include "bloomfilter.h"


class BloomapFamily;

class Bloomap {
	public:
		Bloomap(BloomapFamily* f, unsigned m, unsigned k);
		Bloomap(Bloomap *orig);
		void _init(unsigned _ncomp, unsigned _compsize, unsigned _nfunc);

		/* Most common operations */
		bool add(unsigned ele);
		bool add(Bloomap *map);
		bool contains(unsigned ele);
		bool isEmpty(void);
		void clear(void);
		Bloomap* intersect(Bloomap* map);
		Bloomap* or_from(Bloomap *filter);

		/* Purges the map according to the family records. */
		void purge();

		/* Split this map from the family. 
		 * Can not be reversed! */
		void splitFamily(void);

		/* Helper function to compute hash */
		unsigned hash(unsigned ele, unsigned i);

		BloomapFamily* family() { return f; }

		/* Comparison operators */
		bool operator==(const Bloomap* rhs);
		bool operator!=(const Bloomap* rhs);

		/* Debugging and slow stuff */
		void dump(void);
		void dumpStats(void);
		unsigned popcount(void);
		unsigned mapsize(void);

	protected:
		unsigned nfunc, compsize, compsize_shiftbits, ncomp, bits_segsize, bits_size;
		BloomapFamily *f;
		BITS_TYPE* bits;
		BITS_TYPE* side_index;

		/* The data manipulation functions. The class-wide changed flag is
		 * used, and has to be reset by it's user. */
		bool changed;
		bool inline set(unsigned comp, unsigned bit) {
			unsigned index = comp*bits_segsize + bit / sizeof(BITS_TYPE);
			BITS_TYPE mask = 1 << (bit % sizeof(BITS_TYPE));
			changed |= !(bits[index] & mask);
			bits[index] |= mask;
			return changed;
		}

		bool inline reset(unsigned comp, unsigned bit) {
			unsigned index = comp*bits_segsize + bit / sizeof(BITS_TYPE);
			BITS_TYPE mask = 1 << (bit % sizeof(BITS_TYPE));
			changed |= bits[index] & mask;
			bits[index] &= ~mask;
			return changed;
		}

		bool inline get(unsigned comp, unsigned bit) const {
			unsigned index = comp*bits_segsize + bit / sizeof(BITS_TYPE);
			BITS_TYPE mask = 1 << (bit % sizeof(BITS_TYPE));
			return bits[index] & mask;
		}

	friend class BloomapIterator;
#ifdef DEBUG_STATS
	protected:
		std::set<unsigned> real_contents;
	public:
		unsigned counter_fp; /* False positive counter */
		unsigned counter_query; /* Query counter */

		void resetStats(void);

#endif
};

class BloomapIterator : public std::iterator<std::input_iterator_tag, unsigned > {
	public:
		BloomapIterator(const BloomapIterator& orig);
		BloomapIterator(Bloomap *map, bool end = false);
		void _init(Bloomap *map, bool end = false);
		BloomapIterator(Bloomap *map, unsigned& first);
		BloomapIterator& operator++();
		BloomapIterator operator++(int);
		bool operator==(const BloomapIterator& rhs);
		bool operator!=(const BloomapIterator& rhs);
		unsigned operator*();
		bool isValid(void);

//		BloomapIterator(const BloomapIterator& mit) : p(mit.p) {}

	protected:
		Bloomap* map;
		unsigned glob_pos; /* A position in the global vector of sets. */
		std::set<unsigned>::iterator set_iterator;
		bool advanceSetIterator(void);
};

BloomapIterator begin(Bloomap *map);
BloomapIterator end(Bloomap *map);

#define BLOOMAP_FOR_EACH(VAR, MAP) for (BloomapIterator _bloomap_it_##MAP(MAP,VAR); _bloomap_it_##MAP != end(MAP); _bloomap_it_##MAP++, VAR=*_bloomap_it_##MAP)

#endif
