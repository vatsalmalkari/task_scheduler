#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s [scheduler|arithmetic|cancel]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "scheduler") == 0) {
        extern int scheduler_test_main();
        return scheduler_test_main();
    } else if (strcmp(argv[1], "arithmetic") == 0) {
        extern int arithmetic_test_main();
        return arithmetic_test_main();
    } else if (strcmp(argv[1], "cancel") == 0) {
        extern int cancellation_edge_test();
        return cancellation_edge_test();
    } else {
        printf("Unknown test: %s\n", argv[1]);
        return 1;
    }
}