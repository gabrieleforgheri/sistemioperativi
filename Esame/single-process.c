#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int is_prime(int x) {
    if (x < 2)
        return 0;

    if (x == 2)
        return 1;

    if (x % 2 == 0)
        return 0;

    for (int d = 3; d * d <= x; d += 2) {
        if (x % d == 0)
            return 0;
    }

    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);

    if (n <= 0) {
        fprintf(stderr, "Error: n must be a positive integer\n");
        return 1;
    }

    int *numbers = malloc(n * sizeof(int));

    if (numbers == NULL) {
        perror("malloc");
        return 1;
    }

    FILE *fp = fopen("numbers.txt", "r");

    if (fp == NULL) {
        perror("numbers.txt");
        free(numbers);
        return 1;
    }

    for (int i = 0; i < n; i++) {
        if (fscanf(fp, "%d", &numbers[i]) != 1) {
            fprintf(stderr, "Error: numbers.txt contains fewer than %d integers\n", n);
            fclose(fp);
            free(numbers);
            return 1;
        }
    }

    fclose(fp);

    for (int i = 0; i < n; i++) {
        if (is_prime(numbers[i])) {
            printf("%d is prime\n", numbers[i]);
        } else {
            printf("%d is not prime\n", numbers[i]);
        }
    }

    free(numbers);

    return 0;
}
