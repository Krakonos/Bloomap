#include <iostream>

#include "bloomap.h"
#include "bloomapfamily.h"

using namespace std;

unsigned last_id = 0;

bool fuzz() {
    BloomapFamily *f = BloomapFamily::forElementsAndProb(100, 0.01);

    Bloomap *map1 = f->newMap();
    last_id++;
    cerr << last_id << endl;
    map1->add(last_id);

    for (BloomapIterator it = begin(map1); !it.atEnd(); ++it) {
	if (*it != last_id) return true;
    }
    delete map1;
    delete f;
    return false;
}

int main() {
    /*
    while (1) {
	    if (fuzz()) return 1;
    }
    */

    BloomapFamily *f = BloomapFamily::forElementsAndProb(100, 0.01);

    Bloomap *map1 = f->newMap();

	/*
    for (unsigned i = 1; i < 100; i++) {
	map1->add(i);
    }
	*/

    vector<unsigned> l = {
		 333582338,
		 513937457,
		 711845894,
	    1046370347,
		1402492972,
		1520982030};

	for (vector<unsigned>::iterator it = l.begin(); it != l.end(); ++it) {
		unsigned x = *it;
			map1->add(x-1);
		unsigned h1 = map1->last_index_hash;
			map1->add(x);
		unsigned h2 = map1->last_index_hash;
			map1->add(x+1);
		unsigned h3 = map1->last_index_hash;
		cerr << "Inserting: " << x << " with hashes: " << h1 << ", " << h2 << ", "<<  h3 << "." << endl;
	}

    unsigned max = 0;
    for (BloomapIterator it = begin(map1); !it.atEnd(); ++it) {
	unsigned found = *it;
	cerr << "----> Found: " << found << endl;

	/*
	if ((found - max) != 1) {
	    cerr << "Missing a number!" << endl;
	    ;
	}
	max = found;
	*/
    }

    return 0;
}
