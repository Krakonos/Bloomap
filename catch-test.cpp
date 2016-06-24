#include <catch/catch.hpp>
#include <vector>
#include <map>

#include "bloomap.h"
#include "bloomapfamily.h"
#include "bloomfilter.h"

#define ELE 1000
#define ITER 10
#define SLACK 1.2

using namespace std;
typedef map<uint64_t,bool> Contents;

Contents bloomap_fill(Bloomap* map, unsigned count, unsigned seed = 0) {
	if (seed) srand(seed);
	Contents insert;
	for (unsigned i = 0; i < count; i++) {
		unsigned e = rand();
		while (insert.count(e)) e = rand(); /* Don't insert value twice */
		map->add( e );
		insert[e] = true;
	}
	return insert;
}

unsigned bloomap_count_elements(Bloomap* map, Contents& list) {
	unsigned insert_found = 0;
	for (const auto& pair : list ) {
		if (map->contains(pair.first)) insert_found++;
	}
	return insert_found;
}

bool bloomap_check_fp_rate(Bloomap* map, Contents& list, double exp_rate) {
	/* We might go ahead and compute variance, but something simple should be allright for testing */
	unsigned fp_count = 0;
	unsigned samples = 10*ELE;
	for (unsigned i = 0; i < samples; i++) {
		unsigned e = rand();
		while (list.count(e)) e = rand(); /* Don't count elements known to be in the map. */
		if (map->contains(e)) fp_count++;
	}
	double fp_rate = 1.0*fp_count / samples;
	return ( fp_rate < exp_rate );
}

/* Insert some random bogus elements into the map, to fill the sidelist. */
void bloomap_messup(BloomapFamily *f, unsigned mess_factor) {
	Bloomap* dummy_map = f->newMap();
	bloomap_fill(dummy_map, mess_factor*ELE);
	delete dummy_map;
}


TEST_CASE( "****** Elementary Bloomap operations." ) {
	BloomapFamily *f = BloomapFamily::forElementsAndProb(ELE, 0.01);
	Bloomap* map1 = f->newMap();

	REQUIRE (map1 != NULL);

	REQUIRE (map1->popcount() == 0);

	SECTION("--> fill with random elements and check them") {
		for (unsigned i = 0; i < ITER; i++) {
			map1->clear();
			Contents c = bloomap_fill(map1, ELE);
			REQUIRE( bloomap_count_elements(map1, c) == ELE );
		}
	}

	SECTION("--> clear() and check") {
		bloomap_fill(map1, ELE);
		map1->clear();
		REQUIRE (map1->popcount() == 0);
	}

	SECTION("--> FP rate is acceptable") {
		for (unsigned i = 0; i < ITER; i++) {
			Contents c = bloomap_fill(map1, ELE);
			REQUIRE( bloomap_check_fp_rate(map1, c, 0.01*SLACK) ); /* Allow ourselves some slack, it's randomized and bad stuff happens */
			map1->clear();
		}
	}
}

TEST_CASE( "***** Bloomap union.", "[union]" ) {
	BloomapFamily *f = BloomapFamily::forElementsAndProb(ELE, 0.01);
	Bloomap* map1 = f->newMap();
	Bloomap* map2 = f->newMap();

	REQUIRE (map1 != NULL);
	REQUIRE (map2 != NULL);

	REQUIRE (map1->popcount() == 0);
	REQUIRE (map2->popcount() == 0);

	Contents c1 = bloomap_fill(map1, ELE/2);
	Contents c2 = bloomap_fill(map2, ELE/2);
	c1.insert(c2.begin(), c2.end());
	map1->or_from(map2);

	SECTION("--> FP rate is acceptable") {
		REQUIRE( bloomap_check_fp_rate(map1, c1, 0.01*SLACK) );
	}

	SECTION("--> contains all elements") {
		REQUIRE( bloomap_count_elements(map1, c1) == c1.size() );
	}

	/* TODO: check if it's the same map as if the elements were inserted all into one */
}

TEST_CASE( "****** Bloomap iterator.", "[operators]" ) {
	BloomapFamily *f = BloomapFamily::forElementsAndProb(ELE, 0.01);

	bloomap_messup(f, 10);

	Bloomap* map1 = f->newMap();

	REQUIRE (map1 != NULL);
	REQUIRE (map1->popcount() == 0);

	Contents c = bloomap_fill(map1, ELE);

	REQUIRE (c.size() == ELE);

	CAPTURE (c.size());

	unsigned found = 0;
	unsigned found_from_c = 0;
	unsigned found_not_in_map = 0;
	for (const auto& e : map1 ) {
		found++;
		if (c.count(e)) {
			found_from_c++;
		}
		if (!map1->contains(e)) 
			found_not_in_map++;
	}

	REQUIRE( found_from_c == c.size() );
	REQUIRE( found_not_in_map == 0 );
}
