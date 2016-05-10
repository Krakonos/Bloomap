#include <iostream>
#include <map>
#include <vector>

#include "bloomfilter.h"
#include "bloomapfamily.h"

#ifndef SEED
#define SEED 666
#endif

using namespace std;

void usage(void) {
	cerr << "Usage: ./test insert_elements check_elements probability" << endl;
	cerr << "  Where elements in number of elements to be inserted, and probability is the required false-positive rate." << endl;
}

BloomapFamily *family;

int main(int argc, char* argv[]) {
	
	/* Check for sufficient arguments and their sanity*/
	if (argc != 4) {
		usage();
		return 1;
	}

	unsigned ninsert = atoi(argv[1]);
	unsigned ncheck = atoi(argv[2]);
	double prob = atof(argv[3]);

	/* Allright, we are all set. Let's generate the elements for test. We'll to
	 * this now, because bloomap will use rng and we want the test to be
	 * repeatable.
	 */

	srand(SEED);
	map<uint64_t,bool> insert, check;
	while (insert.size() < ninsert) {
		uint64_t n = rand();
		if (insert.count(n)) continue;
		insert[n] = true;
	}

	while (check.size() < ncheck) {
		uint64_t n = rand();
		if (insert.count(n) || check.count(n)) continue;
		check[n] = true;
	}

	family = BloomapFamily::forElementsAndProb(ninsert, prob);

	/* Create the map and counters for test results */
	Bloomap *bmap = family->newMap();
	unsigned insert_found = 0, check_found = 0;

	/* Insert all the elements */
	for (const auto &pair : insert) {
		bmap->add(pair.first);
	}

	/* Check the inserted elements, they should all be there */
	for (const auto &pair : insert) {
		if (bmap->contains(pair.first)) insert_found++;
	}

	bmap->resetStats();


	/* Check the set that should not be contained */
	for (const auto &pair : check) {
		if (bmap->contains(pair.first)) check_found++;
	}
	
	if (insert_found != ninsert) {
		cerr << "Whoa! There were less elements found than inserted. The bloomap probably has some bugs!" << endl;
	}

	double fp_rate = (1.0*check_found)/ncheck;
	
	bmap->dump();
	cerr << "ninsert\tncheck\tins_f\tchk_f\tfp_rate\tfp_target\tpop\tpop_rate" << endl;
	cout << ninsert << "\t" << ncheck << "\t" << insert_found << "\t" << check_found << "\t" << fp_rate << "\t" << prob << endl;
	check_found = bmap->counter_fp;
	ncheck = bmap->counter_query;
	fp_rate = (1.0*check_found)/ncheck;
	cout << ninsert << "\t" << ncheck << "\t" << insert_found << "\t" << check_found << "\t" << fp_rate << "\t" << prob << "\t" << bmap->popcount()  << "\t" << (bmap->popcount()*1.0/ bmap->mapsize()) << endl;
	return 0;
}
