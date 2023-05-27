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

	inline bool pop_front(T& item)
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
	ThreadSafeRingBuffer<std::function<void()>, 256> jobPool;
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

		// Create all our worker threads while immediately starting them
		for (uint32_t threadID = 0; threadID < numThreads; ++threadID)
		{
			std::thread worker([] {
				std::function<void()> job; // the current job for the thread

				// Infinity loop that a worker thread will do
				while (true)
				{
					if (jobPool.pop_front(job)) // try to grab some job from the jobPool queue
					{
						job();
						finisheLabel.fetch_add(1);
					}
					else
					{
						// no job, put thread in sleep
						std::unique_lock<std::mutex> lock(wakeMutex);
						wakeCondition.wait(lock);
					}
				}
			});
#ifdef _WIN32
			// Do window specific thread set up
			HANDLE handle = (HANDLE)worker.native_handle();

			// Put each thread on to dedicated core
			DWORD_PTR affinityMask = 1ull << threadID;
			DWORD_PTR affinity_result = SetThreadAffinityMask(handle, affinityMask);
			assert(affinity_result > 0);

			//// Increase thread priority:
			//BOOL priority_result = SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST);
			//assert(priority_result != 0);

			// Name the thread:
			std::wstringstream wss;
			wss << "JobSystem_" << threadID;
			HRESULT hr = SetThreadDescription(handle, wss.str().c_str());
			assert(SUCCEEDED(hr));
#endif // _WIN32

			worker.detach(); // forget about this thread, let it do it's job in the infinite loop that we created above
		}
	}

	// This little helper function will not let the system to be deadlocked while the main thread is waiting for something
	inline void poll()
	{
		wakeCondition.notify_one(); // wake our worker thread
		std::this_thread::yield(); // allow this thread to be rescheduled
	}

	void Execute(const std::function<void()>& job)
	{
		// The main thread label state is update
		currentLabe += 1;

		// Try to push a new job until it is pushed successfully
		while (!jobPool.push_back(job))
			poll();

		wakeCondition.notify_one();
	}

	void Dispatch(uint32_t jobCount, uint32_t groupSize, const std::function<void(JobDispatchArgs)>& job)
	{
		if (jobCount == 0 || groupSize == 0)
		{
			return;
		}

		// Calculate the amount of job groups to dispatch
		const uint32_t groupCount = (jobCount + groupSize - 1) / groupSize;

		// THe main thread label state is update
		currentLabe += groupCount;

		for (uint32_t groupIndex = 0; groupIndex < groupCount; ++groupIndex)
		{
			const auto& jobGroup = [jobCount, groupSize, job, groupIndex]()
			{
				const uint32_t groupJobOffset = groupIndex * groupSize;
				const uint32_t groupJobEnd = std::min(groupJobOffset + groupSize, jobCount);

				JobDispatchArgs args;
				args.groupIndex = groupIndex;

				for (uint32_t i = groupJobOffset; i < groupJobEnd; i++)
				{
					args.groupIndex = i;
					job(args);
				}
			};

			while (!jobPool.push_back(jobGroup))
				poll();

			wakeCondition.notify_one();
		}
	}

	bool IsBusy()
	{
		return finisheLabel.load() < currentLabe;
	}

	void Wait()
	{
		while (IsBusy())
		{
			poll();
		}
	}
}