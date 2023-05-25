#include "JobSystem.h"
#include <algorithm>    // std::max
#include <atomic>    // to use std::atomic<uint64_t>
#include <thread>    // to use std::thread
#include <condition_variable>    // to use std::condition_variable
#include <sstream>
#include <assert.h>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

// Fixed size very simple thread safe ring buffer
template<typename T, size_t capacity>
class ThreadSafeRingBuffer
{
public:
	inline bool push_back(const T& item)
	{
		bool result = false;

		lock.lock();
		size_t next = (head + 1) % capacity;
		if (next != tail)
		{
			data[head] = item;
			head = next;
			result = true;
		}
		lock.unlock();

		return result;
	}

	inline bool pop_fron(T& item)
	{
		bool result = false;
		lock.lock();
		if (tail != head)
		{
			item = data[tail];
			tail = (tail + 1) % capacity;
			result = true;
		}
		lock.unlock();
		return result;
	}

private:
	T data[capacity];
	size_t head = 0;
	size_t tail = 0;
	std::mutex lock;
};

namespace JobSystem
{
	uint32_t numThreads = 0;
	// A thread safe queue to put pending jobs onto the end (with a capacity of 256 jobs). A worker thread can grab a job from the beginning
	ThreadSafeRingBuffer<std::function<void>(), 256> jobPool; 
	// Used in conjunction with the wakeMutex below. Worker threads just sleep when there is no job, and the main thread can wake them up
	std::condition_variable wakeCondition;
	std::mutex wakeMutex;
	uint64_t currentLabe; // Track of the excution of the main thread
	std::atomic<uint64_t> finisheLabel; // Track the state of excution across background worker threads

	void Initialize()
	{
		// Initialize the worker execution state to 0:
		finisheLabel = 0;

		// Retrieve the number of hardware threads in this system:
		auto numCores = std::thread::hardware_concurrency();

		// Calculate the actual number of worker threads we want:
		numThreads = std::max(1u, numCores);
	}

	void Excute(const std::function<void()>& job)
	{

	}

	void Dispatch(uint32_t jobCount, uint32_t groupSize, const std::function<void(JobDispatchArgs)>& jobs)
	{

	}

	bool IsBusy()
	{

	}

	void Wait()
	{

	}
}