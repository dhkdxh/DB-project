# B+ Tree Implementation Project

B+ Tree implementation based on tree degree ($b$).


## Features & Architecture
- **Separation of Index & Data**: Non-leaf nodes only store search keys and child pointers, while all actual key-value records are stored exclusively in leaf nodes.
- **Linked Leaf Nodes**: All leaf nodes are linked sequentially via `next_node_id`, enabling efficient range queries.
- **Node ID-Based Persistence**: Uses unique `node_id` identifiers instead of memory pointers to serialize and deserialize the tree to/from a binary index file (`index.dat`).



## Core Operations

### 1. Search
- **Single Search (`-s`)**: Traverses down from the root, prints the search keys of each visited non-leaf node, and retrieves the value upon reaching the leaf node.
- **Range Search (`-r`)**: Finds the leaf node containing the start key, then traverses through the leaf linked list to output key-value pairs within the range.

### 2. Insertion & Split
Inserts data into the leaf node in sorted order. When the number of keys reaches degree $b$ (overflow), the node splits:
- **Leaf Split**: Splits at index $\lceil b / 2 \rceil$, and **copies up** the first key of the new leaf node to the parent.
- **Non-Leaf Split**: Splits at index $\lceil b / 2 \rceil - 1$. The median key is **moved up** to the parent, propagating recursively up to the root if needed.

### 3. Deletion & Rebalance
Deletes the key from the leaf node. When the number of keys falls below the threshold (underflow), rebalancing is performed:
- **Minimum Key Requirements**: $\lceil (b - 1) / 2 \rceil$ for leaf nodes, and $\lceil b / 2 \rceil - 1$ for non-leaf nodes.
- **Borrow (Redistribution)**: Borrows a key from an adjacent sibling (left sibling prioritized) if it has spare keys, then updates the parent separator key.
- **Merge**: Merges with an adjacent sibling (left sibling prioritized) if borrowing is not possible. Underflow in the parent node is handled recursively.
