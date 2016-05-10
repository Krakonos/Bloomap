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

#include <cstdint>
#include <bitset>
#include <vector>
#include <unordered_set>

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

		void purge();

	protected:
		BloomapFamily *f;

#ifdef DEBUG_STATS
	protected:
		std::unordered_set<unsigned> real_contents;
	public:
		unsigned counter_fp = 0; /* False positive counter */
		unsigned counter_query = 0; /* Query counter */

		void resetStats() {
			counter_fp = 0;
			counter_query = 0;
		}

#endif
};

#endif
