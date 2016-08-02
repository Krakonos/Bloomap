/******************************************************************************
 * Filename: bloomapfamily.h
 *
 * Created: 2016/05/10 20:47
 * Author: Ladislav Láska
 * e-mail: laska@kam.mff.cuni.cz
 *
 ******************************************************************************/

#ifndef __BLOOMAPFAMILY_H__
#define __BLOOMAPFAMILY_H__

#include <stdint.h>
#include <vector>
#include <iterator>

class Bloomap;
class BloomapFamily;

class BloomapFamilyIterator : public std::iterator<std::input_iterator_tag, unsigned > {
	public:
		BloomapFamilyIterator() : family(NULL), hash(0), pmajor(0), pminor(0), flagAtEnd(true) { };
		BloomapFamilyIterator(BloomapFamily *family, unsigned hash, bool flagAtEnd = false);
		BloomapFamilyIterator& operator++();
		BloomapFamilyIterator operator++(int);
		bool operator==(const BloomapFamilyIterator& rhs);
		bool operator!=(const BloomapFamilyIterator& rhs);
		bool atEnd();
		unsigned operator*();
	protected:
		BloomapFamily *family;
		unsigned hash;
		/* The position in the bit array, major being the higher bits and minor the lower ones. */
		unsigned pmajor;
		unsigned pminor;
		bool flagAtEnd;
};

class BloomapFamily {
	public:
		BloomapFamily(unsigned m, unsigned k);

		static BloomapFamily* forElementsAndProb(unsigned n, double p);
		static BloomapFamily* forSizeAndFunctions(unsigned m, unsigned k);

		Bloomap* newMap(void);

		unsigned m, k;

		/* Inserts a new element and returns it's hash */
		unsigned newElement(unsigned ele);

		void dumpCandidates(void);

		BloomapFamilyIterator begin(unsigned hash) { return BloomapFamilyIterator(this, hash); }
		BloomapFamilyIterator end(void)			   { return BloomapFamilyIterator(this, 0, true); }


	private:
		std::vector< Bloomap* > bloomaps;

		/* Global storage for inserted elements. Let h_1 be the first hash
		 * function, then element e is found in set h_1(e). */
	protected:
		std::vector< uint64_t > index_data;
		const unsigned index_logsize;

	friend class Bloomap;
	friend class BloomapIterator;
	friend class BloomapFamilyIterator;
};

#endif
