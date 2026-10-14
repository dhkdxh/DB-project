#include "bptree.h"

#define BPTREE_MAGIC 0x5450422B 

void save_tree(const BPlusTree *tree, const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file '%s' for writing.\n", filename);
        exit(EXIT_FAILURE);
    }

    int magic = BPTREE_MAGIC;
    fwrite(&magic, sizeof(int), 1, fp);
    fwrite(&tree->b, sizeof(int), 1, fp);
    fwrite(&tree->root_id, sizeof(node_id_t), 1, fp);
    fwrite(&tree->next_node_id, sizeof(node_id_t), 1, fp);

    /* 트리 내 유효 노드 개수 계산 */
    int valid_node_count = 0;
    for (int i = 0; i < tree->next_node_id; i++) {
        if (tree->nodes[i] != NULL) {
            valid_node_count++;
        }
    }
    fwrite(&valid_node_count, sizeof(int), 1, fp);

    /* 각 노드 데이터 직렬화 */
    for (int i = 0; i < tree->next_node_id; i++) {
        Node *node = tree->nodes[i];
        if (!node) continue;

        fwrite(&node->node_id, sizeof(node_id_t), 1, fp);
        int is_leaf = node->is_leaf ? 1 : 0;
        fwrite(&is_leaf, sizeof(int), 1, fp);
        fwrite(&node->num_keys, sizeof(int), 1, fp);

        if (node->num_keys>0) {
            fwrite(node->keys, sizeof(key_t), node->num_keys, fp);
        }

        if (node->is_leaf) {
            if (node->num_keys>0) {
                fwrite(node->values, sizeof(val_t), node->num_keys, fp);
            }
            fwrite(&node->next_node_id, sizeof(node_id_t), 1, fp);
        } else {
            fwrite(node->children, sizeof(node_id_t), node->num_keys + 1, fp);
        }
    }

    fclose(fp);
}

BPlusTree *load_tree(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        printf("Error: Index file '%s' does not exist.\n", filename);
        exit(EXIT_FAILURE);
    }

    int magic = 0;
    if (fread(&magic, sizeof(int), 1, fp) != 1 || magic != BPTREE_MAGIC) {
        fprintf(stderr, "Error: Invalid or corrupted index file '%s'.\n", filename);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    BPlusTree *tree = (BPlusTree *)malloc(sizeof(BPlusTree));
    if (!tree) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    if (fread(&tree->b, sizeof(int), 1, fp) != 1 ||
        fread(&tree->root_id, sizeof(node_id_t), 1, fp) != 1 ||
        fread(&tree->next_node_id, sizeof(node_id_t), 1, fp) != 1) {
        fprintf(stderr, "Error: Failed to read tree header from '%s'.\n", filename);
        fclose(fp);
        free(tree);
        exit(EXIT_FAILURE);
    }

    int valid_node_count = 0;
    if (fread(&valid_node_count, sizeof(int), 1, fp) != 1) {
        fprintf(stderr, "Error: Failed to read node count from '%s'.\n", filename);
        fclose(fp);
        free(tree);
        exit(EXIT_FAILURE);
    }

    tree->node_capacity = (tree->next_node_id<64) ? 64 : tree->next_node_id;
    tree->nodes = (Node **)calloc(tree->node_capacity, sizeof(Node *));

    for (int i = 0; i < valid_node_count; i++) {
        node_id_t nid;
        int is_leaf;
        int num_keys;

        if (fread(&nid, sizeof(node_id_t), 1, fp)!= 1 ||
            fread(&is_leaf, sizeof(int), 1, fp)!= 1 ||
            fread(&num_keys, sizeof(int), 1, fp)!= 1) {
            fprintf(stderr, "Error: Failed to read node header.\n");
            fclose(fp);
            free_bptree(tree);
            exit(EXIT_FAILURE);
        }

        Node *node = (Node *)malloc(sizeof(Node));
        node->node_id = nid;
        node->is_leaf = (is_leaf != 0);
        node->num_keys = num_keys;
        node->keys = (key_t *)calloc(tree->b + 2, sizeof(key_t));

        if (num_keys > 0) {
            if (fread(node->keys, sizeof(key_t), num_keys, fp) != (size_t)num_keys) {
                fprintf(stderr, "Error: Failed to read keys.\n");
                fclose(fp);
                free_bptree(tree);
                exit(EXIT_FAILURE);
            }
        }

        if (node->is_leaf) {
            node->values = (val_t *)calloc(tree->b + 2, sizeof(val_t));
            node->children = NULL;
            if (num_keys > 0) {
                if (fread(node->values, sizeof(val_t), num_keys, fp) != (size_t)num_keys) {
                    fprintf(stderr, "Error: Failed to read values.\n");
                    fclose(fp);
                    free_bptree(tree);
                    exit(EXIT_FAILURE);
                }
            }
            if (fread(&node->next_node_id, sizeof(node_id_t), 1, fp) != 1) {
                fprintf(stderr, "Error: Failed to read next_node_id.\n");
                fclose(fp);
                free_bptree(tree);
                exit(EXIT_FAILURE);
            }
        } else {
            node->values = NULL;
            node->next_node_id = INVALID_NODE_ID;
            node->children = (node_id_t *)calloc(tree->b + 3, sizeof(node_id_t));
            if (fread(node->children, sizeof(node_id_t), num_keys + 1, fp) != (size_t)(num_keys + 1)) {
                fprintf(stderr, "Error: Failed to read children.\n");
                fclose(fp);
                free_bptree(tree);
                exit(EXIT_FAILURE);
            }
        }

        tree->nodes[nid] = node;
    }

    fclose(fp);
    return tree;
}

void create_index(const char *index_file, int b) {
    BPlusTree *tree = create_bptree(b);
    save_tree(tree, index_file);
    free_bptree(tree);
}

void insert(const char *index_file, const char *data_file) {
    BPlusTree *tree = load_tree(index_file);

    FILE *fp = fopen(data_file, "r");
    if (!fp) {
        fprintf(stderr, "Error: Data file '%s' does not exist.\n", data_file);
        free_bptree(tree);
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        /* 공백 줄 건너뛰어야함 */
        char *ptr = line;
        while (*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n') {
            ptr++;
        }
        if (*ptr == '\0') continue;

        key_t key;
        val_t value;
        if (sscanf(ptr, "%lld,%lld", &key, &value) == 2) {
            insert_single_data(tree, key, value);
        }
    }

    fclose(fp);
    save_tree(tree, index_file);
    free_bptree(tree);
}

void delete(const char *index_file, const char *data_file) {
    BPlusTree *tree = load_tree(index_file);

    FILE *fp = fopen(data_file, "r");
    if (!fp) {
        fprintf(stderr, "Error: Data file '%s' does not exist.\n", data_file);
        free_bptree(tree);
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char *ptr = line;
        while (*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n') {
            ptr++;
        }
        if (*ptr == '\0') continue;

        key_t key;
        if (sscanf(ptr, "%lld", &key) == 1) {
            delete_single_data(tree, key);
        }
    }

    fclose(fp);
    save_tree(tree, index_file);
    free_bptree(tree);
}
