#ifndef TREE
#define TREE

enum node_link
{
  L = 0,
  R = 1
};

enum dup_link
{
  P = 0,
  N = 1
};

struct dupnode
{
  struct dupnode *links[2];
  struct node *parent;
};

struct node
{
  struct node *links[2];
  struct dupnode *dups;
};

struct tree
{
  struct node *root;
};

#endif
