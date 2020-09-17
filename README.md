# B*-tree

Knuth defines a **B*-tree** to be a B-tree variant in which each node is at least 2/3 full (instead of just 1/2 full).

## Properties

A **B*-tree** of order *m* is a tree that satisfies the following properties:

1. Every node except the root has at most *m* children.
2. Every node, except for the root and the leaves, has at least$(2m-1)/3$ children.
3. The root has at least 2 and at most $2⌊(2m-2)/3⌋ + 1$ children.
4. All leaves appear on the same level.
5. A non-leaf node with *k* - 1 keys.
