#include <iostream>
#include <map>
#include <vector>
#include <iomanip>
#include <cstdlib>

#include "bloomfilter.h"
#include "bloomapfamily.h"

#ifndef SEED
#define SEED 666
#endif

using namespace std;

BloomapFamily *family;

unsigned empty_prepurge;
unsigned empty_purge;

bool testIteration(unsigned ninsert, unsigned space, double prob, unsigned prefill) {
	/* First, prepare two disjoint sets */
	map<uint64_t,bool> insert1, insert2;
	while (insert1.size() < ninsert) {
		uint64_t n = rand();
		if (insert1.count(n)) continue;
		insert1[n] = true;
	}

	while (insert2.size() < ninsert) {
		uint64_t n = rand();
		if (insert1.count(n) || insert2.count(n)) continue;
		insert2[n] = true;
	}

	/* Preapre family and filters */
	if (!family) {
		family = BloomapFamily::forElementsAndProb(space, prob);
		/* Now, let's pretend to have 2^prefill elements in some map */
		if (prefill) {
			Bloomap *funky = family->newMap();
			for (unsigned i = 0; i < (1U<<prefill); i++) {
				funky->add(i);
			}
		}
	}

	Bloomap *bmap1 = family->newMap();
	for (map<uint64_t,bool>::iterator it = insert1.begin(); it != insert1.end(); ++it) {
		bmap1->add((*it).first);
	}

	Bloomap *bmap2 = family->newMap();
	for (map<uint64_t,bool>::iterator it = insert2.begin(); it != insert2.end(); ++it) {
		bmap2->add((*it).first);
	}
	cout << "==> These maps will be intersected: " << endl;
	bmap1->dumpStats();
	cout << "---" << endl;
	bmap2->dumpStats();

	/* Do the intersection and print stats */
	cout << "==> Results for intersection of two maps, filled with " << ninsert << "elements (out of " << space << "):";
	bmap1->intersect(bmap2);
	if (bmap1->isEmpty()) {
		cout << "empty";
		empty_prepurge++;
	} else {
		cout << "non-empty";
	}
	cout << endl;
	bmap1->dumpStats();
	cout << "--- After purge()" << endl;;
	bmap1->purge();
	bmap1->dumpStats();

	if (bmap1->isEmpty()) { empty_purge++; return true; }
	else return false;
}

void usage(void) {
	cerr << "Usage: ./test insert_elements space_for probability iterations prefill" << endl;
	cerr << "  Where elements in number of elements to be inserted, and probability is the required false-positive rate." << endl;
}

int main(int argc, char* argv[]) {
	
	/* Check for sufficient arguments and their sanity*/
	if (argc != 6) {
		usage();
		return 1;
	}

	unsigned ninsert = atoi(argv[1]);
	unsigned space = atoi(argv[2]);
	double prob = atof(argv[3]);
	unsigned iter = atoi(argv[4]);
	unsigned prefill = atoi(argv[5]);

	/* Allright, we are all set. Let's generate the elements for test. We'll to
	 * this now, because bloomap will use rng and we want the test to be
	 * repeatable.
	 */

	for (unsigned i = 0; i < iter; i++) {
		testIteration(ninsert, space, prob, prefill);
	}

	cout << ":: (pre-purge)   Attempted " << iter << " iterations, " << empty_prepurge << "(" << setprecision(2)<<(100*empty_prepurge/iter) <<"%) of there were empty." << endl;
	cout << ":: (After purge) Attempted " << iter << " iterations, " << empty_purge << "(" << setprecision(2)<<(100*empty_purge/iter) <<"%) of there were empty." << endl;

	return 0;
}
