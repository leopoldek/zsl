#pragma once
#include "core.h"

namespace zsl{

#if defined(__clang__) || defined(__GNUC__)
#define ZSL_LOCK_FREE(type) static_assert(__atomic_always_lock_free(sizeof(type), 0) == true, "This type is not lock-free.")

using atomic_uint8   = uint8_t;
using atomic_uint16  = uint16_t;
using atomic_uint32  = uint32_t;
using atomic_uint64  = uint64_t;
using atomic_uint128 = unsigned __int128;

using atomic_int8   = int8_t;
using atomic_int16  = int16_t;
using atomic_int32  = int32_t;
using atomic_int64  = int64_t;
using atomic_int128 = __int128;

using atomic_size = size_t;

enum AtomicOrder: int{
	ORDER_RELAXED = __ATOMIC_RELAXED,
	ORDER_CONSUME = __ATOMIC_CONSUME,
	ORDER_ACQUIRE = __ATOMIC_ACQUIRE,
	ORDER_RELEASE = __ATOMIC_RELEASE,
	ORDER_ACQ_REL = __ATOMIC_ACQ_REL,
	ORDER_SEQ_CST = __ATOMIC_SEQ_CST,
};

template<typename T>
ALWAYS_INLINE T atomicLoad(T* ptr, AtomicOrder order = ORDER_SEQ_CST){
	ZSL_LOCK_FREE(T);
	return __atomic_load_n(ptr, order);
}

template<typename T>
ALWAYS_INLINE void atomicStore(T* ptr, T val, AtomicOrder order = ORDER_SEQ_CST){
	ZSL_LOCK_FREE(T);
	__atomic_store_n(ptr, val, order);
}

template<typename T>
ALWAYS_INLINE T atomicExchange(T* ptr, T val, AtomicOrder order = ORDER_SEQ_CST){
	ZSL_LOCK_FREE(T);
	return __atomic_exchange_n(ptr, val, order);
}

template<typename T>
ALWAYS_INLINE bool atomicCompareExchangeWeak(T* ptr, T* expected, T desired, AtomicOrder success = ORDER_SEQ_CST, AtomicOrder failure = ORDER_SEQ_CST){
	if constexpr(sizeof(T) == 16){
		atomic_uint128* const expectedInt = (atomic_uint128*)expected;
		const atomic_uint128 check = *expectedInt;
		*expectedInt = __sync_val_compare_and_swap((atomic_uint128*)ptr, *expectedInt, *(atomic_uint128*)&desired);
		return check == *expectedInt;
	}else{
		ZSL_LOCK_FREE(T);
		return __atomic_compare_exchange_n(ptr, expected, desired, true, success, failure);
	}
}

template<typename T>
ALWAYS_INLINE bool atomicCompareExchangeStrong(T* ptr, T* expected, T desired, AtomicOrder success = ORDER_SEQ_CST, AtomicOrder failure = ORDER_SEQ_CST){
	//TODO: Implement 128-bit
	ZSL_LOCK_FREE(T);
	return __atomic_compare_exchange_n(ptr, expected, desired, false, success, failure);
}

template<typename T> ALWAYS_INLINE T atomicAdd (T *ptr, T val, AtomicOrder order = ORDER_SEQ_CST){ZSL_LOCK_FREE(T); return __atomic_fetch_add (ptr, val, order);}
template<typename T> ALWAYS_INLINE T atomicSub (T *ptr, T val, AtomicOrder order = ORDER_SEQ_CST){ZSL_LOCK_FREE(T); return __atomic_fetch_sub (ptr, val, order);}
template<typename T> ALWAYS_INLINE T atomicAnd (T *ptr, T val, AtomicOrder order = ORDER_SEQ_CST){ZSL_LOCK_FREE(T); return __atomic_fetch_and (ptr, val, order);}
template<typename T> ALWAYS_INLINE T atomicXor (T *ptr, T val, AtomicOrder order = ORDER_SEQ_CST){ZSL_LOCK_FREE(T); return __atomic_fetch_xor (ptr, val, order);}
template<typename T> ALWAYS_INLINE T atomicOr  (T *ptr, T val, AtomicOrder order = ORDER_SEQ_CST){ZSL_LOCK_FREE(T); return __atomic_fetch_or  (ptr, val, order);}
template<typename T> ALWAYS_INLINE T atomicNand(T *ptr, T val, AtomicOrder order = ORDER_SEQ_CST){ZSL_LOCK_FREE(T); return __atomic_fetch_nand(ptr, val, order);}

#elif defined(__WIN32__)
#endif

template<typename T = void>
struct alignas(alignof(T*) * 2) TaggedPointer{
	size_t tag;
	T* ptr;
	
	TaggedPointer load(){
		// Even though this is a non-atomic load,
		// We load the tag first so the
		// CAS will catch any shearing.
		TaggedPointer old;
		old.tag = atomicLoad(&tag, ORDER_ACQUIRE);
		old.ptr = ptr;
		return old;
	}
	
	bool store(TaggedPointer* old, T* set){
		return atomicCompareExchangeWeak(this, old, {old->tag + 1, set});
	}
};


}
