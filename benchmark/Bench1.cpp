// Copyright 2024 et-yula

#include <windows.h>

#include <algorithm>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

__declspec(dllimport) ssize_t __cdecl lab2_open(const char *path);
__declspec(dllimport) ssize_t __cdecl lab2_close(int fd);
__declspec(dllimport) ssize_t
    __cdecl lab2_read(int fd, unsigned char *buf, size_t count);
__declspec(dllimport) ssize_t
    __cdecl lab2_write(int fd, unsigned char *buf, size_t count);
__declspec(dllimport) off_t __cdecl lab2_lseek(int fd, off_t offset);
__declspec(dllimport) ssize_t __cdecl lab2_fsync(int fd);
__declspec(dllimport) void __cdecl reportLeaks();

void check_handle(ssize_t handle, const std::string &filename) {
  if (handle < 0) {
    std::cerr << "Error opening file: " << filename << std::endl;
    exit(1);
  }
}

size_t get_file_size(const std::string &filename) {
  HANDLE hFile = CreateFileA(filename.c_str(), GENERIC_READ, 0, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    std::cerr << "Error opening file: " << filename << std::endl;
    exit(1);
  }
  LARGE_INTEGER fileSize;
  if (!GetFileSizeEx(hFile, &fileSize)) {
    CloseHandle(hFile);
    std::cerr << "Error getting file size." << std::endl;
    return 0;
  }
  CloseHandle(hFile);
  return static_cast<size_t>(fileSize.QuadPart);
}

void sort_and_save_chunk(int input_fd, const std::string &temp_file,
                         size_t chunk_size) {
  std::vector<int> buffer(chunk_size);

  ssize_t bytesRead;
  if (lab2_read(input_fd, reinterpret_cast<unsigned char *>(buffer.data()),
                chunk_size * sizeof(int)) != 0) {
    std::cerr << "Error read file in sort_and_save_chunk" << std::endl;
    return;
  }

  std::sort(buffer.begin(), buffer.end());

  ssize_t temp_fd = lab2_open(temp_file.c_str());
  check_handle(temp_fd, temp_file);

  lab2_write(temp_fd, reinterpret_cast<unsigned char *>(buffer.data()),
             chunk_size * sizeof(int));
  lab2_close(temp_fd);
}

void merge_files(const std::vector<std::string> &temp_files,
                 const std::string &output_file) {
  std::vector<int> temp_fds;

  for (const auto &file : temp_files) {
    ssize_t hTemp = lab2_open(file.c_str());
    check_handle(hTemp, file);
    temp_fds.push_back(hTemp);
  }

  ssize_t output_fd = lab2_open(output_file.c_str());
  check_handle(output_fd, output_file);

  std::vector<int> min_elements(temp_fds.size());
  std::vector<bool> eof_flags(temp_fds.size(), false);

  ssize_t bytesRead;

  for (size_t i = 0; i < temp_fds.size(); ++i) {
    if (lab2_read(temp_fds[i],
                  reinterpret_cast<unsigned char *>(&min_elements[i]),
                  sizeof(int)) != 0) {
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

    if (lab2_read(temp_fds[min_index],
                  reinterpret_cast<unsigned char *>(&min_elements[min_index]),
                  sizeof(int)) != 0) {
      eof_flags[min_index] = true;
    }

    if (buffer.size() == buffer_size) {
      lab2_write(output_fd, reinterpret_cast<unsigned char *>(buffer.data()),
                 buffer.size() * sizeof(int));
      buffer.clear();
    }
  }

  if (!buffer.empty()) {
    lab2_write(output_fd, reinterpret_cast<unsigned char *>(buffer.data()),
               buffer.size() * sizeof(int));
  }
  lab2_close(output_fd);

  for (auto fd : temp_fds) {
    lab2_close(fd);
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

  ssize_t input_fd = lab2_open(input_file.c_str());

  check_handle(input_fd, input_file);

  std::vector<std::string> temp_files;
  size_t chunk_index = 0;

  for (int i = 0; i < 8; i++) {
    std::string temp_file = temp_prefix + std::to_string(chunk_index++);
    sort_and_save_chunk(input_fd, temp_file, max_memory_size / sizeof(int));
    temp_files.push_back(temp_file);
  }

  lab2_close(input_fd);

  merge_files(temp_files, output_file);

  for (const auto &file : temp_files) {
    remove(file.c_str());
  }

  reportLeaks();
  return 0;
}
