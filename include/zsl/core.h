#pragma once

#include "stdint.h"
#include "stddef.h"
#include "limits.h"

#if defined(__WIN32__)
	#define ZSL_WINDOWS
	#if !defined(_WINDOWS_)
		#define WIN32_LEAN_AND_MEAN
		#define NOMINMAX
		#include "windows.h"
	#endif
	#ifndef ZSL_NO_CLZ
		#include <intrin.h>
	#endif
#elif defined(__unix__)
	#define ZSL_LINUX
	#include "pthread.h"
	#include "semaphore.h"
#else
	#error "Unknown Operating System"
#endif

#ifndef ZSL_ASSERT
#include "assert.h"
#define ZSL_ASSERT assert
#endif

namespace zsl{

// --------------------------------------------------------------------------------------------------------
// ------------------------------------------------ Traits ------------------------------------------------
// --------------------------------------------------------------------------------------------------------

template<bool, typename T = void> struct enableIf{typedef T type;};
template<typename T> struct enableIf<false, T>{};

template<typename L, typename R> struct typeEqual{static constexpr bool same = false;};
template<typename T> struct typeEqual<T, T>{static constexpr bool same = true;};
template<typename L, typename R> inline constexpr bool isTypeEqual = typeEqual<L, R>::same;

// --------------------------------------------------------------------------------------------------------
// ------------------------------------------------ Macros ------------------------------------------------
// --------------------------------------------------------------------------------------------------------
// Since we don't namespace macros, you can rename them like
// #define MY_NAME ZSL_NAME
// #undef ZSL_NAME
// if they collide in user files.

#if defined(__GNUC__) && (__GNUC__ >= 4)
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#elif defined(__llvm__)
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define ALWAYS_INLINE __forceinline
#else
#define ALWAYS_INLINE inline
#endif

#define IGNORE_GCC_WARNING(warning) _Pragma("GCC diagnostic ignored \"-W##warning\"")
#define IGNORE_MSVC_WARNING(warning) _Pragma("warning( disable : warning )")

#if (defined(__GNUC__) && (__GNUC__ >= 4)) || defined(__llvm__)
#define IGNORE_WARNING(gcc, msvc) IGNORE_GCC_WARNING(gcc)
#elif defined(_MSC_VER)
#define IGNORE_WARNING(gcc, msvc) IGNORE_MSVC_WARNING(msvc)
#endif

#define RAW_ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define BIT_MODULO(value, pow) ((value) & ((pow) - 1))
//#define MEMBER_BY_OFFSET(type, base, offset) (*(type*)((char*)&(base) + (offset)))

//TODO Place in io.h?
#define ANSI(code) "\033["#code
#define ANSI_COLOR(code) ANSI(code ## m)
#define ANSI_COLOR_RGB(r, g, b) ANSI_COLOR(38;2;r;g;b)
#define ANSI_RESET      ANSI_COLOR(0)
#define ANSI_BLACK      ANSI_COLOR(1;30)
#define ANSI_RED        ANSI_COLOR(1;31)
#define ANSI_GREEN      ANSI_COLOR(1;32)
#define ANSI_YELLOW     ANSI_COLOR(1;33)
#define ANSI_BLUE       ANSI_COLOR(1;34)
#define ANSI_MAGENTA    ANSI_COLOR(1;35)
#define ANSI_CYAN       ANSI_COLOR(1;36)
#define ANSI_WHITE      ANSI_COLOR(1;37)
#define ANSI_BG_BLACK   ANSI_COLOR(1;40)
#define ANSI_BG_RED     ANSI_COLOR(1;41)
#define ANSI_BG_GREEN   ANSI_COLOR(1;42)
#define ANSI_BG_YELLOW  ANSI_COLOR(1;43)
#define ANSI_BG_BLUE    ANSI_COLOR(1;44)
#define ANSI_BG_MAGENTA ANSI_COLOR(1;45)
#define ANSI_BG_CYAN    ANSI_COLOR(1;46)
#define ANSI_BG_WHITE   ANSI_COLOR(1;47)
#define ANSI_CLEAR_LINE ANSI(1K)

#define TOKEN_PASTE2(x, y) x ## y
#define TOKEN_PASTE(x, y) TOKEN_PASTE2(x, y)

template<typename F>
struct Defer{
	F f;
	ALWAYS_INLINE Defer(F f):f(f){}
	ALWAYS_INLINE ~Defer(){f();}
};
template<typename F> ALWAYS_INLINE Defer<F> makeDefer(F f){return Defer<F>(f);}
#define DEFER(code) auto TOKEN_PASTE(_defer_, __LINE__) = makeDefer([&](){code;})

#define ZSL_NO_MOVE(name) name(name&&) = delete; name& operator=(name&&) = delete
#define ZSL_NO_COPY(name) ZSL_NO_MOVE(name); name(const name&) = delete; name& operator=(const name&) = delete
#define ZSL_SCOPED_OBJECT(name) ZSL_NO_COPY(name); \
		void *operator new(size_t) = delete; void operator delete(void*) = delete; \
		void *operator new[](size_t) = delete; void operator delete[](void*) = delete


// --------------------------------------------------------------------------------------------------------
// ------------------------------------------------- Utils ------------------------------------------------
// --------------------------------------------------------------------------------------------------------

template<typename T>
struct ArrayView{
	using ValueType = T;
	
	size_t size;
	T* data;
	
	ALWAYS_INLINE T& operator[](size_t i){ZSL_ASSERT(i >= 0 && i < size); return data[i];}
	ALWAYS_INLINE T* begin(){return data;}
	ALWAYS_INLINE T* end(){return data + size;}
};

template<typename T>
struct GlobalVar{
	T var;
	GlobalVar(){var.init();}
	~GlobalVar(){var.deinit();}
	operator T&(){return var;}
	T& operator*(){return var;}
};

template<typename T>
ALWAYS_INLINE T min(T x, T y){
	return x < y ? x : y;
}

template<typename T>
ALWAYS_INLINE T max(T x, T y){
	return x > y ? x : y;
}

template<typename T>
ALWAYS_INLINE T clamp(T value, T min, T max){
	if(value < min) return min;
	if(value > max) return max;
	return value;
}

template<typename T>
ALWAYS_INLINE bool isPow2(T value){
	return value && !(value & (value - 1));
}

template<typename T>
ALWAYS_INLINE T align(T value, size_t align = 8){
	ZSL_ASSERT(isPow2(align));
	align--;
	return (T)(((size_t)value + align) & ~align);
}

template<typename T>
ALWAYS_INLINE T alignFloor(T value, size_t align = 8){
	ZSL_ASSERT(isPow2(align));
	align--;
	return (T)((size_t)value & ~align);
}

ALWAYS_INLINE uint32_t nextPow2(uint32_t v){
	ZSL_ASSERT(v != 0);
	ZSL_ASSERT(v <= (UINT32_C(1) << 31));
#if (defined(__GNUC__) || defined(__clang__)) && !defined(ZSL_NO_CLZ)
	return v == 1 ? 1 : UINT32_C(1) << (32 - __builtin_clz(v - 1));
#elif defined(_MSC_VER) && !defined(ZSL_NO_CLZ)
	unsigned long int leadingZeroes;
	if(_BitScanReverse(&leadingZeroes, v - 1)) return UINT32_C(1) << (leadingZeroes + 1);
	else return 1;
#else
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	return v + 1;
#endif
}

ALWAYS_INLINE uint64_t nextPow2(uint64_t v){
	ZSL_ASSERT(v != 0);
	ZSL_ASSERT(v <= (UINT64_C(1) << 63));
#if (defined(__GNUC__) || defined(__clang__)) && !defined(ZSL_NO_CLZ)
	return v == 1 ? 1 : UINT64_C(1) << (64 - __builtin_clzl(v - 1));
#elif defined(_MSC_VER) && !defined(ZSL_NO_CLZ)
	unsigned long int leadingZeroes;
	if(_BitScanReverse64(&leadingZeroes, v - 1)) return UINT64_C(1) << (leadingZeroes + 1);
	else return 1;
#else
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v |= v >> 32;
	return v + 1;
#endif
}

ALWAYS_INLINE uint32_t log2Ceil(uint32_t v){
	ZSL_ASSERT(v != 0);
	ZSL_ASSERT(v <= (UINT32_C(1) << 31));
#if (defined(__GNUC__) || defined(__clang__)) && !defined(ZSL_NO_CLZ)
	return v == 1 ? 0 : 32 - __builtin_clz(v - 1);
#elif defined(_MSC_VER) && !defined(ZSL_NO_CLZ)
	unsigned long int leadingZeroes;
	if(_BitScanReverse(&leadingZeroes, v - 1)) return leadingZeroes + 1;
	else return 1;
#else
	uint32_t i = 0;
	while(true){
		v >>= 1;
		if(!v) return i;
		i++;
	}
#endif
}

ALWAYS_INLINE uint64_t log2Ceil(uint64_t v){
	ZSL_ASSERT(v != 0);
	ZSL_ASSERT(v <= (UINT64_C(1) << 63));
#if (defined(__GNUC__) || defined(__clang__)) && !defined(ZSL_NO_CLZ)
	return v == 1 ? 0 : 64 - __builtin_clzl(v - 1);
#elif defined(_MSC_VER) && !defined(ZSL_NO_CLZ)
	unsigned long int leadingZeroes;
	if(_BitScanReverse64(&leadingZeroes, v - 1)) return leadingZeroes + 1;
	else return 0;
#else
	uint64_t i = 0;
	while(true){
		v >>= 1;
		if(!v) return i;
		i++;
	}
#endif
}

inline bool isStringEqual(const char* a, const char* b){
	size_t i = 0;
	while(true){
		if(a[i] != b[i]) return false;
		if(a[i] == '\0') return true;
		i++;
	}
}

template<typename Container>
bool hasString(Container& buffer, const char* str){
	for(const char* value: buffer){
		if(isStringEqual(value, str)) return true;
	}
	return false;
}

template<typename Container, typename T>
bool contains(Container& buffer, const T& value){
	for(const T& element: buffer) if(value == element) return true;
	return false;
}

// --------------------------------------------------------------------------------------------------------
// -------------------------------------------------- OS --------------------------------------------------
// --------------------------------------------------------------------------------------------------------

void* allocateVirtualMemory(size_t);
void* reserveVirtualMemory(size_t);
void commitVirtualMemory(void*, size_t);
void freeVirtualMemory(void*, size_t);

using ThreadFunction = void(*)(void*);
void threadCreate(ThreadFunction, void*);

#if defined(ZSL_WINDOWS)
using MutexType = CRITICAL_SECTION;
//using SemaphoreType = HANDLE;
#elif defined(ZSL_LINUX)
using MutexType = pthread_mutex_t;
//using SemaphoreType = sem_t;
using ConditionType = pthread_cond_t;
#endif

struct Mutex{
	ZSL_NO_COPY(Mutex);
	Mutex() = default;
	MutexType mutex;
	void init();
	void lock();
	bool tryLock();
	void unlock();
	void deinit();
};

struct LockScope{
	ZSL_SCOPED_OBJECT(LockScope);
	Mutex& mutex;
	ALWAYS_INLINE LockScope(Mutex& m): mutex(m){mutex.lock();}
	ALWAYS_INLINE ~LockScope(){mutex.unlock();}
};

struct Condition{
	ZSL_NO_COPY(Condition);
	Condition() = default;
	ConditionType cond;
	void init();
	void wait(Mutex*);
	void broadcast();
	void signal();
	void deinit();
};

struct Semaphore{
	Mutex mutex;
	Condition cond;
	size_t count;
	
	void init(){
		mutex.init();
		cond.init();
		count = 0;
	}
	
	void wait(size_t amount = 1){
		LockScope lock(mutex);
		while(count < amount) cond.wait(&mutex);
		count -= amount;
	}
	
	void post(size_t amount = 1){
		LockScope lock(mutex);
		count += amount;
		if(amount == 1) cond.signal();
		else cond.broadcast();
	}
	
	void deinit(){
		cond.deinit();
		mutex.deinit();
	}
};

// --------------------------------------------------------------------------------------------------------
// ------------------------------------------------ Memory ------------------------------------------------
// --------------------------------------------------------------------------------------------------------

template<auto value> inline constexpr auto KB =    value  * (decltype(value))1000;
template<auto value> inline constexpr auto MB = KB<value> * (decltype(value))1000;
template<auto value> inline constexpr auto GB = MB<value> * (decltype(value))1000;
template<auto value> inline constexpr auto TB = GB<value> * (decltype(value))1000;

using Allocator = void*(*)(void*, size_t, size_t);

struct Arena{
	static inline constexpr size_t MAX_ARENA_SIZE = sizeof(size_t) == 8 ? TB<size_t(1)> : MB<size_t(100)>;
	static inline constexpr size_t ARENA_BLOCK_SIZE = sizeof(size_t) == 8 ? MB<size_t(100)> : KB<size_t(100)>;
	
	char* capacity;
	char* mark;
	char* data;
	Mutex mutex;
	
	void init();
	void deinit();
	void* alloc(void*, size_t, size_t);
	void reset();
};

template<Arena& arena>
ALWAYS_INLINE void* aalloc(void* data, size_t size, size_t alignment){
	return arena.alloc(data, size, alignment);
}

void resetTalloc();
void* talloc(void*, size_t, size_t);

#ifndef NDEBUG
struct NallocInfo{
	size_t blockSize;
	size_t totalBlocks;
	size_t usedSize;
};
ArrayView<NallocInfo> nallocGetInfo();
#endif
void* nalloc(void*, size_t, size_t);
#ifndef ZSL_DEFAULT_ALLOCATOR
#define ZSL_DEFAULT_ALLOCATOR nalloc
#endif

template<Allocator allocator, typename T>
ALWAYS_INLINE T* alloc(size_t size = 1){
	return (T*)allocator(nullptr, size * sizeof(T), alignof(T));
}

template<Allocator allocator, typename T>
ALWAYS_INLINE T* realloc(size_t size, T* ptr){
	// Allocator is responsible for saving alignment, so we set it to zero.
	return (T*)allocator(ptr, size * sizeof(T), 0);
}

template<Allocator allocator, typename T>
ALWAYS_INLINE void dealloc(T* ptr){
	allocator(ptr, 0, 0);
}

}
