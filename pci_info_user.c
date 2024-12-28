#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PROC_PATH "/proc/pci_info"

void read_pci_info() {
    FILE *fp = fopen(PROC_PATH, "r");
    if (!fp) {
        perror("Failed to open proc file");
        return;
    }

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), fp)) {
        printf("%s", buffer);
    }

    fclose(fp);
}

int main(int argc, char *argv[]) {
    if (argc != 1) {
        fprintf(stderr, "Usage: %s\n", argv[0]);
        return EXIT_FAILURE;
    }

    read_pci_info();

    return EXIT_SUCCESS;
}

