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

class Bloomap : public BloomFilter {
	public:
		Bloomap(BloomapFamily* f, unsigned m, unsigned k);
		Bloomap(Bloomap *orig);

		bool add(unsigned ele);
		bool contains(unsigned ele);
		void dump(void);
		void dumpStats(void);
		void clear(void);

		BloomapFamily* family() { return f; }

		/* Logic functions available. These might be provided as operators in
		 * the future. */
		Bloomap* intersect(Bloomap* map);
		//Bloomap* unite(Bloomap* map);

		bool operator==(const Bloomap* rhs);
		bool operator!=(const Bloomap* rhs);

		void purge();

	protected:
		BloomapFamily *f;

#ifdef DEBUG_STATS
	protected:
		std::set<unsigned> real_contents;
	public:
		unsigned counter_fp; /* False positive counter */
		unsigned counter_query; /* Query counter */

		void resetStats() {
			counter_fp = 0;
			counter_query = 0;
		}

	friend class BloomapIterator;

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
