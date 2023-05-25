#include <condition_variable>
#include <functional>
#include <deque>
#include <optional>
#include <semaphore>
#include <cassert>
#include <ranges>
#include <variant>
#include <future>
#include <cmath>

#include "Multithreading/Constant.h"

struct Task
{
	double value;
	bool is_Heavy;

	double Process() const
	{
		const auto iterations = is_Heavy ? HeavyIterations : LightIterations;
		double result = value;
		for (size_t i = 0; i < iterations; i++)
		{
			result = std::sin(std::cos(result));
		}
		return result;
	}
};

//std::vector<std::array<Task, ChunkSize>> GenerateDataSets()
//{
//	std::vector<std::array<Task, ChunkSize>> datasets{ 4 };
//
//	return datasets;
//}

int main()
{
	return 0;
}