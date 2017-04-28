#include "hash.h"

#define max(a, b) (((a) > (b))? (a) : (b))

/* allocate and initialize a node */
static struct ht_node* __ht_new_node(uint64_t key, void *value);

/* return the node whose key is key */
static struct ht_node *__ht_get_node(struct ht_node *node, uint64_t key);

/* print the (key, value) stored in a hash table */
static void __ht_print(struct ht_node *node, int depth);

/* check if a hashtable is consistent */
static void __ht_check(struct ht_node *node);


/* update the height of a node based on its children height */
static void __ht_update_height(struct ht_node *node);
/* perform a right rotation and return the new root */
static struct ht_node *__ht_right_rotate(struct ht_node *z);
/* perform a left rotation and return the new root */
static struct ht_node *__ht_left_rotate(struct ht_node *z);
/* balance a subtree. Return the new root */
static struct ht_node* __ht_balance_tree(struct ht_node *node);

void ht_print(struct ht_node *node) {
  __ht_print(node, 0);
}

void ht_check(struct ht_node *node) {
  __ht_check(node);
}



// return the height of a node
int ht_height(struct ht_node *node) {
  if (!node)
    return 0;
  return node->height;
}

/* return the node whose key is key */
static struct ht_node *__ht_get_node(struct ht_node *node, uint64_t key) {
  if(!node)
    return NULL;
  if(node->key > key) {
    return __ht_get_node(node->left, key);
  } else if(node->key < key) {
    return __ht_get_node(node->right, key);
  } else {
    return node;
  }
}

/* return the value associated with key */
void *ht_get_value(struct ht_node *node, uint64_t key) {
  struct ht_node*n = __ht_get_node(node, key);
  if(n)
    return n->value;
  return NULL;
}

/* allocate and initialize a node */
struct ht_node* __ht_new_node(uint64_t key, void *value) {
  struct ht_node* n = malloc(sizeof(struct ht_node));
  n->key = key;
  n->value = value;
  n->left = NULL;
  n->parent = NULL;
  n->right = NULL;
  n->height = 1;
  return n;
}

/* update the height of a node based on its children height */
static void __ht_update_height(struct ht_node *node) {
  if(node) {
    node->height = max(ht_height(node->left), ht_height(node->right));
    node->height++;
  }
}

static struct ht_node *__ht_right_rotate(struct ht_node *z) {
  struct ht_node *y = z->left;
  y->parent = z->parent;
  z->parent = y;
  z->left = y->right;
  y->right = z;
  __ht_update_height(z);
  __ht_update_height(y);
  return y;
}

static struct ht_node *__ht_left_rotate(struct ht_node *z) {
  struct ht_node *y = z->right;
  z->right = y->left;
  y->left = z;
  y->parent = z->parent;
  z->parent = y;
  __ht_update_height(z);
  __ht_update_height(y);
  return y;
}

static struct ht_node* __ht_balance_tree(struct ht_node*node ) {
  if(!node)
    return node;
  int balance = ht_height(node->left)-ht_height(node->right);
  struct ht_node *y, *z;
  if(balance < -1 || balance > 1) {
    z = node;

    if(ht_height(node->left) > ht_height(node->right)) {
      /* case 1 or 3 */
      y = node->left;
      if(ht_height(y->left) > ht_height(y->right)) {
	// case 1
	z = __ht_right_rotate(z);
      } else {
	// case 3
	z->left = __ht_left_rotate(y);
	z = __ht_right_rotate(z);
      }
    } else {
      /* case 2 or 4 */
      y = node->right;
      if(ht_height(y->left) < ht_height(y->right)) {
	// case 2
	z = __ht_left_rotate(z);
      } else {
	/* case 4 */
	z->right = __ht_right_rotate(y);
	z = __ht_left_rotate(z);
      }
    }
    node = z;
  }
  return node;
}

/* insert a (key, value) in the subtree node
 * returns the new root of this treee
 */
struct ht_node* ht_insert(struct ht_node* node, uint64_t key, void* value) {
  if(!node) {
    return __ht_new_node(key, value);
  }

  if(node->key > key){
    /* insert on the left */
    node->left = ht_insert(node->left, key, value);
    node->left->parent = node;
  } else if (node->key < key){
    /* insert on the right */
    node->right = ht_insert(node->right, key, value);
    node->right->parent = node;
  } else {
    /* replace the value of the current node */
    node->value = value;
    return node;
  }

  node = __ht_balance_tree(node);
  __ht_update_height(node);
  return node;
}

void connect_nodes(struct ht_node* parent,
		   struct ht_node* to_remove,
		   struct ht_node* child) {
#if 0
  printf("While removing %llx: connecting %llx and %llx\n",
	 to_remove->key, parent->key, child?child->key:NULL);
#endif
  if(parent->right == to_remove)
    parent->right = child;
  else
    parent->left = child;
  if(child)
    child->parent = parent;
}


/* todo:
   bug when running ./plop 12346
 */

/* remove key from the hash table
 * return the new root of the hash table
 */
struct ht_node* ht_remove_key(struct ht_node* node, uint64_t key) {
  struct ht_node *to_remove = node;
  struct ht_node *parent = NULL;
  struct ht_node *n=NULL;
  while(to_remove) {
    if(to_remove->key < key) {
      parent = to_remove;
      to_remove = to_remove->right;
    } else if(to_remove->key > key) {
      parent = to_remove;
      to_remove = to_remove->left;
    } else {
      /* we found the node to remove */
      break;
    }
  }
  n = parent;
  if(!to_remove) {
    /* key not found */
    return node;
  }

  if(!to_remove->right  && !to_remove->left) {
    /* to_remove is a leaf */
    //    printf("Removing a leaf\n");
    if(parent) {
      connect_nodes(parent, to_remove, NULL);
    } else {
      /* removing the root */
      node = NULL;
    }
    free(to_remove);
    /* todo: balance the tree */
  } else if (!to_remove->right || !to_remove->left) {
    /* to_remove has 1 child */

    //    printf("Removing a node with 1 child\n");

    if(parent) {
      if(to_remove->right) {
	connect_nodes(parent, to_remove, to_remove->right);
      } else {
	connect_nodes(parent, to_remove, to_remove->left);
      }
    } else {
      /* removing the root -> right/left node becomes the new root */
      if(to_remove->right)
	node = to_remove->right;
      else
	node = to_remove->left;
    }
    //    __ht_update_height(parent);
    free(to_remove);
  } else {
    /* to_remove has 2 children */
    struct ht_node* succ = to_remove->right;
    struct ht_node* succ_parent = to_remove;
    while(succ->left) {
      succ_parent = succ;
      succ = succ->left;
    }

    n = succ_parent;

    /* copy succ to to_remove and connect succ child */
    to_remove->key = succ->key;
    to_remove->value = succ->value;
    connect_nodes(succ_parent, succ, succ->right);
    /* free succ (that has being copied to to_remove */
    free(succ);
  }

  struct ht_node* new_root = node;
#if 1
#if 0
  if(n)
    printf("About to balance starting from %p (%llx)\n", n, n->key);
#endif
  struct ht_node* nbis = n;
  while (nbis) {
    __ht_update_height(n);
    nbis = nbis->parent;
  }
#if 0
  printf("Before balancing: \n");
  __ht_print(new_root, 0);
  printf("\n\n");
#endif
  n = n;
  while (n) {
    //    printf("\nBalancing %p (key %llx)\n", n, n->key);
    if(n->parent) {
      if(n->parent->left == n)
	n->parent->left = __ht_balance_tree(n);
      else if(n->parent->right == n)
	n->parent->right = __ht_balance_tree(n);
    } else {
      break;
    }
    //    __ht_print(new_root, 0);
    n = n->parent;
  }
  new_root = __ht_balance_tree(new_root);

#endif
  return new_root;
}


static void print_tabs(int nb_tabs) {
  for(int i = 0; i<nb_tabs; i++) printf("  ");
}

/* print the (key, value) stored in a hash table */
static void __ht_print(struct ht_node *node, int depth) {
  if (node) {
    print_tabs(depth);
    printf("Height %d : \"%llx\" value: %p. node=%p\n", node->height, node->key, node->value, node);

    print_tabs(depth);
    printf("left of \"%llx\"\n", node->key);
    __ht_print(node->left, depth+1);

    print_tabs(depth);
    printf("right of \"%llx\"\n", node->key);
    __ht_print(node->right, depth+1);
  }
}

/* Free a subtree */
void ht_release(struct ht_node *node) {
  if(node) {
    ht_release(node->left);
    ht_release(node->right);
    free(node);
  }
}

static void __ht_check(struct ht_node*node) {
  if(node) {
    if(node->left) {
      if(node->left->key > node->key) {
	printf("Found a violation in the binary search tree\n");
	abort();
      }
      __ht_check(node->left);
    }
    if(node->right) {
      if(node->right->key < node->key) {
	printf("Found a violation in the binary search tree\n");
	abort();
      }
      __ht_check(node->right);
    }

#if 0
    int balance = ht_height(node->left)-ht_height(node->right);
    if(balance < -1 || balance > 1) {
      printf("the tree is not balanced !\n");
      abort();
    }
#endif
#if 0
    if(node->key > 10000) {
      printf("Key %llx is way too big !\n", node->key);
      abort();
    }
#endif
  }
}

int ht_size(struct ht_node* node) {
  if(!node)
    return 0;
  return ht_size(node->left)+ht_size(node->right)+1;
}

/* return 1 if the hash table contains the key */
int ht_contains_key(struct ht_node* node, uint64_t key) {
  return __ht_get_node(node, key) != NULL;
}


/* return 1 if the hash table contains at least one key that is mapped to value */
int ht_contains_value(struct ht_node* node, void* value) {
  if(!node)
    return 0;
  if(node->value == value)
    return 1;
  return ht_contains_value(node->left, value) ||
    ht_contains_value(node->right, value);
}