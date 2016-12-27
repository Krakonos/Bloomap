#include <catch/catch.hpp>
#include <vector>
#include <map>
#include <iostream>
#include <cassert>

#include "bloomap.h"
#include "bloomapfamily.h"

#define ELE 100
#define ELE_BENCH 1000000
#define ITER 10
#define SLACK 1.2

using namespace std;
typedef map<uint64_t,bool> Contents;

Contents bloomap_fill(Bloomap* map, unsigned count, unsigned seed = 0) {
	if (seed) srand(seed);
	Contents insert;
	unsigned e = 1; /* First attempt should try to insert 1, as it's special. */
	for (unsigned i = 0; i < count; i++) {
		while (insert.count(e)) e = rand(); /* Don't insert value twice */
		map->add( e );
		assert(map->contains(e));
		insert[e] = true;
	}
	return insert;
}

unsigned bloomap_count_elements(Bloomap* map, Contents& list) {
	unsigned insert_found = 0;
	for (Contents::iterator it = list.begin(); it != list.end(); ++it) {
		pair<uint64_t,bool> pair = *it;
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
	if ( fp_rate < exp_rate ) return true;
	else {
		std::cerr << "FP rate is NOT acceptable! Required: " << exp_rate << "Result: " << fp_rate << std::endl;
		return false;
	}
}

/* Insert some random bogus elements into the map, to fill the sidelist. */
void bloomap_messup(BloomapFamily *f, unsigned mess_factor) {
	Bloomap* dummy_map = f->newMap();
	bloomap_fill(dummy_map, mess_factor*ELE);
	delete dummy_map;
}

unsigned gen_element(Bloomap* map) {
	unsigned e = rand();
	while (map->contains(e)) e = rand();
	return e;
}


TEST_CASE( "****** Elementary Bloomap operations." ) {
	BloomapFamily *f = BloomapFamily::forElementsAndProb(ELE, 0.01);
	Bloomap* map1 = f->newMap();

	REQUIRE (map1 != NULL);

	REQUIRE (map1->popcount() == 0);

	SECTION("--> empty map does not contain elements") {
		REQUIRE( !map1->contains(666) );
	}

	SECTION("--> insertion of special elements work") {
		map1->add(1);
		REQUIRE( map1->contains(1) );
	}

	SECTION("--> fill with random elements and check them") {
		for (unsigned i = 0; i < ITER; i++) {
			map1->clear();
			Contents c = bloomap_fill(map1, ELE);
			REQUIRE( bloomap_count_elements(map1, c) == ELE );
		}
	}

	SECTION("--> add() returns correct changed status") {
		unsigned e = gen_element(map1);
		REQUIRE(  map1->add(e) ); /* First addition should return true. */
		REQUIRE( !map1->add(e) ); /* Other should return false. */
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
	delete map1;
	delete f;
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

	SECTION("--> add() notices if map didn't change") {
		REQUIRE( map1->add(map2) == false );
	}

	SECTION("--> add() notices if map did change") {
		unsigned e = rand();
		while (map1->contains(e)) e = rand();
		map2->add(e);
		REQUIRE( map1->add(map2) == true );
	}

	/* TODO: check if it's the same map as if the elements were inserted all into one */
	delete map1;
	delete map2;
	delete f;
}

TEST_CASE( "***** Bloomap intersection.", "[intersection]" ) {
	BloomapFamily *f = BloomapFamily::forElementsAndProb(ELE, 0.01);
	Bloomap* map1 = f->newMap();
	Bloomap* map2 = f->newMap();
	Bloomap* map3 = f->newMap();
	Bloomap* mapi = f->newMap();

	REQUIRE (map1 != NULL);
	REQUIRE (map2 != NULL);
	REQUIRE (mapi != NULL);

	REQUIRE (map1->popcount() == 0);
	REQUIRE (map2->popcount() == 0);
	REQUIRE (mapi->popcount() == 0);

	Contents c1 = bloomap_fill(map1, ELE/2);
	Contents c2 = bloomap_fill(map2, ELE/2);

	/* Add known intersecting elements */
	unsigned known_element = 666;
	map1->add( known_element );
	map2->add( known_element );

	/* Find an element NOT in map2, and insert it into map1. */
	unsigned non_intersecting = gen_element(map2);
	map1->add(non_intersecting);

	/* Copy map1 into mapi, and intersect */
	mapi->add(map1);
	mapi->intersect(map2);

	SECTION("--> Elements contained in intersection are indeed in both maps") {
		unsigned missing_elements = 0;
		/* Iterate over elements from map1 */
		for (Contents::iterator it = c1.begin(); it != c1.end(); ++it) {
			/* Check if the element is in map2. If true, expect it in mapi also. */
			if (map2->contains((*it).first)) {
				if (!mapi->contains((*it).first)) {
					missing_elements++;
				}
			}
		}
		REQUIRE( missing_elements == 0 );
	}

	SECTION("--> Known element is in the intersection.") {
		REQUIRE( mapi->contains(known_element) );
	}

	SECTION("--> Known element not in map2 is NOT in intersection.") {
		REQUIRE( !map2->contains(non_intersecting) );
		REQUIRE( !mapi->contains(non_intersecting) );
	}

	SECTION("--> Intersection with empty map yields empty map." ) {
		mapi->intersect(map3);
		REQUIRE( mapi->popcount() == 0 );

	}

	delete map1;
	delete map2;
	delete map3;
	delete mapi;
	delete f;
}

TEST_CASE( "****** BloomapFamily iterator.", "[operators]" ) {
	BloomapFamily *f = BloomapFamily::forElementsAndProb(ELE, 0.01);

	Bloomap* map1 = f->newMap();
	map1->add(0U);
	map1->add(1);
	map1->add(255);
	map1->add(666);
	map1->add(3333);

	for (BloomapFamilyIterator it = f->begin(0); !it.atEnd(); ++it) {
	//	std::cerr << "Candidate: " << *it << std::endl;
	}

	delete map1;
	delete f;
}

TEST_CASE( "****** Bloomap iterator.", "[operators]" ) {
	BloomapFamily *f = BloomapFamily::forElementsAndProb(ELE, 0.01);

	//bloomap_messup(f, 10);

	Bloomap* map1 = f->newMap();

	REQUIRE (map1 != NULL);
	REQUIRE (map1->popcount() == 0);

	Contents c = bloomap_fill(map1, ELE);

	REQUIRE (c.size() == ELE);

	CAPTURE (c.size());

	unsigned found = 0;
	unsigned found_from_c = 0;
	unsigned found_not_in_map = 0;
	unsigned e;
	BLOOMAP_FOR_EACH(e, map1) {
		found++;
		if (c.count(e)) {
			found_from_c++;
			c[e] = false;
		}
		if (!map1->contains(e)) {
			found_not_in_map++;
		}
	}

	for (Contents::iterator it = c.begin(); it != c.end(); ++it) {
		if ((*it).second)
			std::cerr << "Missing: " << (*it).first << std::endl;
	}

	REQUIRE( found_from_c == c.size() );
	REQUIRE( found_not_in_map == 0 );
	REQUIRE( map1->contains(1) ); /* 1 was inserted in bloomap_fill */
	delete map1;
	delete f;
}

TEST_CASE( "****** Bloomap operator==().", "[operators]" ) {
	BloomapFamily *f = BloomapFamily::forElementsAndProb(ELE, 0.01);

	Bloomap* map1 = f->newMap();
	Bloomap* map2 = f->newMap();

	REQUIRE (map1 != NULL);
	REQUIRE (map1->popcount() == 0);
	REQUIRE (map2 != NULL);
	REQUIRE (map2->popcount() == 0);

	SECTION("--> Empty maps are equal.") {
		REQUIRE(*map1 == map2);
	}

	SECTION("--> Copies are equal.") {
		bloomap_fill(map1, ELE);
		map2->or_from(map1);
		REQUIRE(*map1 == map2);
	}

	SECTION("--> Special element break equality.") {
		map2->add(1L);
		REQUIRE(*map1 != map2);
	}

	SECTION("--> Distinc copies are not equal.") {
		bloomap_fill(map1, ELE);
		map2->or_from(map1);
		unsigned e = rand();
		while (map2->contains(e)) e = rand();
		map2->add(e);
		REQUIRE(*map1 != map2);
	}
	delete map1;
	delete map2;
	delete f;
}

TEST_CASE( "****** Bloomap speed.", "[benchmark]" ) {
	BloomapFamily *f = BloomapFamily::forElementsAndProb(ELE_BENCH, 0.01);

	vector<uint64_t> ins_data;
	for (unsigned i = 0; i < ELE_BENCH; i++)
		ins_data.push_back(rand());
	Bloomap* map1 = f->newMap();
	map<uint64_t,bool> map2;
	vector<uint64_t> map3;

	SECTION("[bench] Bloomap->add()") {
		for (unsigned i = 0; i < ins_data.size(); i++) {
			map1->add( ins_data[i] );
		}
	}
	SECTION("[bench] map->operator[]") {
		for (unsigned i = 0; i < ins_data.size(); i++) {
			map2[ins_data[i]] = true;
		}
	}
	SECTION("[bench] vector->push_back") {
		for (unsigned i = 0; i < ins_data.size(); i++) {
			map3.push_back(ins_data[i]);
		}
	}
	delete map1;
	delete f;
}
