#pragma once
#include "core.h"

namespace zsl{

template<typename T> using HashFunction = size_t(*)(const T&);
template<typename T> using HashMethod = size_t(T::*)() const;
template<typename T> using CompareFunction = bool(*)(const T&, const T&);
template<typename T> using CompareMethod = bool(T::*)(const T&) const;

template<bool B> struct HashWrapper{};
template<typename T, typename B = HashWrapper<true>> struct isCustomHashHelper{static constexpr bool is = false;};
template<typename T> struct isCustomHashHelper<T, HashWrapper<isTypeEqual<decltype(&T::hash), HashMethod<T>>>>{static constexpr bool is = true;};
template<typename T> inline constexpr bool isCustomHash = isCustomHashHelper<T>::is;

template<typename T, typename B = HashWrapper<true>> struct isCustomCompareHelper{static constexpr bool is = false;};
template<typename T> struct isCustomCompareHelper<T, HashWrapper<isTypeEqual<decltype(&T::compare), CompareMethod<T>>>>{static constexpr bool is = true;};
template<typename T> inline constexpr bool isCustomCompare = isCustomCompareHelper<T>::is;

template<typename T>
ALWAYS_INLINE size_t defaultHash(const T& value){
	if constexpr(isCustomHash<T>) return value.hash();
	else return (size_t)value;
}

template<typename T>
ALWAYS_INLINE bool defaultCompare(const T& a, const T& b){
	if constexpr(isCustomCompare<T>) return a.compare(b);
	else return a == b;
}

template<typename K, typename V, Allocator allocator = ZSL_DEFAULT_ALLOCATOR, HashFunction<K> hasher = defaultHash<K>, CompareFunction<K> comparer = defaultCompare<K>>
struct HashMap{
	using KeyType = K;
	using ValueType = V;
	using Self = HashMap<K, V, allocator, hasher, comparer>;
	static constexpr size_t MIN_CAPACITY = 16;// MUST BE POWER OF TWO
	static constexpr double MAX_LOAD_FACTOR = 0.7;
	
	enum class RecordType: uint8_t{
		UNUSED,
		DELETED,
		OCCUPIED,
		
		// Only used in rehash function.
		// No records should have this after rehash finishes.
		PLACED,
	};
	
	struct Record{
		K key;
		V value;
		RecordType type;
	};
	
	size_t capacity;// MUST BE A POWER OF TWO
	size_t size;
	Record* data;
	
	static Self init(size_t initial = MIN_CAPACITY){
		initial = nextPow2(max(initial, MIN_CAPACITY));
		Self v = {initial, 0, alloc<allocator, Record>(initial)};
		v.clear();
		return v;
	}
	
	ALWAYS_INLINE void deinit(){dealloc<allocator>(data);}
	ALWAYS_INLINE double getLoadFactor(){return (double)size / capacity;}
	ALWAYS_INLINE bool has(const K& key){return getRecord(key);}
	ALWAYS_INLINE void clear(){for(size_t i = 0; i < capacity; i++) data[i].type = RecordType::UNUSED;}
	ALWAYS_INLINE size_t getHash(const K& key){return BIT_MODULO(hasher(key), capacity);}
	//ALWAYS_INLINE size_t nextHash(size_t hash){return BIT_MODULO(hash + 1, capacity);}
	
	Record* getRecord(const K& key){
		size_t hash = getHash(key);
		size_t i = hash;
		while(true){
			Record& record = data[i];
			if(record.type == RecordType::UNUSED) return nullptr;
			if(record.type == RecordType::OCCUPIED && comparer(record.key, key)) return &record;
			i = BIT_MODULO(i + 1, capacity);
			if(i == hash) return nullptr;// In case all unused record slots are deleted and we loop around.
		}
	}
	
	Record& getUnusedRecord(const K& key){
		size_t i = getHash(key);
		while(true){
			Record& record = data[i];
			if(record.type != RecordType::OCCUPIED) return record;
			i = BIT_MODULO(i + 1, capacity);
			// Since we always reserve we don't have to worry about wrapping
			// since there will be always an available spot.
		}
	}
	
	V& get(const K& key){
		Record* record = getRecord(key);
		ZSL_ASSERT(record);
		return record->value;
	}
	
	V& insert(const K& key, const V& value){
		ZSL_ASSERT(getRecord(key) == nullptr);
		reserve(size + 1);
		Record& record = getUnusedRecord(key);
		record = {key, value, RecordType::OCCUPIED};
		size++;
		return record.value;
	}
	
	V& operator[](const K& key){
		Record* record = getRecord(key);
		if(record) return record->value;
		else return insert(key, V{});
	}
	
	void remove(const K& key){
		Record* record = getRecord(key);
		ZSL_ASSERT(record);
		record->type = RecordType::DELETED;
		size--;
	}
	
	void rehash(size_t newCapacity){
		//TODO: Instead of doing this inplace, point to previous hashmap and update as you go.
		if(newCapacity > capacity){
			newCapacity = nextPow2(newCapacity);
			data = realloc<allocator>(newCapacity, data);
			for(size_t i = capacity; i < newCapacity; i++) data[i].type = RecordType::UNUSED;
			capacity = newCapacity;
		}
		
		for(size_t index = 0; index < capacity; index++){
			Record* record = data + index;
			if(record->type != RecordType::OCCUPIED){
				if(record->type == RecordType::DELETED) record->type = RecordType::UNUSED;
				continue;
			}
			
			while(true){
				// Find new record that isn't marked.
				size_t newIndex = getHash(record->key);
				while(data[newIndex].type == RecordType::PLACED) newIndex = BIT_MODULO(newIndex + 1, capacity);
				
				// Mark record.
				record->type = RecordType::PLACED;
				if(newIndex == index) break;
				Record* newRecord = data + newIndex;
				
				// If occupied, swap records and repeat.
				if(newRecord->type == RecordType::OCCUPIED){
					Record tempRecord = *newRecord;
					*newRecord = *record;
					*record = tempRecord;
				}else{
					*newRecord = *record;
					record->type = RecordType::UNUSED;
					break;
				}
			}
		}
		// Unmark all records.
		for(Record* record = data; record < data + capacity; record++){
			if(record->type == RecordType::PLACED) record->type = RecordType::OCCUPIED;
		}
	}
	
	void reserve(size_t value){
		if((double)value / capacity > MAX_LOAD_FACTOR){
			size_t newCapacity = capacity;
			do newCapacity <<= 1; while((double)value / newCapacity > MAX_LOAD_FACTOR);
			rehash(newCapacity);
		}
	}
	
	void clearGravestones(){
		bool deletedGroup = false;
		for(size_t i = 0; i < capacity || deletedGroup; i = BIT_MODULO(i + 1, capacity)){
			Record& record = data[i];
			if(record.type == RecordType::DELETED){
				record.type == RecordType::UNUSED;
				deletedGroup = true;
				continue;
			}else if(record.type == RecordType::UNUSED){
				deletedGroup = false;
				continue;
			}
			// Must be occupied by process-of-elimination.
			if(deletedGroup){
				record.type = RecordType::UNUSED;
				Record& newRecord = getUnusedRecord(record.key);
				newRecord = {record.key, record.value, RecordType::OCCUPIED};
			}
		}
	}
	
	size_t getCollisionScore(){
		size_t score = 0;
		for(size_t i = 0; i < capacity; i++){
			Record& record = data[i];
			if(record.type != RecordType::OCCUPIED) continue;
			size_t hash = getHash(record.key);
			score += BIT_MODULO(i - hash, capacity);
		}
		return score;
	}
	
	void optimize(size_t maxCollisionScore, size_t maxCapacity = SIZE_MAX){
		while(getCollisionScore() > maxCollisionScore && capacity < maxCapacity) rehash(capacity << 1);
	}
	
	struct IteratorBase{
		Self& map;
		size_t i;
		ALWAYS_INLINE IteratorBase(Self& m, size_t initial = 0): map(m), i(initial){next();}
		ALWAYS_INLINE void next(){while(i < map.capacity && map.data[i].type != RecordType::OCCUPIED) i++;}
		ALWAYS_INLINE void operator++(){i++; next();}
		ALWAYS_INLINE bool operator==(IteratorBase& other){return i == other.i;}
		ALWAYS_INLINE bool operator!=(IteratorBase& other){return i != other.i;}
	};
#define ITER(name, type, get) struct name: IteratorBase{using IteratorBase::IteratorBase; ALWAYS_INLINE type& operator*(){return IteratorBase::map.data[IteratorBase::i]get;}}
	ITER(Iterator, Record,);
	ITER(KeyIterator, K,.key);
	ITER(ValueIterator, V,.value);
#undef ITER
	template<typename T>
	struct IteratorType{
		Self& map;
		ALWAYS_INLINE T begin(){return {map, 0};}
		ALWAYS_INLINE T end(){return {map, map.capacity};}
	};
	
	ALWAYS_INLINE Iterator begin(){return Iterator{*this, 0};}
	ALWAYS_INLINE Iterator end(){return {*this, capacity};}
	ALWAYS_INLINE IteratorType<KeyIterator> iterateKeys(){return {*this};}
	ALWAYS_INLINE IteratorType<ValueIterator> iterateValues(){return {*this};}
};

}
