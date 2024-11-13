// Copyright 2024 et-yula

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#include <cstdio>

void execute_cd(char *path) {
  if (strcmp(path, "..") == 0) {
    SetCurrentDirectory("..");
  } else if (SetCurrentDirectory(path) == 0) {
    printf("Error: Cannot change directory to %s\n", path);
  }
}

void execute_ls() {
  WIN32_FIND_DATA findFileData;
  HANDLE hFind = FindFirstFile("*", &findFileData);

  if (hFind == INVALID_HANDLE_VALUE) {
    printf("Error: Unable to list directory.\n");
    return;
  }
  do {
    printf("%s\n", findFileData.cFileName);
  } while (FindNextFile(hFind, &findFileData) != 0);
  FindClose(hFind);
}

void execute_program(char *program, char **args) {
  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  DWORD exitCode;
  clock_t start, end;
  double time_spent;

  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);
  memset(&pi, 0, sizeof(pi));

  char cmd[1024] = "";
  snprintf(cmd, sizeof(cmd), "%s", program);
  for (int i = 1; args[i] != NULL; i++) {
    snprintf(cmd + strlen(cmd), sizeof(cmd) - strlen(cmd), " %s", args[i]);
  }

  HANDLE hToken = NULL;

  start = clock();
  if (!CreateProcessInternalA(hToken, NULL, cmd, NULL, NULL, FALSE, 0, NULL,
                              NULL, &si, &pi, NULL)) {
    printf("Error: Failed to start program %s. Error code: %lun", program,
           GetLastError());
    return;
  }

  WaitForSingleObject(pi.hProcess, INFINITE);
  end = clock();

  time_spent = (double)(end - start) / CLOCKS_PER_SEC;
  printf("Execution time: %.3f secondsn", time_spent);

  GetExitCodeProcess(pi.hProcess, &exitCode);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
}

void print_prompt() {
  char cwd[1024];
  if (GetCurrentDirectory(sizeof(cwd), cwd)) {
    printf("%s$ ", cwd);
  } else {
    printf("$ ");
  }
}

int main() {
  char input[1024];
  char *args[64];
  char *token;
  char *saveptr;

  while (1) {
    print_prompt();
    fflush(stdout);
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0;  // Remove newline character

    int i = 0;
    token = strtok_r(input, " ", &saveptr);
    while (token != NULL) {
      args[i++] = token;
      token = strtok_r(NULL, " ", &saveptr);
    }
    args[i] = NULL;

    if (args[0] == NULL) continue;

    if (strcmp(args[0], "exit") == 0) {
      break;
    } else if (strcmp(args[0], "cd") == 0) {
      if (args[1] != NULL) {
        execute_cd(args[1]);
      } else {
        printf("Usage: cd <directory>\n");
      }
    } else if (strcmp(args[0], "ls") == 0) {
      execute_ls();
    } else if (args[0][0] == '.' && args[0][1] == '/') {
      execute_program(args[0], args);
    } else {
      printf("Command not recognized.\n");
    }
  }

  return 0;
}
