#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <getopt.h>

#define VERSION "0.1.0"
#define INITIAL_CAPACITY 100

typedef struct Node {
    char label; // 'P' for pair, 'U' for unpaired, 'R' for root
    struct Node** children;
    int num_children;
    int index; // Index in postorder traversal
    int i, j; // Positions in the dot-bracket string
} Node;

typedef struct TreeInfo {
    Node* root;
    Node** postorder;
    int postorder_size;
    int* leftmost;
    int* keyroots;
    int num_keyroots;
} TreeInfo;

// Function declarations
Node* parse_dot_bracket(const char* db);
void free_node(Node* node);
void postorder_traversal(Node* node, Node** list, int* index);
int find_leftmost_index(Node* node);
void collect_keyroots(Node* node, Node* parent, int is_first_child, int* keyroots, int* index);
TreeInfo* compute_tree_info(Node* root);
void free_tree_info(TreeInfo* ti);
int tree_edit_dist(TreeInfo* t1, TreeInfo* t2);
void fill_tree_edit_matrix(TreeInfo* t1, TreeInfo* t2, int** treedist);
void forest_dist(int i, int j, TreeInfo* t1, TreeInfo* t2, int** treedist);
int cost_insert(char label);
int cost_delete(char label);
int cost_relabel(char label1, char label2);
int min(int a, int b);

static int compare_int(const void *a, const void *b) {
    int ia = *(const int*)a;
    int ib = *(const int*)b;
    return ia - ib;
}

Node* parse_dot_bracket(const char* db) {
    int len = strlen(db);
    Node* root = malloc(sizeof(Node));
    if (!root) {
        fprintf(stderr, "Memory allocation failed for root node\n");
        exit(1);
    }
    root->label = 'R';
    root->children = NULL;
    root->num_children = 0;
    root->index = -1;
    root->i = 0;
    root->j = len - 1;

    int* stack = malloc(sizeof(int) * len);
    if (!stack) {
        fprintf(stderr, "Memory allocation failed for stack\n");
        exit(1);
    }
    int stack_size = 0;
    int* parent_stack = malloc(sizeof(int) * len);
    if (!parent_stack) {
        fprintf(stderr, "Memory allocation failed for parent stack\n");
        exit(1);
    }
    int parent_stack_size = 0;
    parent_stack[parent_stack_size] = -1; // root has no parent
    parent_stack_size++;

    Node** nodes = malloc(sizeof(Node*) * (len + 1));
    if (!nodes) {
        fprintf(stderr, "Memory allocation failed for nodes array\n");
        exit(1);
    }
    int node_count = 0;

    int open_count = 0, close_count = 0;
    for (int i = 0; i < len; i++) {
        char c = db[i];
        if (c == '.') {
            Node* new_node = malloc(sizeof(Node));
            if (!new_node) {
                fprintf(stderr, "Memory allocation failed for node\n");
                exit(1);
            }
            new_node->label = 'U';
            new_node->children = NULL;
            new_node->num_children = 0;
            new_node->index = -1;
            new_node->i = i;
            new_node->j = i;
            nodes[node_count] = new_node;
            node_count++;
            int parent_idx = parent_stack[parent_stack_size - 1];
            if (parent_idx == -1) {
                root->children = realloc(root->children, sizeof(Node*) * (root->num_children + 1));
                root->children[root->num_children] = new_node;
                root->num_children++;
            } else {
                Node* parent = nodes[parent_idx];
                parent->children = realloc(parent->children, sizeof(Node*) * (parent->num_children + 1));
                parent->children[parent->num_children] = new_node;
                parent->num_children++;
            }
        } else if (c == '(') {
            open_count++;
            Node* new_node = malloc(sizeof(Node));
            if (!new_node) {
                fprintf(stderr, "Memory allocation failed for node\n");
                exit(1);
            }
            new_node->label = 'P';
            new_node->children = NULL;
            new_node->num_children = 0;
            new_node->index = -1;
            new_node->i = i;
            new_node->j = -1;
            nodes[node_count] = new_node;
            int parent_idx = parent_stack[parent_stack_size - 1];
            if (parent_idx == -1) {
                root->children = realloc(root->children, sizeof(Node*) * (root->num_children + 1));
                root->children[root->num_children] = new_node;
                root->num_children++;
            } else {
                Node* parent = nodes[parent_idx];
                parent->children = realloc(parent->children, sizeof(Node*) * (parent->num_children + 1));
                parent->children[parent->num_children] = new_node;
                parent->num_children++;
            }
            stack[stack_size] = node_count;
            stack_size++;
            parent_stack[parent_stack_size] = node_count;
            parent_stack_size++;
            node_count++;
        } else if (c == ')') {
            close_count++;
            if (stack_size == 0) {
                fprintf(stderr, "Unmatched closing parenthesis in %s at position %d\n", db, i);
                exit(1);
            }
            stack_size--;
            int node_idx = stack[stack_size];
            nodes[node_idx]->j = i;
            parent_stack_size--;
        } else {
            fprintf(stderr, "Invalid character '%c' in %s at position %d\n", c, db, i);
            exit(1);
        }
    }
    if (open_count != close_count) {
        fprintf(stderr, "Unbalanced parentheses in %s\n", db);
        exit(1);
    }
    if (stack_size != 0) {
        fprintf(stderr, "Unclosed parentheses in %s\n", db);
        exit(1);
    }
    free(stack);
    free(parent_stack);
    free(nodes);
    return root;
}

void free_node(Node* node) {
    if (!node) return;
    for (int i = 0; i < node->num_children; i++) {
        free_node(node->children[i]);
    }
    free(node->children);
    free(node);
}

void postorder_traversal(Node* node, Node** list, int* index) {
    if (!node) return;
    for (int i = 0; i < node->num_children; i++) {
        postorder_traversal(node->children[i], list, index);
    }
    list[*index] = node;
    node->index = *index;
    (*index)++;
}

int find_leftmost_index(Node* node) {
    if (!node) return -1;
    if (node->num_children == 0) {
        return node->index;
    }
    return find_leftmost_index(node->children[0]);
}

void collect_keyroots(Node* node, Node* parent, int is_first_child, int* keyroots, int* index) {
    if (!node) return;
    if (parent == NULL || !is_first_child) {
        keyroots[*index] = node->index;
        (*index)++;
    }
    for (int i = 0; i < node->num_children; i++) {
        collect_keyroots(node->children[i], node, i == 0, keyroots, index);
    }
}

TreeInfo* compute_tree_info(Node* root) {
    TreeInfo* ti = malloc(sizeof(TreeInfo));
    if (!ti) {
        fprintf(stderr, "Memory allocation failed for TreeInfo\n");
        exit(1);
    }
    ti->root = root;
    int max_nodes = 1000; // Adjust based on input size
    ti->postorder = malloc(sizeof(Node*) * max_nodes);
    if (!ti->postorder) {
        fprintf(stderr, "Memory allocation failed for postorder\n");
        exit(1);
    }
    int index = 0;
    postorder_traversal(root, ti->postorder, &index);
    ti->postorder_size = index;
    ti->postorder = realloc(ti->postorder, sizeof(Node*) * ti->postorder_size);
    if (!ti->postorder && ti->postorder_size > 0) {
        fprintf(stderr, "Memory reallocation failed for postorder\n");
        exit(1);
    }
    ti->leftmost = malloc(sizeof(int) * ti->postorder_size);
    if (!ti->leftmost) {
        fprintf(stderr, "Memory allocation failed for leftmost\n");
        exit(1);
    }
    for (int i = 0; i < ti->postorder_size; i++) {
        ti->leftmost[i] = find_leftmost_index(ti->postorder[i]);
    }
    ti->keyroots = malloc(sizeof(int) * ti->postorder_size);
    if (!ti->keyroots) {
        fprintf(stderr, "Memory allocation failed for keyroots\n");
        exit(1);
    }
    int kr_index = 0;
    collect_keyroots(root, NULL, 0, ti->keyroots, &kr_index);
    ti->num_keyroots = kr_index;
    ti->keyroots = realloc(ti->keyroots, sizeof(int) * ti->num_keyroots);
    if (!ti->keyroots && ti->num_keyroots > 0) {
        fprintf(stderr, "Memory reallocation failed for keyroots\n");
        exit(1);
    }
    qsort(ti->keyroots, ti->num_keyroots, sizeof(int), compare_int);
    return ti;
}

void free_tree_info(TreeInfo* ti) {
    if (!ti) return;
    free(ti->postorder);
    free(ti->leftmost);
    free(ti->keyroots);
    free(ti);
}

int cost_insert(char label) {
    return (label == 'P') ? 2 : 1;
}

int cost_delete(char label) {
    return (label == 'P') ? 2 : 1;
}

int cost_relabel(char label1, char label2) {
    return (label1 == label2) ? 0 : 1;
}

int min(int a, int b) {
    return a < b ? a : b;
}

void forest_dist(int i, int j, TreeInfo* t1, TreeInfo* t2, int** treedist) {
    int l1 = t1->leftmost[i];
    int l2 = t2->leftmost[j];
    int base_d1 = i - l1 + 2;
    int base_d2 = j - l2 + 2;
    int** fd = malloc(sizeof(int*) * base_d1);
    if (!fd) {
        fprintf(stderr, "Memory allocation failed for forest distance array\n");
        exit(1);
    }
    for (int di = 0; di < base_d1; di++) {
        fd[di] = malloc(sizeof(int) * base_d2);
        if (!fd[di]) {
            fprintf(stderr, "Memory allocation failed for forest distance row\n");
            exit(1);
        }
        for (int dj = 0; dj < base_d2; dj++) {
            fd[di][dj] = base_d1 + base_d2;
        }
    }
    fd[0][0] = 0;
    for (int di = 1; di < base_d1; di++) {
        int idx1 = l1 + di - 1;
        char label1 = t1->postorder[idx1]->label;
        fd[di][0] = fd[di - 1][0] + cost_delete(label1);
    }
    for (int dj = 1; dj < base_d2; dj++) {
        int idx2 = l2 + dj - 1;
        char label2 = t2->postorder[idx2]->label;
        fd[0][dj] = fd[0][dj - 1] + cost_insert(label2);
    }
    for (int di = 1; di < base_d1; di++) {
        for (int dj = 1; dj < base_d2; dj++) {
            int idx1 = l1 + di - 1;
            int idx2 = l2 + dj - 1;
            char label1 = t1->postorder[idx1]->label;
            char label2 = t2->postorder[idx2]->label;
            int cost;
            if (t1->leftmost[idx1] == l1 && t2->leftmost[idx2] == l2) {
                int delete_cost = fd[di - 1][dj] + cost_delete(label1);
                int insert_cost = fd[di][dj - 1] + cost_insert(label2);
                int relabel_cost = fd[di - 1][dj - 1] + cost_relabel(label1, label2);
                cost = min(delete_cost, min(insert_cost, relabel_cost));
                treedist[idx1][idx2] = cost;
            } else {
                int delete_cost = fd[di - 1][dj] + cost_delete(label1);
                int insert_cost = fd[di][dj - 1] + cost_insert(label2);
                int subtree_cost = fd[t1->leftmost[idx1] - l1][t2->leftmost[idx2] - l2] + treedist[idx1][idx2];
                cost = min(delete_cost, min(insert_cost, subtree_cost));
            }
            fd[di][dj] = cost;
        }
    }
    for (int di = 0; di < base_d1; di++) {
        free(fd[di]);
    }
    free(fd);
}

void fill_tree_edit_matrix(TreeInfo* t1, TreeInfo* t2, int** treedist) {
    for (int ki = 0; ki < t1->num_keyroots; ki++) {
        int i = t1->keyroots[ki];
        for (int kj = 0; kj < t2->num_keyroots; kj++) {
            int j = t2->keyroots[kj];
            forest_dist(i, j, t1, t2, treedist);
        }
    }
}

int tree_edit_dist(TreeInfo* t1, TreeInfo* t2) {
    int m = t1->postorder_size;
    int n = t2->postorder_size;
    int** treedist = malloc(sizeof(int*) * m);
    if (!treedist) {
        fprintf(stderr, "Memory allocation failed for tree edit distance matrix\n");
        exit(1);
    }
    for (int i = 0; i < m; i++) {
        treedist[i] = malloc(sizeof(int) * n);
        if (!treedist[i]) {
            fprintf(stderr, "Memory allocation failed for tree edit distance row\n");
            exit(1);
        }
        for (int j = 0; j < n; j++) {
            treedist[i][j] = m + n;
        }
    }
    fill_tree_edit_matrix(t1, t2, treedist);
    int dist = treedist[m-1][n-1];
    for (int i = 0; i < m; i++) {
        free(treedist[i]);
    }
    free(treedist);
    return dist;
}

int main(int argc, char* argv[]) {
    int opt;
    int num_threads = omp_get_max_threads();
    int row_wise = 0;  // Default to full matrix mode
    int first_only = 0; // New option to compare only the first structure

    struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {"threads", required_argument, 0, 't'},
        {"row-wise", no_argument, 0, 'r'},
        {"first-only", no_argument, 0, 'f'}, // New option
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "hvt:rf", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h':
                printf("Usage: %s [options]\n", argv[0]);
                printf("Options:\n");
                printf("  --help, -h         Display this help message\n");
                printf("  --version, -v      Display version information\n");
                printf("  --threads, -t      Set number of threads (default: %d)\n", num_threads);
                printf("  --row-wise, -r     Output the distance matrix row by row (memory-efficient)\n");
                printf("  --first-only, -f   Compute distances only for the first structure against all others\n");
                printf("\nThe program reads RNA secondary structures in dot-bracket notation\n");
                printf("from standard input, one per line, and outputs either a distance matrix\n");
                printf("or distances for the first structure based on tree edit distance.\n");
                return 0;
            case 'v':
                printf("Version: %s\n", VERSION);
                return 0;
            case 't':
                num_threads = atoi(optarg);
                if (num_threads <= 0) {
                    fprintf(stderr, "Invalid number of threads: %s\n", optarg);
                    return 1;
                }
                break;
            case 'r':
                row_wise = 1;
                break;
            case 'f':
                first_only = 1;
                break;
            default:
                fprintf(stderr, "Invalid option. Use --help for usage information.\n");
                return 1;
        }
    }

    omp_set_num_threads(num_threads);

    char** structures = malloc(INITIAL_CAPACITY * sizeof(char*));
    if (!structures) {
        fprintf(stderr, "Memory allocation failed for structures\n");
        return 1;
    }
    int capacity = INITIAL_CAPACITY;
    int num_structures = 0;
    char line[1024];

    while (fgets(line, sizeof(line), stdin) != NULL) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) break;
        if (num_structures >= capacity) {
            capacity *= 2;
            structures = realloc(structures, capacity * sizeof(char*));
            if (!structures) {
                fprintf(stderr, "Memory reallocation failed for structures\n");
                return 1;
            }
        }
        structures[num_structures] = strdup(line);
        if (!structures[num_structures]) {
            fprintf(stderr, "Memory allocation failed for structure string\n");
            return 1;
        }
        num_structures++;
    }

    if (num_structures == 0) {
        fprintf(stderr, "No structures provided.\n");
        free(structures);
        return 1;
    }

    structures = realloc(structures, num_structures * sizeof(char*));
    if (!structures && num_structures > 0) {
        fprintf(stderr, "Memory reallocation failed for structures\n");
        return 1;
    }

    Node** trees = malloc(num_structures * sizeof(Node*));
    if (!trees) {
        fprintf(stderr, "Memory allocation failed for trees\n");
        return 1;
    }
    for (int i = 0; i < num_structures; i++) {
        trees[i] = parse_dot_bracket(structures[i]);
    }

    TreeInfo** ti_array = malloc(num_structures * sizeof(TreeInfo*));
    if (!ti_array) {
        fprintf(stderr, "Memory allocation failed for tree info array\n");
        return 1;
    }
    for (int i = 0; i < num_structures; i++) {
        ti_array[i] = compute_tree_info(trees[i]);
    }

    if (first_only) {
        // Compute distances only for the first structure against all others
        if (num_structures < 2) {
            fprintf(stderr, "At least two structures are required for comparison.\n");
            for (int i = 0; i < num_structures; i++) {
                free(structures[i]);
                free_node(trees[i]);
                free_tree_info(ti_array[i]);
            }
            free(structures);
            free(trees);
            free(ti_array);
            return 1;
        }

        int* distances = malloc((num_structures - 1) * sizeof(int));
        if (!distances) {
            fprintf(stderr, "Memory allocation failed for distances array\n");
            for (int i = 0; i < num_structures; i++) {
                free(structures[i]);
                free_node(trees[i]);
                free_tree_info(ti_array[i]);
            }
            free(structures);
            free(trees);
            free(ti_array);
            return 1;
        }

        #pragma omp parallel for schedule(dynamic)
        for (int j = 1; j < num_structures; j++) {
            int ted = tree_edit_dist(ti_array[0], ti_array[j]);
            distances[j - 1] = ted;
        }

        // Output distances one below the other
        for (int j = 0; j < num_structures - 1; j++) {
            printf("%d\n", distances[j]);
        }

        free(distances);
    } else if (row_wise) {
        // Row-wise computation and output
        _Atomic int completed_rows = 0;
        int last_percentage = -1;

        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < num_structures; i++) {
            int* row = malloc(num_structures * sizeof(int));
            if (!row) {
                fprintf(stderr, "Memory allocation failed for row\n");
                exit(1);
            }
            row[i] = 0; // Distance to itself is 0
            for (int j = 0; j < num_structures; j++) {
                if (j != i) {
                    int ted = tree_edit_dist(ti_array[i], ti_array[j]);
                    row[j] = ted;
                }
            }
            #pragma omp critical
            {
                for (int j = 0; j < num_structures; j++) {
                    printf("%d ", row[j]);
                }
                printf("\n");
                fflush(stdout);
            }
            free(row);
            __atomic_fetch_add(&completed_rows, 1, __ATOMIC_SEQ_CST);
            int percentage = (completed_rows * 100) / num_structures;
            #pragma omp critical
            {
                if (percentage > last_percentage) {
                    fprintf(stderr, "\rProgress: %d%%", percentage);
                    fflush(stderr);
                    last_percentage = percentage;
                }
            }
        }
        fprintf(stderr, "\n");
    } else {
        // Full matrix computation
        _Atomic int completed_rows = 0;
        int last_percentage = -1;

        int* distance_matrix = malloc(num_structures * num_structures * sizeof(int));
        if (!distance_matrix) {
            fprintf(stderr, "Memory allocation failed for distance matrix\n");
            return 1;
        }
        for (int i = 0; i < num_structures; i++) {
            distance_matrix[i * num_structures + i] = 0;
        }

        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < num_structures; i++) {
            for (int j = i + 1; j < num_structures; j++) {
                int ted = tree_edit_dist(ti_array[i], ti_array[j]);
                distance_matrix[i * num_structures + j] = ted;
                distance_matrix[j * num_structures + i] = ted;
            }
            __atomic_fetch_add(&completed_rows, 1, __ATOMIC_SEQ_CST);
            int percentage = (completed_rows * 100) / num_structures;
            #pragma omp critical
            {
                if (percentage > last_percentage) {
                    fprintf(stderr, "\rProgress: %d%%", percentage);
                    fflush(stderr);
                    last_percentage = percentage;
                }
            }
        }

        // Output the full matrix
        for (int i = 0; i < num_structures; i++) {
            for (int j = 0; j < num_structures; j++) {
                printf("%d ", distance_matrix[i * num_structures + j]);
            }
            printf("\n");
        }
        fprintf(stderr, "\n");
        free(distance_matrix);
    }

    // Clean up memory
    for (int i = 0; i < num_structures; i++) {
        free(structures[i]);
        free_node(trees[i]);
        free_tree_info(ti_array[i]);
    }
    free(structures);
    free(trees);
    free(ti_array);

    return 0;
}