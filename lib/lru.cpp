// Copyright 2024 et-yula

#include <windows.h>

#include <iostream>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#define BLOCK_SIZE 8192

struct lru_node {
  int fd;
  off_t offset;
  void *data;
  lru_node *prev;
  lru_node *next;
};

static std::unordered_map<void *, size_t>
    allocations;                   // To track allocated memory
static size_t totalAllocated = 0;  // Total allocated memory

// Custom _malloc function
void *_malloc(size_t size) {
  void *ptr = malloc(size);
  if (ptr) {
    allocations[ptr] = size;  // Track the allocated size
    totalAllocated += size;   // Update total allocated size
  }
  return ptr;
}

size_t lru_current_size = 0;

// Custom _free function
void _free(void *ptr) {
  if (ptr) {
    auto it = allocations.find(ptr);
    if (it != allocations.end()) {
      totalAllocated -= it->second;  // Update total allocated size
      allocations.erase(it);         // Remove from tracking
    } else {
      std::cerr << "Warning: Attempting to free untracked pointer: " << ptr
                << std::endl;
    }
    free(ptr);
  }
}

// Function to report memory leaks
__declspec(dllexport) void __cdecl reportLeaks() {
  if (allocations.empty()) {
    std::cout << "No memory leaks detected." << std::endl;
  } else {
    std::cout << "Memory leaks detected:" << std::endl;
    for (const auto &pair : allocations) {
      std::cout << "Leaked pointer: " << pair.first << ", Size: " << pair.second
                << " bytes" << std::endl;
    }
  }
  if (totalAllocated) {
    std::cout << "Total allocated memory: " << totalAllocated << " bytes"
              << std::endl;
    std::cout << "Total lru_current_size: " << lru_current_size << std::endl;
  }
}

size_t lru_capacity = 16000;
lru_node *lru_head = nullptr;
lru_node *lru_tail = nullptr;
struct pair_hash {
  template <class T1, class T2>
  std::size_t operator()(const std::pair<T1, T2> &p) const {
    auto h1 = std::hash<T1>{}(p.first);
    auto h2 = std::hash<T2>{}(p.second);
    return h1 ^ h2;
  }
};
std::unordered_map<std::pair<int, off_t>, lru_node *, pair_hash> lru_cache_map;

void lru_sync(int fd, off_t offset, void *data);

void lru_remove_node(lru_node *node) {
  if (!node) return;

  if (node->prev) {
    node->prev->next = node->next;
  } else {
    lru_tail = node->next;
  }

  if (node->next) {
    node->next->prev = node->prev;
  } else {
    lru_head = node->prev;
  }

  lru_cache_map.erase({node->fd, node->offset});
  lru_sync(node->fd, node->offset, node->data);
  _free(node->data);
  _free(node);
  lru_current_size--;
}

void *lru_find_in_cache(int fd, off_t offset) {
  auto it = lru_cache_map.find({fd, offset});
  if (it != lru_cache_map.end()) {
    lru_node *node = it->second;

    if (node->prev) {
      node->prev->next = node->next;
    } else {
      lru_tail = node->next;
    }

    if (node->next) {
      node->next->prev = node->prev;
    } else {
      lru_head = node->prev;
    }

    node->prev = lru_head;
    node->next = nullptr;
    lru_head->next = node;
    lru_head = node;
    return node->data;
  }
  return nullptr;
}

void lru_add_to_cache(int fd, off_t offset, void *data) {
  auto it = lru_cache_map.find({fd, offset});
  if (it != lru_cache_map.end()) {
    lru_node *node = it->second;
    if (node->data != data) _free(node->data);
    node->data = data;

    if (node->prev) {
      node->prev->next = node->next;
    } else {
      lru_tail = node->next;
    }

    if (node->next) {
      node->next->prev = node->prev;
    } else {
      lru_head = node->prev;
    }

    node->prev = lru_head;
    node->next = nullptr;
    lru_head->next = node;
    lru_head = node;
    return;
  }

  lru_node *new_node = reinterpret_cast<lru_node *>(_malloc(sizeof(lru_node)));
  new_node->fd = fd;
  new_node->offset = offset;
  new_node->data = data;
  new_node->next = nullptr;

  if (!lru_head) {
    new_node->prev = nullptr;
    lru_head = new_node;
    lru_tail = new_node;
  } else {
    new_node->prev = lru_head;
    lru_head->next = new_node;
  }
  lru_head = new_node;

  lru_cache_map[{fd, offset}] = new_node;
  lru_current_size++;

  if (lru_current_size > lru_capacity) {
    lru_remove_node(lru_tail);
  }
}

void lru_remove_all_by_fd(int fd) {
  lru_node *current = lru_head;
  while (current) {
    lru_node *next = current->prev;
    if (current->fd == fd) {
      lru_remove_node(current);
    }
    current = next;
  }
}

struct my_file {
  HANDLE handle;
  ssize_t size;
  off_t cur;
};

std::vector<my_file *> files;

void lru_sync(int fd, off_t offset, void *data) {
  my_file *mf = files[fd];
  DWORD bytesWritten;
  SetFilePointer(mf->handle, offset, NULL, FILE_BEGIN);
  WriteFile(mf->handle, data, BLOCK_SIZE, &bytesWritten, NULL);
}

__declspec(dllexport) ssize_t __cdecl lab2_open(const char *path) {
  my_file *mf = reinterpret_cast<my_file *>(_malloc(sizeof(my_file)));

  mf->handle =
      CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
                  FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING, NULL);

  if (mf->handle == INVALID_HANDLE_VALUE) {
    _free(mf);
    return -1;
  }

  mf->cur = 0;
  DWORD fileSizeHigh;
  mf->size = GetFileSize(mf->handle, &fileSizeHigh);
  if (mf->size == INVALID_FILE_SIZE) {
    CloseHandle(mf->handle);
    return -1;
  }
  files.push_back(mf);
  return files.size() - 1;
}

__declspec(dllexport) ssize_t __cdecl lab2_close(int fd) {
  if (fd < 0 && fd >= files.size()) return -1;
  if (!files[fd]) return -2;
  lru_remove_all_by_fd(fd);
  HANDLE hFile = files[fd]->handle;
  _free(files[fd]);
  return CloseHandle(hFile) ? 0 : -1;
}

__declspec(dllexport) ssize_t
    __cdecl lab2_read(int fd, unsigned char *buf, size_t count) {
  if (fd < 0 && fd >= files.size()) return -1;
  if (!files[fd]) return -2;
  if (files[fd]->cur + count > files[fd]->size) return -3;
  my_file *mf = files[fd];
  size_t from = mf->cur;
  mf->cur = mf->cur / BLOCK_SIZE * BLOCK_SIZE;
  size_t buf_cur = 0;
  size_t to = (mf->cur + count + (BLOCK_SIZE - 1)) /
              BLOCK_SIZE;  // округляем в большую сторону
  for (int block = from / BLOCK_SIZE; block <= to; block++) {
    unsigned char *data =
        (unsigned char *)lru_find_in_cache(fd, block * BLOCK_SIZE);
    if (!data) {
      DWORD bytesRead;
      SetFilePointer(mf->handle, mf->cur, NULL, FILE_BEGIN);
      data = (unsigned char *)_malloc(BLOCK_SIZE + 1);
      BOOL result = ReadFile(mf->handle, data, BLOCK_SIZE, &bytesRead, NULL);
      if (!result) {
        return -4;
      }
      lru_add_to_cache(fd, block * BLOCK_SIZE, data);
    }
    for (int i = 0; i < BLOCK_SIZE; i++) {
      if (mf->cur - block * BLOCK_SIZE >= BLOCK_SIZE) continue;
      if (buf_cur >= count) break;
      if (mf->cur >= from && mf->cur - from < count) {
        buf[buf_cur++] = data[mf->cur % BLOCK_SIZE];
      }
      mf->cur++;
    }
  }
  return 0;
}

__declspec(dllexport) ssize_t
    __cdecl lab2_write(int fd, unsigned char *buf, size_t count) {
  if (fd < 0 && fd >= files.size()) return -1;
  if (!files[fd]) return -2;
  my_file *mf = files[fd];
  size_t from = mf->cur;
  mf->cur = mf->cur / BLOCK_SIZE * BLOCK_SIZE;
  size_t to = (mf->cur + count + (BLOCK_SIZE - 1)) /
              BLOCK_SIZE;  // округляем в большую сторону
  size_t buf_cur = 0;
  for (int block = from / BLOCK_SIZE; block <= to; block++) {
    unsigned char *data =
        (unsigned char *)lru_find_in_cache(fd, block * BLOCK_SIZE);
    bool from_file = false;
    if (!data) {
      from_file = true;
      DWORD bytesRead;
      SetFilePointer(mf->handle, mf->cur, NULL, FILE_BEGIN);
      data = (unsigned char *)_malloc(BLOCK_SIZE + 2);
      BOOL result = ReadFile(mf->handle, data, BLOCK_SIZE, &bytesRead, NULL);
      if (!result) {
        _free(data);
        return -3;
      }
      lru_add_to_cache(fd, block * BLOCK_SIZE, data);
    }
    for (int i = 0; i < BLOCK_SIZE; i++) {
      if (mf->cur - block * BLOCK_SIZE >= BLOCK_SIZE) continue;
      if (buf_cur >= count) return 0;
      if (mf->cur >= from && mf->cur - from < count) {
        data[mf->cur % BLOCK_SIZE] = buf[buf_cur++];
      }
      mf->cur++;
    }
    if (mf->size < mf->cur) mf->size = mf->cur + 1;
  }
  return 0;
}

__declspec(dllexport) off_t __cdecl lab2_lseek(int fd, off_t offset) {
  if (fd < 0 && fd >= files.size()) return -1;
  if (!files[fd]) return -2;
  files[fd]->cur = offset;
  return offset;
}

__declspec(dllexport) ssize_t __cdecl lab2_fsync(int fd) {
  if (fd < 0 && fd >= files.size()) return -1;
  if (!files[fd]) return -2;

  lru_node *current = lru_head;
  while (current) {
    lru_node *next = current->prev;
    if (current->fd == fd) {
      lru_sync(current->fd, current->offset, current->data);
    }
    current = next;
  }
  return 0;
}
