#include <cmath>
#include <cassert>
#include <iostream>

#include "bloomapfamily.h"
#include "bloomap.h"

unsigned round_to_log(unsigned x) {
	unsigned il = 0;
	while (x > (1U << il)) il++;
	return il;
}

BloomapFamily::BloomapFamily(unsigned m, unsigned k)
	: m(m), k(k), index_logsize(round_to_log(m))
{
}

/* Convenience functions to create right families depending on the needs */
BloomapFamily* BloomapFamily::forElementsAndProb(unsigned n, double p) {
	unsigned m = ceil((n * log(p)) / log(1.0 / (pow(2.0, log(2.0)))));
	unsigned k = round(log(2.0) * m / n);
	assert(m);
	assert(k);

	return new BloomapFamily(m, k);
}

BloomapFamily* BloomapFamily::forSizeAndFunctions(unsigned m, unsigned k) {
	return new BloomapFamily(m, k);
}

/* Create and return a new map from this family */
Bloomap* BloomapFamily::newMap(void) {
	Bloomap* nm = new Bloomap(this, m, k, index_logsize);
	bloomaps.push_back(nm);
	return nm;
}

unsigned BloomapFamily::newElement(unsigned e) {
	/* Variable index_logsize specifies the index table size, as log_2. Here we
	 * want to extract some bits to store.
	 *
	 * Let's assume the elements inserted will be condensed a bit. Be it 4 bits.
	 *
	 * Note that changing this function requires changing the BloomapFamilyIterator below.
	 * */

	unsigned bits_condensed = 6; /* Because the type is uint64_t */
	unsigned hash_mask = (1 << index_logsize) - 1;
	unsigned condensed_mask = (1 << bits_condensed) -1;
	unsigned hash = (e >> bits_condensed) & hash_mask;
	assert(hash < (1U << index_logsize));
	unsigned ip = e >> (bits_condensed);
	if (ip >= index_data.size()) {
		index_data.resize(ip+1, 0); /* TODO: Possibly check if the vector reserves a lot more space */
	}

	index_data[ip] |= (1ULL << (e & condensed_mask));
	//std::cerr << "index_data[" << ip << "] |= " << (1ULL << (e & condensed_mask)) << std::endl;
	return hash;
}

void BloomapFamily::dumpCandidates(void) {
}

/* Iterator */
BloomapFamilyIterator::BloomapFamilyIterator(BloomapFamily *family, unsigned hash, bool flagAtEnd)
	: family(family), hash(hash), pmajor(hash), pminor(0), flagAtEnd(flagAtEnd)
{
	//std::cerr << "New iterator. hash=" << hash << std::endl;
	/* If the first element isn't in the family, call the ++() to find one */
	if ((family->index_data[hash] & 1) == 0)
		operator++();
}

bool BloomapFamilyIterator::atEnd(void) {
	return flagAtEnd;
}

BloomapFamilyIterator& BloomapFamilyIterator::operator++() {
	//std::cerr << "++" << std::endl;
	/* If we are past the end */
	if (pmajor >= family->index_data.size()) {
		flagAtEnd = true;
		return *this;
	}
	uint64_t current_data = family->index_data[pmajor] >> pminor;
	//std::cerr << "index_data[" << pmajor << "] = " << current_data << std::endl;
	do {
		if (!current_data) {
			pminor = 0;
			/* Let's break out if this was the last place */
			pmajor += (1 << family->index_logsize);
			if (pmajor >= family->index_data.size()) {
				flagAtEnd = true;
				break;
			}
			current_data = family->index_data[pmajor];
			//if (current_data)
				//std::cerr << "index_data[" << pmajor << "] = " << current_data << std::endl;
		} else {
			current_data >>= 1;
			pminor++;
		}
	} while (0 == (current_data & 1));
	return *this;
}

BloomapFamilyIterator BloomapFamilyIterator::operator++(int) {
	BloomapFamilyIterator tmp(*this);
	operator++();
	return tmp;
}

/* Warning: if the end() iterators for different hashes are the same! */
bool BloomapFamilyIterator::operator==(const BloomapFamilyIterator& rhs) {
	return (
			(family == rhs.family) &&
			(
				((hash == rhs.hash) && (pminor == rhs.pminor) && (pmajor == rhs.pmajor))
				||
				(flagAtEnd == rhs.flagAtEnd)
			)
		   );
}

bool BloomapFamilyIterator::operator!=(const BloomapFamilyIterator& rhs) {
	return !operator==(rhs);
}

unsigned BloomapFamilyIterator::operator*() {
	if (flagAtEnd) return 0xdeadbeef;
	assert(pminor < 64);
	assert(pmajor < family->index_data.size());
	/* Stak the data together: pmajor . hash . pminor */
	unsigned val = (pmajor << 6) | pminor;
	//std::cerr << "Candidate: " << val << std::endl;
	return val;
}
