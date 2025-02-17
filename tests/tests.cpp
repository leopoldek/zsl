#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>
#include <algorithm>
#include "tests.h"

std::vector<void*> allocations;
void* testAlloc(void* ptr, size_t size, size_t alignment){
	if(ptr == nullptr){
		void* a = malloc(size);
		allocations.push_back(a);
		return a;
	}else{
		if(size == 0){
			auto it = std::find(allocations.begin(), allocations.end(), ptr);
			ZSL_ASSERT(it != allocations.end());
			free(ptr);
			allocations.erase(it);
			return nullptr;
		}else{
			auto it = std::find(allocations.begin(), allocations.end(), ptr);
			ZSL_ASSERT(it != allocations.end());
			void* a = realloc(ptr, size);
			*it = a;
			return a;
		}
	}
}

template<typename Map>
bool mapSanityCheck(Map& map){
	for(size_t i = 0; i < map.capacity; i++){
		if(map.data[i].type == Map::RecordType::PLACED) return false;
	}
	if(!isPow2(map.capacity)) return false;
	if(map.getLoadFactor() > Map::MAX_LOAD_FACTOR) return false;
	map.deinit();
	return true;
}

TEST("Hash Map Initialization"){
	auto map = HashMap<int, int>::init();
	return mapSanityCheck(map);
};

TEST("Hash Map Insertion"){
	const size_t count = 2000000;
	auto map = HashMap<int, int>::init();
	for(int i = 0; i < count; i++) map[i] = i;
	if(map.size != count) return false;
	for(int i = 0; i < count; i++) if(map.get(i) != i) return false;
	return mapSanityCheck(map);
};

TEST("Hash Map Deletion"){
	const size_t count = 2000000;
	auto map = HashMap<int, int>::init();
	for(int i = 0; i < count; i++) map[i] = i;
	for(int i = 0; i < count; i++) map.remove(i);
	bool success = true;
	if(map.size != 0) return false;
	for(size_t i = 0; i < map.capacity; i++){
		if(map.data[i].type == decltype(map)::RecordType::OCCUPIED) return false;
	}
	return mapSanityCheck(map);
};

TEST("Synchronization"){
	Mutex mutex; mutex.init();
	auto map = HashMap<int, int>::init();
	int i = 0;
	CONCURRENT{
		LockScope lock(mutex);
		map[i++] = 16;
	};
	mutex.deinit();
	map.deinit();
	if(i != threadCount) return false;
	CONCURRENT{
		auto map = HashMap<int, int>::init();
		for(int i = 0; i < 1000; i++) map[i] = 16;
		map.deinit();
	};
	CONCURRENT{
		for(int i = 0; i < 10000; i++){
			auto ptr = alloc<nalloc, int>();
			dealloc<nalloc>(ptr);
		}
	};
	return true;
};

timespec diff(timespec start, timespec end){
	timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

int main(){
	int failedCount = 0;
	for(auto test: tests){
		printInfo(test, testStatus);
		bool passed = performTest(*test);
		printInfo(test, passed ? passedStatus : failStatus);
		printf("\n");
	}
	
	const size_t count = 10000;
	timespec time1, time2;
	
	//srand(time(NULL));
	srand(0);
	
	{
		auto benchmark = HashMap<int, int>::init();
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time1);
		for(int i = 0; i < count; i++) benchmark[rand()] = i;
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time2);
		for(int i = 0; i < count; i++) if(benchmark.has(i)) benchmark.remove(i);
		benchmark.deinit();
	}
	
	printf("zsl: %ld\n", diff(time1, time2).tv_sec * 1'000'000'000 + diff(time1, time2).tv_nsec);
	srand(0);
	
	{
		std::unordered_map<int, int> benchmark;
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time1);
		for(int i = 0; i < count; i++) benchmark[rand()] = i;
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time2);
		for(int i = 0; i < count; i++) benchmark.erase(i);
	}
	printf("std: %ld\n", diff(time1, time2).tv_sec * 1'000'000'000 + diff(time1, time2).tv_nsec);
}
