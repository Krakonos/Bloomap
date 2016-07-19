#include <map>
#include <list>
#include <vector>
#include <set>
#include <benchmark/benchmark.h>
#include <cstdlib>
#include "bloomap.h"
#include "bloomapfamily.h"

using namespace std;

#define CLOBBER_MEMORY asm volatile("" : : : "memory")

static void BM_bloomap_insert( benchmark::State& state ) {
	BloomapFamily *f = BloomapFamily::forElementsAndProb(state.range_x(), 1.0/state.range_y());
	Bloomap *map = f->newMap();
	uint32_t n = 0;
	while (state.KeepRunning()) {
		benchmark::DoNotOptimize(map->add(n++));
	}
}

static void BM_stdmap_insert( benchmark::State& state ) {
	map<uint32_t,bool> map;	
	uint32_t n = 0;
	while (state.KeepRunning()) {
		benchmark::DoNotOptimize(map[n++] = true);
	}
}

static void BM_stdset_insert( benchmark::State& state ) {
	set<uint32_t> set;	
	uint32_t n = 0;
	while (state.KeepRunning()) {
		benchmark::DoNotOptimize(set.insert(n++));
	}
}

static void BM_stdvector_insert( benchmark::State& state ) {
	vector<uint32_t> vec;	
	uint32_t n = 0;
	while (state.KeepRunning()) {
		benchmark::DoNotOptimize(vec.data());
		vec.push_back(n++);
		CLOBBER_MEMORY;
	}
}

static void BM_stdlist_insert( benchmark::State& state ) {
	list<uint32_t> lst;	
	uint32_t n = 0;
	while (state.KeepRunning()) {
		benchmark::DoNotOptimize(lst);
		lst.push_back(n++);
		CLOBBER_MEMORY;
	}
}

static void CustomArgs( benchmark::internal::Benchmark* b ) {
	b->ArgPair(1 << 10, 10);
	b->ArgPair(1 << 10, 100);
	b->ArgPair(1 << 13, 10);
	b->ArgPair(1 << 13, 100);
}

BENCHMARK(BM_bloomap_insert)->Apply(CustomArgs);
BENCHMARK(BM_stdmap_insert)->Apply(CustomArgs);
BENCHMARK(BM_stdset_insert)->Apply(CustomArgs);
BENCHMARK(BM_stdvector_insert)->Apply(CustomArgs);
BENCHMARK(BM_stdlist_insert)->Apply(CustomArgs);

BENCHMARK_MAIN();
