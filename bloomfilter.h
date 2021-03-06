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

#include <stdint.h>
#include <bitset>
#include <vector>

#define BITS_TYPE unsigned

class BloomFilter {
	public:
		BloomFilter(unsigned m, unsigned k);
		/* Constructor takes arguments:
		 * 	ncomp		number of compartments in the bloom filter
		 * 	compsize	size of each compartment, in bits
		 * 	nfunc		number of hash functions used in each compartment
		 */
		BloomFilter(unsigned ncomp, unsigned compsize, unsigned _nfunc);
		void _init(unsigned _ncomp, unsigned _compsize, unsigned _nfunc);
		BloomFilter(BloomFilter *orig);
		~BloomFilter();

		bool add(unsigned ele);
		bool add(BloomFilter *filter);
		bool contains(unsigned ele);
		void dump(void);
		void clear(void);

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

		bool operator==(const BloomFilter* rhs);
		bool operator!=(const BloomFilter* rhs);

	protected:
		unsigned nfunc, compsize, compsize_shiftbits, ncomp, bits_segsize, bits_size;
		BITS_TYPE* bits;

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

};

#endif
