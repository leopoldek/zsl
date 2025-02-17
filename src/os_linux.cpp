#include "zsl/core.h"
#include "zsl/atomics.h"
#include "string.h"
#include "unistd.h"
#include "sys/mman.h"
//#include "pthread.h"

namespace zsl{

void* allocateVirtualMemory(size_t size){
	return mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
}

void* reserveVirtualMemory(size_t size){
	return mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
}

void commitVirtualMemory(void* ptr, size_t size){
	void* alignPtr = alignFloor(ptr, sysconf(_SC_PAGE_SIZE));
	size += (size_t)ptr - (size_t)alignPtr;
	mprotect(ptr, size, PROT_READ | PROT_WRITE);
}

void freeVirtualMemory(void* ptr, size_t size){
	void* alignPtr = alignFloor(ptr, sysconf(_SC_PAGE_SIZE));
	size += (size_t)ptr - (size_t)alignPtr;
	munmap(alignPtr, size);
}

struct ThreadData{
	ThreadFunction function;
	void* userData;
};

static void* threadWrapperFunction(void* voidData){
	ThreadData data = *(ThreadData*)voidData;
	dealloc<nalloc>((ThreadData*)voidData);
	data.function(data.userData);
	return nullptr;
}

struct ThreadAttributes{
	pthread_attr_t attr;
	// POSIX spec says these can error out, although on linux they don't. And frankly why would they?
	ThreadAttributes(){pthread_attr_init(&attr); pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);}
	~ThreadAttributes(){pthread_attr_destroy(&attr);}
};

void threadCreate(ThreadFunction function, void* userData){
	static ThreadAttributes attr;
	ThreadData* data = alloc<nalloc, ThreadData>();
	data->function = function;
	data->userData = userData;
	pthread_t threadID;
	int err = pthread_create(&threadID, &attr.attr, threadWrapperFunction, (void*)data);
	if(err != 0) dealloc<nalloc>(data);
}

void Mutex::init(){
	int err = pthread_mutex_init(&mutex, nullptr);
}

void Mutex::lock(){
	pthread_mutex_lock(&mutex);
}

bool Mutex::tryLock(){
	return pthread_mutex_trylock(&mutex);
}

void Mutex::unlock(){
	pthread_mutex_unlock(&mutex);
}

void Mutex::deinit(){
	pthread_mutex_destroy(&mutex);
}

void Condition::init(){
	pthread_cond_init(&cond, nullptr);
}

void Condition::wait(Mutex* mutex){
	pthread_cond_wait(&cond, &mutex->mutex);
}

void Condition::broadcast(){
	pthread_cond_broadcast(&cond);
}

void Condition::signal(){
	pthread_cond_signal(&cond);
}

void Condition::deinit(){
	pthread_cond_destroy(&cond);
}

}
