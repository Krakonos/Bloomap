#include <map>
#include <list>
#include <vector>
#include <set>
#include <benchmark/benchmark.h>
#include <algorithm>
#include <cstdlib>
#include "bloomap.h"
#include "bloomapfamily.h"

using namespace std;

#define CLOBBER_MEMORY asm volatile("" : : : "memory")

static void BM_bloomap_insert( benchmark::State& state ) {
	BloomapFamily *f = BloomapFamily::forElementsAndProb(state.range_x(), 1.0/state.range_y());
	Bloomap *map = f->newMap();
	uint32_t n = 0;
	uint32_t range = state.range_x();
	while (state.KeepRunning()) {
		for (uint32_t i = 0; i < range; i++) {
			benchmark::DoNotOptimize(map->add(n++));
		}
		state.PauseTiming();
		map->clear();
		state.ResumeTiming();
	}
}

static void BM_bloomap_nofamily_insert( benchmark::State& state ) {
	BloomapFamily *f = BloomapFamily::forElementsAndProb(state.range_x(), 1.0/state.range_y());
	Bloomap *map = f->newMap();
	map->splitFamily();
	uint32_t n = 0;
	uint32_t range = state.range_x();
	while (state.KeepRunning()) {
		for (uint32_t i = 0; i < range; i++) {
			benchmark::DoNotOptimize(map->add(n++));
		}
		state.PauseTiming();
		map->clear();
		state.ResumeTiming();
	}
}

static void BM_stdmap_insert( benchmark::State& state ) {
	map<uint32_t,bool> map;	
	uint32_t n = 0;
	uint32_t range = state.range_x();
	while (state.KeepRunning()) {
		for (uint32_t i = 0; i < range; i++) {
			benchmark::DoNotOptimize(map[n++] = true);
		}
		state.PauseTiming();
		map.clear();
		state.ResumeTiming();
	}
}

static void BM_stdset_insert( benchmark::State& state ) {
	set<uint32_t> set;	
	uint32_t n = 0;
	uint32_t range = state.range_x();
	while (state.KeepRunning()) {
		for (uint32_t i = 0; i < range; i++) {
			benchmark::DoNotOptimize(set.insert(n++));
		}
		state.PauseTiming();
		set.clear();
		state.ResumeTiming();
	}
}

static void BM_stdvector_insert( benchmark::State& state ) {
	vector<uint32_t> vec;	
	uint32_t n = 0;
	uint32_t range = state.range_x();
	while (state.KeepRunning()) {
		for (uint32_t i = 0; i < range; i++) {
			benchmark::DoNotOptimize(vec.data());
			vec.push_back(n++);
			CLOBBER_MEMORY;
		}
		state.PauseTiming();
		vec.clear();
		state.ResumeTiming();
	}
}

static void BM_stdlist_insert( benchmark::State& state ) {
	list<uint32_t> lst;	
	uint32_t n = 0;
	uint32_t range = state.range_x();
	while (state.KeepRunning()) {
		for (uint32_t i = 0; i < range; i++) {
			benchmark::DoNotOptimize(lst);
			lst.push_back(n++);
			CLOBBER_MEMORY;
		}
		state.PauseTiming();
		lst.clear();
		state.ResumeTiming();
	}
}


static void H_fill_bloomap( Bloomap* map, uint32_t elements, uint32_t start ) {
	for (uint32_t i = 0; i < elements; i++) {
		map->add(rand());
	}
}

static void H_fill_vector( vector<uint32_t>& v, uint32_t elements, uint32_t start ) {
	v.resize(elements);
	for (uint32_t i = 0; i < elements; i++) {
		v.push_back(rand());
	}
}

static void BM_bloomap_intersect( benchmark::State& state ) {
	uint32_t range = state.range_x();
	BloomapFamily *f = BloomapFamily::forElementsAndProb(state.range_x(), 1.0/state.range_y());
	Bloomap *map1 = f->newMap();
	Bloomap *map2 = f->newMap();
	Bloomap *map_intersection = f->newMap();
	H_fill_bloomap(map1, range, 0);
	H_fill_bloomap(map2, range, range/2);

	while (state.KeepRunning()) {
		state.PauseTiming();
		map_intersection->clear();
		map_intersection->add(map1);
		state.ResumeTiming();
		benchmark::DoNotOptimize(map_intersection->intersect(map2));
	}

	delete map2;
	delete map1;
	delete f;
	delete map_intersection;
}

static void BM_stdvector_intersect( benchmark::State& state ) {
	uint32_t range = state.range_x();
    vector<uint32_t> v1,v2;
	H_fill_vector(v1, range, 0);
	H_fill_vector(v2, range, range/2);

    vector<uint32_t> v_intersection;
	v_intersection.resize(range, 0);

	while (state.KeepRunning()) {
		//for (unsigned i = 0; i < 100; i++) {
			benchmark::DoNotOptimize(v_intersection.data());
			benchmark::DoNotOptimize(v1.data());
			benchmark::DoNotOptimize(v2.data());
			set_intersection(v1.begin(), v1.end(),
					v2.begin(), v2.end(),
					back_inserter(v_intersection));
			CLOBBER_MEMORY;
		//}
		state.PauseTiming();
		v_intersection.clear();
		state.ResumeTiming();
	}
}

static void BloomapCustomArgs( benchmark::internal::Benchmark* b ) {
	b->ArgPair(1 << 8, 10);
	b->ArgPair(1 << 8, 50);
	b->ArgPair(1 << 8, 100);
	b->ArgPair(1 << 10, 10);
	b->ArgPair(1 << 10, 50);
	b->ArgPair(1 << 10, 100);
	b->ArgPair(1 << 12, 10);
	b->ArgPair(1 << 12, 50);
	b->ArgPair(1 << 12, 100);
	b->ArgPair(1 << 14, 10);
	b->ArgPair(1 << 14, 50);
	b->ArgPair(1 << 14, 100);
}

static void CustomArgs( benchmark::internal::Benchmark* b ) {
	b->Arg(1 << 8);
	b->Arg(1 << 10);
	b->Arg(1 << 12);
	b->Arg(1 << 14);
}

BENCHMARK(BM_bloomap_intersect)->Apply(BloomapCustomArgs);
BENCHMARK(BM_stdvector_intersect)->Apply(CustomArgs);
BENCHMARK(BM_bloomap_insert)->Apply(BloomapCustomArgs);
BENCHMARK(BM_bloomap_nofamily_insert)->Apply(BloomapCustomArgs);
BENCHMARK(BM_stdmap_insert)->Apply(CustomArgs);
BENCHMARK(BM_stdset_insert)->Apply(CustomArgs);
BENCHMARK(BM_stdvector_insert)->Apply(CustomArgs);
BENCHMARK(BM_stdlist_insert)->Apply(CustomArgs);

BENCHMARK_MAIN();
