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

#include <unordered_set>

#include "bloomap.h"

class BloomapFamily {
	public:
		BloomapFamily(unsigned m, unsigned k);

		static BloomapFamily* forElementsAndProb(unsigned n, double p);
		static BloomapFamily* forSizeAndFunctions(unsigned m, unsigned k);

		Bloomap* newMap(void);

		unsigned m, k;

		void newElement(unsigned ele, unsigned index);

		void dumpCandidates(void);


	private:
		std::vector< Bloomap* > bloomaps;

		/* Global storage for inserted elements. Let h_1 be the first hash
		 * function, then element e is found in set h_1(e). */
	protected:
		std::vector< std::unordered_set<unsigned> > ins_data;

	friend class Bloomap;
	friend class BloomapIterator;
};

#endif
