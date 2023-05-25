#pragma once

#include <functional>

struct JobDispatchArgs
{
	uint32_t jobIndex;
	uint32_t groupIndex;
};

namespace JobSystem
{
	void Initialize();

	void Excute(const std::function<void()>& job);

	void Dispatch(uint32_t jobCount, uint32_t groupSize, const std::function<void(JobDispatchArgs)>& jobs);

	bool IsBusy();

	void Wait();
}