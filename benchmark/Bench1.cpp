// Copyright 2024 et-yula

#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

size_t get_file_size(const std::string& filename) {
  std::ifstream file(filename, std::ios::binary | std::ios::ate);
  return file.tellg();
}

void sort_and_save_chunk(std::ifstream& input, const std::string& temp_file,
                         size_t chunk_size) {
  std::vector<int> buffer(chunk_size);
  size_t count = 0;

  while (input.read(reinterpret_cast<char*>(buffer.data() + count),
                    (chunk_size - count) * sizeof(int)) &&
         count < chunk_size) {
    count += input.gcount() / sizeof(int);
  }

  buffer.resize(count);
  std::sort(buffer.begin(), buffer.end());

  std::ofstream temp(temp_file, std::ios::binary | std::ios::app);
  temp.write(reinterpret_cast<char*>(buffer.data()), count * sizeof(int));
}

void merge_files(const std::vector<std::string>& temp_files,
                 const std::string& output_file) {
  std::vector<std::ifstream> temp_streams;
  for (const auto& file : temp_files) {
    temp_streams.emplace_back(file, std::ios::binary);
  }

  std::ofstream output(output_file, std::ios::binary);
  std::vector<int> min_elements(temp_streams.size());
  std::vector<bool> eof_flags(temp_streams.size(), false);

  for (size_t i = 0; i < temp_streams.size(); ++i) {
    if (!temp_streams[i].read(reinterpret_cast<char*>(&min_elements[i]),
                              sizeof(int))) {
      eof_flags[i] = true;
    }
  }

  while (true) {
    int min_index = -1;
    int min_value = std::numeric_limits<int>::max();

    for (size_t i = 0; i < min_elements.size(); ++i) {
      if (!eof_flags[i] && min_elements[i] < min_value) {
        min_value = min_elements[i];
        min_index = i;
      }
    }

    if (min_index == -1) break;

    output.write(reinterpret_cast<char*>(&min_value), sizeof(int));

    if (!temp_streams[min_index].read(
            reinterpret_cast<char*>(&min_elements[min_index]), sizeof(int))) {
      eof_flags[min_index] = true;
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " input_file.txt output_file.txt"
              << std::endl;
    return 1;
  }

  const std::string input_file = argv[1];
  const std::string output_file = argv[2];
  const std::string temp_prefix = "temp_chunk_";

  const size_t max_memory_size = get_file_size(input_file) / 8;

  std::ifstream input(input_file, std::ios::binary);
  if (!input) {
    std::cerr << "Error opening input file." << std::endl;
    return 1;
  }

  std::vector<std::string> temp_files;
  size_t chunk_index = 0;

  while (!input.eof()) {
    std::string temp_file = temp_prefix + std::to_string(chunk_index++);
    sort_and_save_chunk(input, temp_file, max_memory_size / sizeof(int));
    temp_files.push_back(temp_file);
  }

  merge_files(temp_files, output_file);

  for (const auto& file : temp_files) {
    remove(file.c_str());
  }

  return 0;
}
