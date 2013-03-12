#ifndef tree_HEADER
#define tree_HEADER

#include <stdlib.h>

#include "object.h"

#define TREE_NEUTRAL 0
#define TREE_LEFT    1
#define TREE_RIGHT   2

struct _tree_it {
    struct _tree_it *   parent;
    int                 direction;
    struct _tree_node * node;
};

struct _tree_node {
    unsigned int level;
    void * data;
    struct _tree_node * left;
    struct _tree_node * right;
};

struct _tree {
    const struct _object * object;
    struct _tree_node * nodes;
};


struct _tree * tree_create      ();
void           tree_delete      (struct _tree * tree);
struct _tree * tree_copy        (const struct _tree * tree);

void           tree_map (struct _tree * tree, void (* callback) (void *));

void           tree_remove      (struct _tree * tree, const void * data);
void           tree_insert      (struct _tree * tree, const const void * data);
void *         tree_fetch       (const struct _tree * tree, const void * data);
void *         tree_fetch_max   (const struct _tree * tree, const void * data);

struct _tree_node * tree_node_create  (const void * data);
void                tree_node_map     (struct _tree_node * node,
                                       void (* callback) (void *));

struct _tree_node * tree_node_insert  (struct _tree * tree, 
                                       struct _tree_node * node,
                                       struct _tree_node * new_node);

struct _tree_node * tree_node_fetch   (const struct _tree * tree,
                                       struct _tree_node * node,
                                       const void * data);

struct _tree_node * tree_node_fetch_max (const struct _tree * tree,
                                         struct _tree_node * node,
                                         const void * data);

// deletes a node from a tree
struct _tree_node * tree_node_delete  (struct _tree * tree,
                                       struct _tree_node * node,
                                       const void * data);

struct _tree_node * tree_node_skew    (struct _tree_node * node);
struct _tree_node * tree_node_split   (struct _tree_node * node);

struct _tree_it * tree_iterator   (const struct _tree * tree);
struct _tree_it * tree_it_next    (struct _tree_it * tree_it);
void *            tree_it_data    (const struct _tree_it * tree_it);
void              tree_it_delete  (struct _tree_it * tree_it);

#endif