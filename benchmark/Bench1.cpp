// Copyright 2024 et-yula

#include <windows.h>

#include <algorithm>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

void check_handle(HANDLE handle, const std::string &filename) {
  if (handle == INVALID_HANDLE_VALUE) {
    std::cerr << "Error opening file: " << filename << std::endl;
    exit(1);
  }
}

size_t get_file_size(const std::string &filename) {
  HANDLE hFile = CreateFileA(filename.c_str(), GENERIC_READ, 0, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  check_handle(hFile, filename);
  LARGE_INTEGER fileSize;
  if (!GetFileSizeEx(hFile, &fileSize)) {
    CloseHandle(hFile);
    std::cerr << "Error getting file size." << std::endl;
    return 0;
  }
  CloseHandle(hFile);
  return static_cast<size_t>(fileSize.QuadPart);
}

void sort_and_save_chunk(HANDLE input, const std::string &temp_file,
                         size_t chunk_size) {
  std::vector<int> buffer(chunk_size);
  DWORD bytesRead;
  size_t count = 0;

  while (ReadFile(input, buffer.data() + count * sizeof(int),
                  (chunk_size - count) * sizeof(int), &bytesRead, NULL) &&
         bytesRead > 0) {
    count += bytesRead / sizeof(int);
    if (count >= chunk_size) break;
  }

  buffer.resize(count);
  std::sort(buffer.begin(), buffer.end());

  HANDLE temp =
      CreateFileA(temp_file.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                  FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING, NULL);
  check_handle(temp, temp_file);
  DWORD bytesWritten;
  WriteFile(temp, buffer.data(), count * sizeof(int), &bytesWritten, NULL);
  CloseHandle(temp);
}

void merge_files(const std::vector<std::string> &temp_files,
                 const std::string &output_file) {
  std::vector<HANDLE> temp_handles;
  for (const auto &file : temp_files) {
    HANDLE hTemp = CreateFileA(file.c_str(), GENERIC_READ, 0, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    check_handle(hTemp, file);
    temp_handles.push_back(hTemp);
  }

  HANDLE output =
      CreateFileA(output_file.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                  FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING, NULL);
  check_handle(output, output_file);

  std::vector<int> min_elements(temp_handles.size());
  std::vector<bool> eof_flags(temp_handles.size(), false);

  DWORD bytesRead;
  for (size_t i = 0; i < temp_handles.size(); ++i) {
    if (!ReadFile(temp_handles[i], &min_elements[i], sizeof(int), &bytesRead,
                  NULL) ||
        bytesRead == 0) {
      eof_flags[i] = true;
    }
  }

  const size_t buffer_size = 8192 / sizeof(int);
  std::vector<int> buffer;
  buffer.reserve(buffer_size);

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

    buffer.push_back(min_value);

    if (!ReadFile(temp_handles[min_index], &min_elements[min_index],
                  sizeof(int), &bytesRead, NULL) ||
        bytesRead == 0) {
      eof_flags[min_index] = true;
    }

    if (buffer.size() == buffer_size) {
      DWORD bytesWritten;
      WriteFile(output, buffer.data(), buffer.size() * sizeof(int),
                &bytesWritten, NULL);
      buffer.clear();
    }
  }

  if (!buffer.empty()) {
    DWORD bytesWritten;
    WriteFile(output, buffer.data(), buffer.size() * sizeof(int), &bytesWritten,
              NULL);
  }

  CloseHandle(output);
  for (auto handle : temp_handles) {
    CloseHandle(handle);
  }
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " input_file.txt output_file.txt"
              << std::endl;
    return 1;
  }
  const std::string input_file = argv[1];
  const std::string output_file = argv[2];
  const std::string temp_prefix = "temp_chunk_";

  const size_t max_memory_size = get_file_size(input_file) / 8;

  HANDLE input = CreateFileA(input_file.c_str(), GENERIC_READ, 0, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  check_handle(input, input_file);

  std::vector<std::string> temp_files;
  size_t chunk_index = 0;

  for (int i = 0; i < 8; i++) {
    std::string temp_file = temp_prefix + std::to_string(chunk_index++);
    sort_and_save_chunk(input, temp_file, max_memory_size / sizeof(int));
    temp_files.push_back(temp_file);
  }

  CloseHandle(input);

  merge_files(temp_files, output_file);

  for (const auto &file : temp_files) {
    remove(file.c_str());
  }

  return 0;
}
