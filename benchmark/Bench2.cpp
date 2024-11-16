// Copyright 2024 et-yula

#include <algorithm>
#include <iostream>
#include <vector>

void benchmark_sort(int size, int runs) {
  unsigned int seed = 123;
  for (int run = 0; run < runs; ++run) {
    std::vector<int> arr(size);
    for (int &num : arr) {
      num = rand_r(&seed) % 1000000;
    }
    std::sort(arr.begin(), arr.end());
  }
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
