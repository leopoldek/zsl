#pragma once
#include "core.h"
#include "memory.h"

namespace zsl{

template<typename T, Allocator allocator = ZSL_DEFAULT_ALLOCATOR>
struct ArrayList{
	using ValueType = T;
	using Self = ArrayList<T, allocator>;
	static constexpr size_t MIN_CAPACITY = 16;
	
	size_t capacity;
	size_t size;
	T* data;
	
	ALWAYS_INLINE T& operator[](size_t i){ZSL_ASSERT(i >= 0 && i < size); return data[i];}
	ALWAYS_INLINE T* begin(){return data;}
	ALWAYS_INLINE T* end(){return data + size;}
	ALWAYS_INLINE operator ArrayView<T>(){return {size, data};}
	ALWAYS_INLINE ArrayView<T> slice(size_t first, size_t last){ZSL_ASSERT(first < last && first >= 0 && last <= size); return {last - first, data + first};}
	ALWAYS_INLINE ArrayView<T> slice(size_t first = 0){ZSL_ASSERT(first >= 0 && first <= size); return {size - first, data + first};}
	ALWAYS_INLINE void clear(){size = 0;}
	ALWAYS_INLINE void deinit(){dealloc<allocator>(data);}
	
	static Self init(size_t initial = MIN_CAPACITY){
		initial = max(initial, MIN_CAPACITY);
		return {initial, 0, alloc<allocator, T>(initial)};
	}
	
	void reserve(size_t value){
		if(capacity < value){
			do capacity <<= 1; while(capacity < value);
			data = realloc<allocator>(capacity, data);
		}
	}
	
	void resize(size_t value){
		reserve(value);
		size = value;
	}
	
	void shrink(){
		capacity = max(size, MIN_CAPACITY);
		data = realloc<allocator>(capacity, data);
	}
	
	void insert(size_t place, const T& value){
		ZSL_ASSERT(place >= 0 && place <= size);
		reserve(size + 1);
		for(size_t i = size; i > place; i--) data[i] = data[i - 1];
		data[place] = value;
		size++;
	}
	
	void append(const T& value){
		reserve(size + 1);
		data[size] = value;
		size++;
	}
	
	void remove(size_t place){
		ZSL_ASSERT(place >= 0 && place < size);
		size--;
		for(size_t i = place; i < size; i++) data[i] = data[i + 1];
	}
	
	void removePlace(size_t place){
		ZSL_ASSERT(place >= 0 && place < size);
		data[place] = data[--size];
	}
	
	template<Allocator newAllocator = allocator>
	ArrayList<T, newAllocator> copy(){
		auto clone = ArrayList<T, newAllocator>::init(capacity);
		memcpy(clone.data, data, size * sizeof(T));
		return clone;
	}
};

//template<typename T> using TempList = ArrayList<T, talloc>;
//template<typename T> using TempView = ArrayView<T>;

}
