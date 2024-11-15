// Copyright 2024 et-yula

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

void benchmark_sort(int size, int runs) {
  std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
  std::uniform_int_distribution<int> dist(1, 1000000);

  auto start = std::chrono::high_resolution_clock::now();
  for (int run = 0; run < runs; ++run) {
    std::vector<int> arr(size);
    for (int &num : arr) {
      num = dist(rng);
    }

    std::sort(arr.begin(), arr.end());
  }
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> duration = end - start;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <size> <runs>" << std::endl;
    return 1;
  }

  int size = std::atoi(argv[1]);
  int runs = std::atoi(argv[2]);

  benchmark_sort(size, runs);

  return 0;
}
