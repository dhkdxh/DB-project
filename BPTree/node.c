#include "bptree.h"

/* PathStack 연산 */
void path_init(PathStack *s) {
    s->top = 0;
}

void path_push(PathStack *s, node_id_t id) {
    if (s->top < MAX_PATH_DEPTH) {
        s->items[s->top++] = id;
    }
}

node_id_t path_pop(PathStack *s) {
    if (s->top > 0) {
        return s->items[--s->top];
    }
    return INVALID_NODE_ID;
}

node_id_t path_peek(const PathStack *s) {
    if (s->top > 0) {
        return s->items[s->top - 1];
    }
    return INVALID_NODE_ID;
}

bool path_is_empty(const PathStack *s) {
    return s->top == 0;
}

/* 동적 노드 풀 용량 확장 */
void ensure_node_capacity(BPlusTree *tree, int min_capacity) {
    if (min_capacity <= tree->node_capacity) {
        return;
    }
    int new_cap = tree->node_capacity == 0 ? 64 : tree->node_capacity;
    while (new_cap < min_capacity) {
        new_cap *= 2;
    }
    Node **new_nodes = realloc(tree->nodes, sizeof(Node *) * new_cap);
    if (!new_nodes) {
        fprintf(stderr, "Error: Memory allocation failed for tree nodes\n");
        exit(EXIT_FAILURE);
    }
    for (int i = tree->node_capacity; i < new_cap; i++) {
        new_nodes[i] = NULL;
    }
    tree->nodes = new_nodes;
    tree->node_capacity = new_cap;
}

/* 초기화 작업 */
BPlusTree *create_bptree(int b) {
    BPlusTree *tree = (BPlusTree *)malloc(sizeof(BPlusTree));
    if (!tree) {
        fprintf(stderr, "Error: Memory allocation failed for BPlusTree\n");
        exit(EXIT_FAILURE);
    }
    tree->b = b;
    tree->next_node_id = 0;
    tree->node_capacity = 0;
    tree->nodes = NULL;

    ensure_node_capacity(tree, 64);

    Node *root = create_leaf_node(tree);
    tree->root_id = root->node_id;
    return tree;
}

/* 리프 노드 생성 */
Node *create_leaf_node(BPlusTree *tree) {
    node_id_t nid = tree->next_node_id++;
    ensure_node_capacity(tree, nid + 1);

    Node *node = (Node *)malloc(sizeof(Node));
    if (!node) {
        fprintf(stderr, "Error: Memory allocation failed for LeafNode\n");
        exit(EXIT_FAILURE);
    }
    node->node_id = nid;
    node->is_leaf = true;
    node->num_keys = 0;
    node->keys = (key_t *)calloc(tree->b + 2, sizeof(key_t));
    node->values = (val_t *)calloc(tree->b + 2, sizeof(val_t));
    node->children = NULL;
    node->next_node_id = INVALID_NODE_ID;

    tree->nodes[nid] = node;
    return node;
}

/* 내부(Non-Leaf) 노드 생성 */
Node *create_non_leaf_node(BPlusTree *tree) {
    node_id_t nid = tree->next_node_id++;
    ensure_node_capacity(tree, nid + 1);

    Node *node = (Node *)malloc(sizeof(Node));
    if (!node) {
        fprintf(stderr, "Error: Memory allocation failed for NonLeafNode\n");
        exit(EXIT_FAILURE);
    }
    node->node_id = nid;
    node->is_leaf = false;
    node->num_keys = 0;
    node->keys = (key_t *)calloc(tree->b + 2, sizeof(key_t));
    node->values = NULL;
    node->children = (node_id_t *)calloc(tree->b + 3, sizeof(node_id_t));
    node->next_node_id = INVALID_NODE_ID;

    tree->nodes[nid] = node;
    return node;
}

/* 노드 해제 */
void free_node(Node *node) {
    if (!node) return;
    if (node->keys) free(node->keys);
    if (node->values) free(node->values);
    if (node->children) free(node->children);
    free(node);
}

/* 트리에서 특정 노드 삭제 및 메모리 해제 */
void remove_node(BPlusTree *tree, node_id_t node_id) {
    if (node_id >= 0 && node_id < tree->next_node_id && tree->nodes[node_id]) {
        free_node(tree->nodes[node_id]);
        tree->nodes[node_id] = NULL;
    }
}

/* 전체 트리 메모리 해제 */
void free_bptree(BPlusTree *tree) {
    if (!tree) return;
    for (int i = 0; i<tree->next_node_id; i++) {
        if (tree->nodes[i]) {
            free_node(tree->nodes[i]);
        }
    }
    free(tree->nodes);
    free(tree);
}

/* 배열 조작 추가 함수 */
void insert_key_at(key_t *arr, int *len, int idx, key_t key) {
    if (idx < *len) {
        memmove(&arr[idx + 1], &arr[idx], (*len - idx) * sizeof(key_t));
    }
    arr[idx] = key;
    (*len)++;
}

void remove_key_at(key_t *arr, int *len, int idx) {
    if (idx < *len-1) {
        memmove(&arr[idx], &arr[idx + 1], (*len - idx - 1) * sizeof(key_t));
    }
    (*len)--;
}

void insert_val_at(val_t *arr, int *len, int idx, val_t val) {
    if (idx<*len) {
        memmove(&arr[idx + 1], &arr[idx], (*len - idx) * sizeof(val_t));
    }
    arr[idx] = val;
    (*len)++;
}

void remove_val_at(val_t *arr, int *len, int idx) {
    if (idx<*len - 1) {
        memmove(&arr[idx], &arr[idx + 1], (*len - idx - 1) * sizeof(val_t));
    }
    (*len)--;
}

void insert_child_at(node_id_t *arr, int *len, int idx, node_id_t child_id) {
    if (idx < *len) {
        memmove(&arr[idx + 1], &arr[idx], (*len - idx) * sizeof(node_id_t));
    }
    arr[idx] = child_id;
    (*len)++;
}

void remove_child_at(node_id_t *arr, int *len, int idx) {
    if (idx < *len - 1) {
        memmove(&arr[idx], &arr[idx + 1], (*len - idx - 1) * sizeof(node_id_t));
    }
    (*len)--;
}

int find_key_index(const Node *node, key_t key) {
    for (int i = 0; i<node->num_keys; i++) {
        if (node->keys[i] == key) {
            return i;
        }
    }
    return -1;
}

int find_child_index(const Node *node, node_id_t child_id) {
    int total_children = node->num_keys + 1;
    for (int i = 0; i<total_children; i++) {
        if (node->children[i] == child_id) {
            return i;
        }
    }
    return -1;
}
