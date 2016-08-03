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
#include <vector>
#include <set>
#include <iterator>
#include <cassert>

#include "bloomapfamily.h"

#define BITS_TYPE uint64_t
#define BITS_WORD (sizeof(BITS_TYPE)*8)


class BloomapFamily;
class BloomapFamilyIterator;

class Bloomap {
	public:
		Bloomap(BloomapFamily* f, unsigned m, unsigned k, unsigned index_logsize);
		Bloomap(Bloomap *orig);
		~Bloomap();
		void _init(unsigned _ncomp, unsigned _compsize, unsigned _nfunc, unsigned _index_logsize);

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

		/* Side index, only used if part of a family */
		BITS_TYPE* side_index;
		unsigned index_logsize;
		unsigned index_size;

		/* The data manipulation functions. The class-wide changed flag is
		 * used, and has to be reset by it's user. */
		bool changed;
		bool inline set(unsigned comp, unsigned bit) {
			unsigned index = comp*bits_segsize + bit / BITS_WORD;
			assert (index < ((comp+1)*bits_segsize));
			BITS_TYPE mask = ((BITS_TYPE) 1) << (bit % BITS_WORD);
			changed |= !(bits[index] & mask);
			bits[index] |= mask;
			return changed;
		}

		bool inline reset(unsigned comp, unsigned bit) {
			unsigned index = comp*bits_segsize + bit / BITS_WORD;
			BITS_TYPE mask = ((BITS_TYPE) 1) << (bit % BITS_WORD);
			changed |= bits[index] & mask;
			bits[index] &= ~mask;
			return changed;
		}

		bool inline get(unsigned comp, unsigned bit) const {
			unsigned index = comp*bits_segsize + bit / BITS_WORD;
			BITS_TYPE mask = ((BITS_TYPE) 1) << (bit % BITS_WORD);
			return !!(bits[index] & mask);
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
		bool atEnd(void);

//		BloomapIterator(const BloomapIterator& mit) : p(mit.p) {}

	protected:
		Bloomap* map;
		unsigned current_hash; /* A position in the global vector of sets. */
		bool flagAtEnd;

		BloomapFamilyIterator chi; /* current_hash_iterator */
		bool advanceHashIterator(void);
		bool findNextHash(void);
};

BloomapIterator begin(Bloomap *map);
BloomapIterator end(Bloomap *map);

#define BLOOMAP_FOR_EACH(VAR, MAP) for (BloomapIterator _bloomap_it(MAP,VAR); _bloomap_it != end(MAP); _bloomap_it++, VAR=*_bloomap_it)

#endif
