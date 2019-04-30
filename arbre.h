typedef struct Node
{
    struct Node *parent;
    struct Node *left;
    struct Node *right;
    bool is_node;
    bool bit;
    size_t occurence;
    unsigned char value;
} Node;

typedef struct Tree
{
    struct Node *root;
    struct Node **leafs;
    struct Node **node_search;
    unsigned int next_node;
} Tree;
