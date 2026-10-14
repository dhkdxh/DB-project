#include "bptree.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Error: Instruction is not available\n");
        return 1;
    }

    const char *command = argv[1];
    const char *index_file = argv[2];

    if (strcmp(command, "-c") == 0) {
        if (argc != 4) {
            printf("Usage: %s -c index_file b\n", argv[0]);
            return 1;
        }
        int b = atoi(argv[3]);
        if (b < 3) return 1;

        create_index(index_file, b);
    } else if (strcmp(command, "-i") == 0) {
        if (argc != 4) {
            printf("Usage: %s -i index_file data_file\n", argv[0]);
            return 1;
        }
        const char *data_file = argv[3];
        insert(index_file, data_file);
    } else if (strcmp(command, "-d") == 0) {
        if (argc != 4) {
            printf("Usage: %s -d index_file data_file\n", argv[0]);
            return 1;
        }
        const char *data_file = argv[3];
        delete(index_file, data_file);
    } else if (strcmp(command, "-s") == 0) {
        if (argc != 4) {
            printf("Usage: %s -s index_file key\n", argv[0]);
            return 1;
        }
        key_t key = atoll(argv[3]);
        BPlusTree *tree = load_tree(index_file);
        search_single(tree, key);
        free_bptree(tree);
    } else if (strcmp(command, "-r") == 0) {
        if (argc != 5) {
            printf("Usage: %s -r index_file start_key end_key\n", argv[0]);
            return 1;
        }
        key_t start_key = atoll(argv[3]);
        key_t end_key = atoll(argv[4]);
        BPlusTree *tree = load_tree(index_file);
        search_ranged(tree, start_key, end_key);
        free_bptree(tree);
    } else {
        printf("Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}
