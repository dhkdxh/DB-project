#ifndef BPTREE_H
#define BPTREE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define INVALID_NODE_ID -1
#define MAX_PATH_DEPTH 128

typedef long long key_t;
typedef long long val_t;
typedef int node_id_t;

typedef struct Node {
    node_id_t node_id;
    bool is_leaf;
    int num_keys;
    key_t *keys;

    /* Leaf Node */
    val_t *values;
    node_id_t next_node_id;

    /* Non-Leaf Node */
    node_id_t *children;
} Node;

/* B+ Tree 메타데이터 및 전체 노드 */
typedef struct BPlusTree {
    int b;                     //차수
    node_id_t root_id;          
    node_id_t next_node_id;     
    Node **nodes;               
    int node_capacity;          
} BPlusTree;

/* 부모 경로 추적을 위한 stack */
typedef struct PathStack {
    node_id_t items[MAX_PATH_DEPTH];
    int top;
} PathStack;

/* PathStack 연산 */
void path_init(PathStack *s);
void path_push(PathStack *s, node_id_t id);
node_id_t path_pop(PathStack *s);
node_id_t path_peek(const PathStack *s);
bool path_is_empty(const PathStack *s);

/* 트리 및 노드 관리 */
BPlusTree *create_bptree(int b);
void free_bptree(BPlusTree *tree);
Node *create_leaf_node(BPlusTree *tree);
Node *create_non_leaf_node(BPlusTree *tree);
void free_node(Node *node);
void remove_node(BPlusTree *tree, node_id_t node_id);
void ensure_node_capacity(BPlusTree *tree, int min_capacity);

/* 배열 조작 헬퍼 함수 */
void insert_key_at(key_t *arr, int *len, int idx, key_t key);
void remove_key_at(key_t *arr, int *len, int idx);
void insert_val_at(val_t *arr, int *len, int idx, val_t val);
void remove_val_at(val_t *arr, int *len, int idx);
void insert_child_at(node_id_t *arr, int *len, int idx, node_id_t child_id);
void remove_child_at(node_id_t *arr, int *len, int idx);
int find_key_index(const Node *node, key_t key);
int find_child_index(const Node *node, node_id_t child_id);

/* B+ Tree 코어 로직 */
void insert_single_data(BPlusTree *tree, key_t key, val_t value);
void delete_single_data(BPlusTree *tree, key_t key);
void search_single(BPlusTree *tree, key_t key);
void search_ranged(BPlusTree *tree, key_t start_key, key_t end_key);

/* 파일 I/O 및 기타 작업 */
void save_tree(const BPlusTree *tree, const char *filename);
BPlusTree *load_tree(const char *filename);
void create_index(const char *index_file, int b);
void insert(const char *index_file, const char *data_file);
void delete(const char *index_file, const char *data_file);

#endif 
