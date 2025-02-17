#include "stdlib.h"
#include "string.h"
#include "zsl/core.h"
#include "zsl/atomics.h"

namespace zsl{

struct BlockPointer{
	BlockPointer* next;
};

using PoolPointer = TaggedPointer<BlockPointer>;

struct BlockData{
	size_t size;
	size_t alignment;
	
	static BlockData* get(void* ptr){
		return (BlockData*)ptr - 1;
	}
	
	BlockPointer* getBlock(){
		return alignFloor((BlockPointer*)this, alignment);
	}
};
static_assert(alignof(BlockPointer*) <= alignof(BlockData));

void* buildBlock(BlockPointer* ptr, size_t size, size_t alignment){
	BlockData* data = align((BlockData*)ptr + 1, alignment) - 1;
	*data = {size, alignment};
	return (void*)(data + 1);
}

size_t getIndex(size_t size, size_t alignment){
	// All blocks are aligned to BlockData.
	// If alignment greater than sizeof of BlockData,
	// add 'alignment - sizeof(BlockData)' to make sure
	// We have room to align the data.
	size_t padding = alignment > sizeof(BlockData) ? alignment - sizeof(BlockData) : 0;
	// First couple pools are so small just ignore them.
	return log2Ceil(max(sizeof(size_t), size + padding));
}

PoolPointer* getPool(size_t index){
	static PoolPointer pools[sizeof(size_t) * CHAR_BIT];
	return &pools[index];
}

#define CALCULATE_BLOCK_SIZE(index) (size_t(1) << (index))

#ifndef NDEBUG
//TODO have pushBlock/popBlock modify this.
NallocInfo nallocInfos[sizeof(size_t) * CHAR_BIT];
static bool nallocInfosDummy = [](){
	for(size_t i = 0; i < RAW_ARRAY_SIZE(nallocInfos); i++){
		nallocInfos[i].blockSize = CALCULATE_BLOCK_SIZE(i);
	}
};
#endif

BlockPointer* allocBlock(size_t index){
	static GlobalVar<Arena> nodeArena;
	index = CALCULATE_BLOCK_SIZE(index);
	index += sizeof(BlockData);
	return (BlockPointer*)nodeArena.var.alloc(nullptr, index, alignof(BlockData));
}

void pushBlock(size_t index, BlockPointer* block){
	PoolPointer* pool = getPool(index);
	PoolPointer old = pool->load();
	do{
		block->next = old.ptr;
	}while(!pool->store(&old, block));
}

BlockPointer* popBlock(size_t index){
	PoolPointer* pool = getPool(index);
	PoolPointer old = pool->load();
	BlockPointer* block;
	do{
		if(old.ptr == nullptr) return allocBlock(index);
		block = old.ptr;
	}while(!pool->store(&old, block->next));
	return block;
}

void* nalloc(void* ptr, size_t size, size_t alignment){
	// Allocate new block.
	if(!ptr) return buildBlock(popBlock(getIndex(size, alignment)), size, alignment);
	// Get existing block.
	BlockData* blockData = BlockData::get(ptr);
	// Alignment must be same as previous allocations of this data.
	alignment = blockData->alignment;
	// Get index of existing block.
	size_t index = getIndex(blockData->size, alignment);
	if(size == 0){
		// Deallocate existing block.
		pushBlock(index, blockData->getBlock());
		return nullptr;
	}
	
	// Get index of new block.
	size_t newIndex = getIndex(size, alignment);
	// If size is smaller, don't reallocate, just resize.
	if(newIndex <= index){
		blockData->size = size;
		return ptr;
	}
	
	// Size is larger so we must allocate new block.
	void* newPtr = buildBlock(popBlock(newIndex), size, alignment);
	// Copy data to new block.
	memcpy(newPtr, ptr, blockData->size);
	// Deallocate the old block.
	pushBlock(index, blockData->getBlock());
	return newPtr;
}

#ifndef NDEBUG
ArrayView<NallocInfo> nallocGetInfo(){
	return {RAW_ARRAY_SIZE(nallocInfos), nallocInfos};
}
#endif

void Arena::init(){
	// We assume data is aligned to all conceivable alignments.
	char* reservation = (char*)reserveVirtualMemory(MAX_ARENA_SIZE);
	capacity = reservation;
	mark = reservation;
	data = reservation;
	mutex.init();
}

void Arena::deinit(){
	freeVirtualMemory(data, MAX_ARENA_SIZE);
}

void* Arena::alloc(void* ptr, size_t size, size_t alignment){
	if(size == 0) return nullptr;
	
	BlockData* oldData = nullptr;
	char* oldMark = atomicLoad(&mark);
	BlockData* newData;
	void* newPtr;
	char* newMark;// What we set the mark to after CAS.
	
	if(ptr){
		oldData = (BlockData*)ptr - 1;
		alignment = oldData->alignment;
	}
	
	do{
		if(ptr && ptr == (void*)(oldMark - oldData->size)){
			newData = oldData;
			newPtr = ptr;
			newMark = (char*)newPtr + size;
		}else{
			newData = align((BlockData*)align(oldMark, alignof(BlockData)) + 1, alignment) - 1;
			newPtr = (void*)(newData + 1);
			newMark = (char*)newPtr + size;
		}
	}while(!atomicCompareExchangeWeak(&mark, &oldMark, newMark));
	
	// https://stackoverflow.com/questions/6086912/double-checked-lock-singleton-in-c11
	if(newMark > atomicLoad(&capacity, ORDER_ACQUIRE)){
		LockScope lock(mutex);
		if(newMark > capacity){
			size_t markInt = newMark - data;
			markInt += MAX_ARENA_SIZE - (markInt % MAX_ARENA_SIZE);
			char* newCapacity = data + markInt;
			commitVirtualMemory(capacity, newCapacity - capacity);
			atomicStore(&capacity, newCapacity, ORDER_RELEASE);
		}
	}
	
	*newData = {size, alignment};
	if(oldData && newData != oldData) memcpy(newPtr, ptr, oldData->size);
	
	return newPtr;
}

void Arena::reset(){
	mark = data;
}

Arena* getTempArena(){static GlobalVar<Arena> temp; return &temp.var;}
void resetTalloc(){getTempArena()->reset();}
void* talloc(void* ptr, size_t size, size_t alignment){return getTempArena()->alloc(ptr, size, alignment);}

}
