#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LEN 1000000

static int is_positive_number(const char *s) {
    if (s == NULL || *s == '\0')
        return 0;

    for (size_t i = 0; s[i] != '\0'; i++) {
        if (!isdigit((unsigned char)s[i]))
            return 0;
    }

    return s[0] != '0';
}

static void next_round(const char *current, char *next) {
    size_t i = 0;
    size_t j = 0;

    while (current[i] != '\0') {
        char digit = current[i];
        int count = 1;

        while (current[i + count] == digit)
            count++;

        j += sprintf(next + j, "%d%c", count, digit);
        i += count;
    }

    next[j] = '\0';
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <seed> <iterations>\n", argv[0]);
        return 1;
    }

    if (!is_positive_number(argv[1]) || !is_positive_number(argv[2])) {
        fprintf(stderr, "Both seed and iterations must be positive integers.\n");
        return 1;
    }

    int n = atoi(argv[2]);

    char *current = malloc(MAX_LEN);
    char *next = malloc(MAX_LEN);

    if (current == NULL || next == NULL) {
        perror("malloc");
        return 1;
    }

    strncpy(current, argv[1], MAX_LEN - 1);
    current[MAX_LEN - 1] = '\0';

    for (int round = 1; round <= n; round++) {
        next_round(current, next);
        printf("%s\n", next);

        char *tmp = current;
        current = next;
        next = tmp;
    }

    free(current);
    free(next);

    return 0;
}
