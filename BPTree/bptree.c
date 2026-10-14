#include "bptree.h"

/* 정적 헬퍼 함수 선언 */
static void _leaf_split_node(BPlusTree *tree, Node *leaf, PathStack *path);
static void _parent_insert_node(BPlusTree *tree, node_id_t left_id, key_t key, node_id_t right_id, PathStack *path);
static void _balance_leaf_node(BPlusTree *tree, Node *node, PathStack *path);
static void _balance_non_leaf_node(BPlusTree *tree, Node *node, PathStack *path);

void search_single(BPlusTree *tree, key_t search_key) {
    Node *current_node = tree->nodes[tree->root_id];

    /* root에서 leaf까지 traverse */
    while (!current_node->is_leaf) {
        /* 거쳐가는 모든 non-leaf node의 keys를 콤마로 연결하여 출력 */
        for (int i=0; i < current_node->num_keys; i++) {
            printf("%lld%s", current_node->keys[i], (i == current_node->num_keys - 1) ? "" : ",");
        }
        printf("\n");

        int childIdx = -1;
        for (int i=0; i < current_node->num_keys; i++) {
            if (search_key < current_node->keys[i]) {
                childIdx = i;
                break;
            }
        }
        if (childIdx == -1) {
            childIdx = current_node->num_keys;
        }
        current_node = tree->nodes[current_node->children[childIdx]];
    }

    /* leaf node 도착 */
    bool isFound = false;
    for (int i=0; i < current_node->num_keys; i++) {
        if (current_node->keys[i] == search_key) {
            printf("%lld\n", current_node->values[i]);
            isFound = true;
            break;
        }
    }

    if (!isFound) {
        printf("NOT FOUND\n");
    }
}

void search_ranged(BPlusTree *tree, key_t start_key, key_t end_key) {
    Node *current_node = tree->nodes[tree->root_id];

    /* start_key가 들어갈 leaf node 찾기 */
    while (!current_node->is_leaf) {
        int childIdx = -1;
        for (int i = 0; i < current_node->num_keys; i++) {
            if (start_key < current_node->keys[i]) {
                childIdx = i;
                break;
            }
        }
        if (childIdx == -1) {
            childIdx = current_node->num_keys;
        }
        current_node = tree->nodes[current_node->children[childIdx]];
    }

    bool is_stop = false;
    while (current_node != NULL && !is_stop) {
        for (int i=0; i < current_node->num_keys; i++) {
            if (current_node->keys[i] >= start_key && current_node->keys[i] <= end_key) {
                printf("%lld,%lld\n", current_node->keys[i], current_node->values[i]);
            } else if (current_node->keys[i] > end_key) {
                is_stop = true;
                break;
            }
        }

        if (!is_stop && current_node->next_node_id != INVALID_NODE_ID) {
            current_node = tree->nodes[current_node->next_node_id];
        } else {
            break;
        }
    }
}

void insert_single_data(BPlusTree *tree, key_t key, val_t value) {
    PathStack path;
    path_init(&path);
    Node *current_node = tree->nodes[tree->root_id];

    /* leaf node까지 경로 추적하며 탐색 */
    while (!current_node->is_leaf) {
        path_push(&path, current_node->node_id);
        int childIdx = 0;
        while (childIdx < current_node->num_keys && key >= current_node->keys[childIdx]) {
            childIdx++;
        }
        current_node = tree->nodes[current_node->children[childIdx]];
    }

    Node *leaf_node = current_node;

    /* 중복 키 삽입 방지 */
    if (find_key_index(leaf_node, key) != -1) {
        return;
    }

    /* leaf node에 오름차순 삽입 */
    int insertIdx = 0;
    while (insertIdx < leaf_node->num_keys && key > leaf_node->keys[insertIdx]) {
        insertIdx++;
    }

    int old_num = leaf_node->num_keys;
    insert_key_at(leaf_node->keys, &leaf_node->num_keys, insertIdx, key);
    insert_val_at(leaf_node->values, &old_num, insertIdx, value);

    /* overflow 발생 시 분할 처리 */
    if (leaf_node->num_keys == tree->b) {
        _leaf_split_node(tree, leaf_node, &path);
    }
}

static void _leaf_split_node(BPlusTree *tree, Node *leaf, PathStack *path) {
    Node *new_leaf = create_leaf_node(tree);
    int split_index = (int)ceil((double)tree->b / 2.0);

    int move_count = leaf->num_keys - split_index;
    for (int i=0; i < move_count; i++) {
        new_leaf->keys[i] = leaf->keys[split_index + i];
        new_leaf->values[i] = leaf->values[split_index + i];
    }
    new_leaf->num_keys = move_count;
    leaf->num_keys = split_index;

    /* 리프 노드 간 포인터 연결 */
    new_leaf->next_node_id = leaf->next_node_id;
    leaf->next_node_id = new_leaf->node_id;

    /* 새로운 leaf node의 첫 번째 key를 부모로 복사하여 올림 */
    key_t new_parent_key = new_leaf->keys[0];
    _parent_insert_node(tree, leaf->node_id, new_parent_key, new_leaf->node_id, path);
}

static void _parent_insert_node(BPlusTree *tree, node_id_t left_id, key_t key, node_id_t right_id, PathStack *path) {
    /* path가 비어있다면 루트 노드가 분할된 경우 */
    if (path_is_empty(path)) {
        Node *new_root = create_non_leaf_node(tree);
        new_root->keys[0] = key;
        new_root->num_keys = 1;
        new_root->children[0] = left_id;
        new_root->children[1] = right_id;
        tree->root_id = new_root->node_id;
        return;
    }

    node_id_t parent_id = path_pop(path);
    Node *parent = tree->nodes[parent_id];

    int insertIdx = 0;
    while (insertIdx < parent->num_keys && key > parent->keys[insertIdx]) {
        insertIdx++;
    }

    int children_count = parent->num_keys + 1;
    insert_key_at(parent->keys, &parent->num_keys, insertIdx, key);
    insert_child_at(parent->children, &children_count, insertIdx + 1, right_id);

    /* 부모 노드 overflow 발생 시 재귀적 분할 */
    if (parent->num_keys == tree->b) {
        Node *new_parent = create_non_leaf_node(tree);
        int split_index = (int)ceil((double)tree->b / 2.0) - 1; /* 부모로 올릴 key 인덱스 */
        key_t up_key = parent->keys[split_index];

        int move_keys = parent->num_keys - (split_index + 1);
        for (int i=0; i < move_keys; i++) {
            new_parent->keys[i] = parent->keys[split_index + 1 + i];
        }
        new_parent->num_keys = move_keys;

        int move_children = (parent->num_keys + 1) - (split_index + 1);
        for (int i=0; i < move_children; i++) {
            new_parent->children[i] = parent->children[split_index + 1 + i];
        }

        parent->num_keys = split_index;

        _parent_insert_node(tree, parent->node_id, up_key, new_parent->node_id, path);
    }
}

void delete_single_data(BPlusTree *tree, key_t key) {
    PathStack path;
    path_init(&path);
    Node *current_node = tree->nodes[tree->root_id];

    while (!current_node->is_leaf) {
        path_push(&path, current_node->node_id);
        int childIdx = 0;
        while (childIdx < current_node->num_keys && key >= current_node->keys[childIdx]) {
            childIdx++;
        }
        current_node = tree->nodes[current_node->children[childIdx]];
    }

    Node *leaf_node = current_node;
    int delete_idx = find_key_index(leaf_node, key);
    if (delete_idx == -1) {
        return; /* key가 존재하지 않음 */
    }

    int val_count = leaf_node->num_keys;
    remove_key_at(leaf_node->keys, &leaf_node->num_keys, delete_idx);
    remove_val_at(leaf_node->values, &val_count, delete_idx);

    /* 루트이면서 leaf인 경우는 underflow 검사 불필요 */
    if (leaf_node->node_id == tree->root_id) {
        return;
    }

    int min_leaf_key = (int)ceil((double)(tree->b - 1) / 2.0);
    if (leaf_node->num_keys < min_leaf_key) {
        _balance_leaf_node(tree, leaf_node, &path);
    }
}

static void _balance_leaf_node(BPlusTree *tree, Node *node, PathStack *path) {
    node_id_t parent_id = path_peek(path);
    Node *parent = tree->nodes[parent_id];
    int node_position = find_child_index(parent, node->node_id);

    Node *left_sibling = (node_position > 0) ? tree->nodes[parent->children[node_position - 1]] : NULL;
    Node *right_sibling = (node_position < parent->num_keys) ? tree->nodes[parent->children[node_position + 1]] : NULL;

    int min_leaf_key = (int)ceil((double)(tree->b - 1) / 2.0);

    /* 1. 왼쪽 sibling에서 borrow 가능한 경우 */
    if (left_sibling && left_sibling->num_keys > min_leaf_key) {
        key_t b_key = left_sibling->keys[left_sibling->num_keys - 1];
        val_t b_val = left_sibling->values[left_sibling->num_keys - 1];
        left_sibling->num_keys--;

        int v_len = node->num_keys;
        insert_key_at(node->keys, &node->num_keys, 0, b_key);
        insert_val_at(node->values, &v_len, 0, b_val);

        parent->keys[node_position - 1] = node->keys[0];
        return;
    }
    /* 2. 오른쪽 sibling에서 borrow 가능한 경우 */
    else if (right_sibling && right_sibling->num_keys > min_leaf_key) {
        key_t b_key = right_sibling->keys[0];
        val_t b_val = right_sibling->values[0];
        int r_v_len = right_sibling->num_keys;
        remove_key_at(right_sibling->keys, &right_sibling->num_keys, 0);
        remove_val_at(right_sibling->values, &r_v_len, 0);

        node->keys[node->num_keys] = b_key;
        node->values[node->num_keys] = b_val;
        node->num_keys++;

        parent->keys[node_position] = right_sibling->keys[0];
        return;
    }
    /* 3. 둘 다 불가능하면 병합 (Merge) */
    else {
        if (left_sibling) {
            /* 왼쪽 노드로 병합 */
            for (int i=0; i < node->num_keys; i++) {
                left_sibling->keys[left_sibling->num_keys + i] = node->keys[i];
                left_sibling->values[left_sibling->num_keys + i] = node->values[i];
            }
            left_sibling->num_keys += node->num_keys;
            left_sibling->next_node_id = node->next_node_id;

            int p_child_count = parent->num_keys + 1;
            remove_child_at(parent->children, &p_child_count, node_position);
            remove_key_at(parent->keys, &parent->num_keys, node_position - 1);

            remove_node(tree, node->node_id);
        } else {
            /* 오른쪽 노드를 현재 노드로 병합 */
            for (int i=0; i < right_sibling->num_keys; i++) {
                node->keys[node->num_keys + i] = right_sibling->keys[i];
                node->values[node->num_keys + i] = right_sibling->values[i];
            }
            node->num_keys += right_sibling->num_keys;
            node->next_node_id = right_sibling->next_node_id;

            int p_child_count = parent->num_keys + 1;
            remove_child_at(parent->children, &p_child_count, node_position + 1);
            remove_key_at(parent->keys, &parent->num_keys, node_position);

            remove_node(tree, right_sibling->node_id);
        }

        path_pop(path);
        _balance_non_leaf_node(tree, parent, path);
    }
}

static void _balance_non_leaf_node(BPlusTree *tree, Node *node, PathStack *path) {
    /* 루트 처리 (루트 노드의 키가 0개가 되면 유일한 자식이 새로운 루트가 됨) */
    if (node->node_id == tree->root_id) {
        if (node->num_keys == 0) {
            tree->root_id = node->children[0];
            remove_node(tree, node->node_id);
        }
        return;
    }

    int min_non_leaf_key = (int)ceil((double)tree->b / 2.0) - 1;
    if (node->num_keys >= min_non_leaf_key) {
        return; /* underflow 아님 */
    }

    node_id_t parent_id = path_peek(path);
    Node *parent = tree->nodes[parent_id];
    int node_position = find_child_index(parent, node->node_id);

    Node *left_sibling = (node_position > 0) ? tree->nodes[parent->children[node_position - 1]] : NULL;
    Node *right_sibling = (node_position < parent->num_keys) ? tree->nodes[parent->children[node_position + 1]] : NULL;

    /* 1. 왼쪽 sibling에서 borrow */
    if (left_sibling && left_sibling->num_keys > min_non_leaf_key) {
        insert_key_at(node->keys, &node->num_keys, 0, parent->keys[node_position - 1]);
        
        node_id_t b_child = left_sibling->children[left_sibling->num_keys];
        int node_child_count = node->num_keys;
        insert_child_at(node->children, &node_child_count, 0, b_child);

        parent->keys[node_position - 1] = left_sibling->keys[left_sibling->num_keys - 1];
        left_sibling->num_keys--;
        return;
    }
    /* 2. 오른쪽 sibling에서 borrow */
    else if (right_sibling && right_sibling->num_keys > min_non_leaf_key) {
        node->keys[node->num_keys] = parent->keys[node_position];
        node->children[node->num_keys + 1] = right_sibling->children[0];
        node->num_keys++;

        parent->keys[node_position] = right_sibling->keys[0];

        int r_child_count = right_sibling->num_keys + 1;
        remove_key_at(right_sibling->keys, &right_sibling->num_keys, 0);
        remove_child_at(right_sibling->children, &r_child_count, 0);
        return;
    }
    /* 3. 병합 (Merge) */
    else {
        if (left_sibling) {
            /* 부모 키를 끼워넣고 왼쪽 sibling으로 합치기 */
            left_sibling->keys[left_sibling->num_keys] = parent->keys[node_position - 1];
            left_sibling->num_keys++;

            for (int i=0; i < node->num_keys; i++) {
                left_sibling->keys[left_sibling->num_keys + i] = node->keys[i];
            }
            int left_child_count = left_sibling->num_keys;
            for (int i=0; i < node->num_keys + 1; i++) {
                left_sibling->children[left_child_count + i] = node->children[i];
            }
            left_sibling->num_keys += node->num_keys;

            int p_child_count = parent->num_keys + 1;
            remove_child_at(parent->children, &p_child_count, node_position);
            remove_key_at(parent->keys, &parent->num_keys, node_position - 1);

            remove_node(tree, node->node_id);
        } else {
            /* 오른쪽 sibling을 현재 node로 합치기 */
            node->keys[node->num_keys] = parent->keys[node_position];
            node->num_keys++;

            for (int i=0; i < right_sibling->num_keys; i++) {
                node->keys[node->num_keys + i] = right_sibling->keys[i];
            }
            int node_child_count = node->num_keys;
            for (int i=0; i < right_sibling->num_keys + 1; i++) {
                node->children[node_child_count + i] = right_sibling->children[i];
            }
            node->num_keys += right_sibling->num_keys;

            int p_child_count = parent->num_keys + 1;
            remove_child_at(parent->children, &p_child_count, node_position + 1);
            remove_key_at(parent->keys, &parent->num_keys, node_position);

            remove_node(tree, right_sibling->node_id);
        }

        path_pop(path);
        _balance_non_leaf_node(tree, parent, path);
    }
}
