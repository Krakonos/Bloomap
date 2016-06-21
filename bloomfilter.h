/******************************************************************************
 * Filename: bloomfilter.h
 *
 * Created: 2016/05/10 20:34
 * Author: Ladislav Láska
 * e-mail: laska@kam.mff.cuni.cz
 *
 ******************************************************************************/

#ifndef __BLOOMFILTER_H__
#define __BLOOMFILTER_H__

#include <cstdint>
#include <bitset>
#include <vector>

class BloomFilter {
	public:
		BloomFilter(unsigned m, unsigned k);
		/* Constructor takes arguments:
		 * 	ncomp		number of compartments in the bloom filter
		 * 	compsize	size of each compartment, in bits
		 * 	nfunc		number of hash functions used in each compartment
		 */
		BloomFilter(unsigned ncomp, unsigned compsize, unsigned _nfunc);
		BloomFilter(BloomFilter *orig);
		~BloomFilter();

		bool add(unsigned ele);
		bool contains(unsigned ele);
		void dump(void);

		/* Helper function to compute hash */
		unsigned hash(unsigned ele, unsigned i);

		/* Returns number of bits set in filter */
		unsigned popcount(void);

		/* Returns the size of map in bits (ignoring all overhead) */
		unsigned mapsize(void);

		/* Checks if the filter is empty */
		bool isEmpty(void);

		BloomFilter* intersect(BloomFilter *filter);
		BloomFilter* or_from(BloomFilter *filter);

	protected:
		unsigned nfunc, compsize, ncomp;
		std::vector< std::vector<bool> > bits;
};

#endif
