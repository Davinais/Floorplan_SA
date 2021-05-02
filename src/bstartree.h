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
    // Copy constructor
    BStarTreeNode(const BStarTreeNode& other):
        __blk_id(other.__blk_id), __left(other.__left), __right(other.__right), __parent(other.__parent) {}
    // Copy assignment operator
    BStarTreeNode& operator=(const BStarTreeNode& other) {
        __blk_id = other.__blk_id;
        __parent = other.__parent;
        __left = other.__left;
        __right = other.__right;
        return *this;
    }

    int get_blk_id() { return __blk_id; }
    BStarTreeNode *get_parent() { return __parent; }
    BStarTreeNode *get_left() { return __left; }
    BStarTreeNode *get_right() { return __right; }

    static void swap_blk_id(BStarTreeNode *l, BStarTreeNode *r) {
        int tmp = l->__blk_id;
        l->__blk_id = r->__blk_id;
        r->__blk_id = tmp;
    }
    static void swap_position(BStarTreeNode *l, BStarTreeNode *r) {
        BStarTreeNode tmp = *l;
        *l = *r;
        *r = tmp;
        r->__blk_id = l->__blk_id;
        l->__blk_id = tmp.__blk_id;
        if(l->__parent != nullptr) {
            if (l->__parent != l) {
                l->__parent->replace_child(l, (l->__parent->get_left() == r) ? NodeChild::LEFT : NodeChild::RIGHT);
            }
            else {
                l->replace_parent(r);
                r->replace_child(l, (r->get_left() == r) ? NodeChild::LEFT : NodeChild::RIGHT);
            }
        }
        if(r->__parent != nullptr) {
            if (r->__parent != r) {
                r->__parent->replace_child(r, (r->__parent->get_left() == l) ? NodeChild::LEFT : NodeChild::RIGHT);
            }
            else {
                r->replace_parent(l);
                l->replace_child(r, (l->get_left() == l) ? NodeChild::LEFT : NodeChild::RIGHT);
            }
        }
        if(l->__left != nullptr && l->__left != l) { l->__left->replace_parent(l); }
        if(l->__right != nullptr && l->__right != l) { l->__right->replace_parent(l); }
        if(r->__left != nullptr && r->__left != r) { r->__left->replace_parent(r); }
        if(r->__right != nullptr && r->__right != r) { r->__right->replace_parent(r); }

        // check_valid(l);
        // check_valid(r);
        // check_valid(l->__parent);
        // check_valid(l->__left);
        // check_valid(l->__right);
        // check_valid(r->__parent);
        // check_valid(r->__left);
        // check_valid(r->__right);
    };
    static void check_valid(BStarTreeNode *node) {
        if (node == nullptr) return;
        assert(node->__parent != node && node->__left != node && node->__right != node);
        if (node->__parent != nullptr) {
            assert(node->__left != node->__parent && node->__right != node->__parent);
        }
        if (node->__left != nullptr) {
            assert(node->__left != node->__right && node->__left != node->__parent);
        }
        if (node->__right != nullptr) {
            assert(node->__right != node->__left && node->__right != node->__parent);
        }
    }
    void replace_parent(BStarTreeNode *node) { __parent = node; }
    void replace_child(BStarTreeNode *node, NodeChild child) {
        if (child == NodeChild::LEFT) {
            __left = node;
        }
        else {
            __right = node;
        }
    }
    void isolate() {
        // assert(__left == nullptr || __right == nullptr);
        BStarTreeNode *child = (__left != nullptr) ? __left : __right;
        if (__parent != nullptr) {
            __parent->replace_child(child, (__parent->__left == this) ? NodeChild::LEFT : NodeChild::RIGHT);
        }
        if (child != nullptr) {
            child->replace_parent(__parent);
        }
        __parent = nullptr;
        __left = nullptr;
        __right = nullptr;
    }
    void insert(BStarTreeNode *node, NodeChild child) {
        // assert(node->__parent == nullptr);
        node->__parent = this;
        if (child == NodeChild::LEFT) {
            // assert(__left == nullptr);
            __left = node;
        }
        else {
            // assert(__right == nullptr);
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