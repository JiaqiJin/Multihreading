#pragma once

constexpr size_t WorkerCount = 4;
constexpr size_t ChunkSize = 16'000;
constexpr size_t ChunkCount = 1000;
constexpr size_t LightIterations = 2;
constexpr size_t HeavyIterations = 30;

static_assert(ChunkSize >= WorkerCount, "ChunkSize mayor that WorkCount");
static_assert(ChunkSize % WorkerCount == 0, "ChunkSize % WorkerCount equal to zero");