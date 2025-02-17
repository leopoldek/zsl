#pragma once
#include <setjmp.h>
#include <vector>

inline jmp_buf jmp;
/*
#define ZSL_ASSERT assertJmp
inline void assertJmp(bool check){
	if(check) return;
	//TODO: When we add stack trace to the library uncomment this.
	longjmp(jmp, 1);
}
*/
//void* testAlloc(void*, size_t, size_t);
//#define ZSL_DEFAULT_ALLOCATOR testAlloc
#include "zsl/core.h"
#include "zsl/hash_map.h"

using namespace zsl;

using TestFunction = bool(*)();
struct TestGenerator;
inline std::vector<TestGenerator*> tests;
inline std::vector<char*> infos;

struct TestGenerator{
	const char* name;
	TestFunction func;
	
	TestGenerator(const TestGenerator& gen){
		*this = gen;
		tests.push_back(this);
	}
	TestGenerator(const char* set){name = set;}
	TestGenerator& operator<<(TestFunction set){func = set; return *this;}
};

inline const char* testStatus = ANSI_YELLOW "TEST" ANSI_RESET;
inline const char* passedStatus = ANSI_GREEN "PASS" ANSI_RESET;
inline const char* failStatus = ANSI_RED "FAIL" ANSI_RESET;

inline void printInfo(TestGenerator* test, const char* status){
	printf(ANSI_CLEAR_LINE "\r[%s] Testing '%s'.", status, test->name);
	for(char* info: infos) printf(" (%s)", info);
	infos.clear();
	fflush(stdout);
}

inline bool performTest(TestGenerator& test){
	int error = 0;
	error = setjmp(jmp);
	if(error){
		//while(!allocations.empty()) free(*allocations.erase(allocations.begin()));
		return false;
	}
	if(!test.func()) return false;
	//if(!allocations.empty()) return false;
	return true;
}

#define TEST(name) TestGenerator TOKEN_PASTE(test, __LINE__) = TestGenerator{name} << []() -> bool

// CONCURRENCY
inline size_t threadCount = 100;
template<typename F>
struct Concurrent{
	F f;
	Semaphore sem;
	Concurrent(F mf): f(mf){
		sem.init();
		size_t count = threadCount;
		for(size_t i = 0; i < count; i++) threadCreate([](void* userPtr){
			Concurrent* c = (Concurrent*)userPtr;
			c->f();
			c->sem.post();
		}, this);
		sem.wait(count);
		sem.deinit();
	}
};
#define CONCURRENT Concurrent TOKEN_PASTE(concurrent, __LINE__) = [&]()

using BenchmarkFunction = void(*)(size_t*, size_t*);
struct BenchmarkGenerator;
inline std::vector<BenchmarkGenerator*> benchmarks;
/*

struct BenchmarkGenerator{
	const char* name;
	BenchmarkFunction func;
	
	BenchmarkGenerator(const BenchmarkGenerator& gen){
		*this = gen;
		benchmarks.push_back(this);
	}
	BenchmarkGenerator(const char* set){name = set;}
	BenchmarkGenerator& operator<<(TestFunction set){func = set; return *this;}
};

#define TEST(name) BenchmarkGenerator TOKEN_PASTE(benchmark, __LINE__) = BenchmarkGenerator{name} << []() -> void
*/
