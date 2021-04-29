#ifndef __B_STAR_TREE_H__
#define __B_STAR_TREE_H__

#include <cassert>

enum class NodeChild {
    LEFT,
    RIGHT
};

class BStarTreeNode
{
public:
    BStarTreeNode(int id): __blk_id(id), __left(nullptr), __right(nullptr), __parent(nullptr) {}
    ~BStarTreeNode() {}

    int get_blk_id() { return __blk_id; }
    BStarTreeNode *get_parent() { return __parent; }
    BStarTreeNode *get_left() { return __left; }
    BStarTreeNode *get_right() { return __right; }

    void set_parent(BStarTreeNode *node) { __parent = node; }
    void insert(BStarTreeNode *node, NodeChild child) {
        assert(node->__parent == nullptr);
        node->__parent = this;
        if (child == NodeChild::LEFT) {
            assert(__left == nullptr);
            __left = node;
        }
        else {
            assert(__right == nullptr);
            __right = node;
        }
    }

private:
    int __blk_id;
    BStarTreeNode *__parent;
    BStarTreeNode *__left;
    BStarTreeNode *__right;
};

#endif