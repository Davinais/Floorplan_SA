#ifndef __FLOORPLANNER_H__
#define __FLOORPLANNER_H__

#include <list>
#include <map>
#include <memory>
#include <random>
#include <vector>
#include <string>
#include "bstartree.h"
#include "module.h"

enum class PlaceStatus : char {
    SUCCESS = 0,
    ROTATED = 1,
    FAIL    = 2
};

struct Coordinate {
    size_t x;
    size_t y;
};

class Floorplanner {
    public:
        Floorplanner(double alpha, std::fstream& in_blk, std::fstream& in_net):
            __alpha(alpha), __tree_root(nullptr), __prev_tree_root(nullptr), __best_tree_root(nullptr),
            __best_tree_cost(0.0), __area_norm(0.0), __hpwl_norm(), __rng(12345) {
            parseBlk(in_blk);
            parseNet(in_net);
            __outline_ratio = (double)__outline_width/(double)__outline_height;
        }
        ~Floorplanner() {}

        void floorplan();
        void getInitialFloorplan();
        void printSummary(std::fstream& out_log, double exe_time);
        void parseBlk(std::fstream& in_blk);
        void parseNet(std::fstream& in_net);
        double calcHPWL();
        double calcTreeCost();
        bool checkLegal() {
            return (Block::getMaxX() <= __outline_width && Block::getMaxY() <= __outline_height);
        }
        void simNormValue();
        void simulateAnnealing();

        PlaceStatus placeAt(BStarTreeNode *parent, BStarTreeNode *current, NodeChild child, bool check=true);
        size_t find_max_y(size_t x1, size_t x2);
        void update_contour(const std::shared_ptr<Block>& block);
        void clear_contour();
        void swap_block(const std::shared_ptr<Block>& l, const std::shared_ptr<Block>& r) {
            if (l->getNode() == __tree_root) {
                __tree_root = r->getNode();
            }
            else if (r->getNode() == __tree_root) {
                __tree_root = l->getNode();
            }
            BStarTreeNode::swap_position(l->getNode(), r->getNode());
        }
        void remove_block_from_tree(const std::shared_ptr<Block>& block);
        void random_insert(const std::shared_ptr<Block>& block);
        void random_operations();
        void update_all_blocks();

    private:
        double __alpha;
        double __outline_ratio;
        int __outline_width;                // Outline Width
        int __outline_height;               // Outline Height
        int __num_blocks;                   // Blocks Num
        int __num_terminals;                // Terminals Num
        int __num_nets;                     // Nets Num

        double __area_norm;
        double __hpwl_norm;

        std::mt19937 __rng;

        std::vector<std::unique_ptr<Net>> __net_array;           // net array of the circuit
        std::vector<std::shared_ptr<Terminal>> __terminal_array; // module array of the circuit
        std::vector<std::shared_ptr<Block>> __block_array;       // module array of the circuit
        std::map<std::string, int> __name2block_idx;
        std::map<std::string, int> __name2terminal_idx;

        BStarTreeNode *__tree_root;
        /* Careful that the __best_tree_root cannot be traversed since the pointer it stored is pointed to original nodes */
        BStarTreeNode *__prev_tree_root;
        BStarTreeNode *__best_tree_root;
        double __best_tree_cost;

        std::list<std::unique_ptr<Coordinate>> __horizontal_contour;
        // std::list<std::unique_ptr<Coordinate>> __vertical_contour;

        void __updatePrevTree();
        void __restorePrevTree();
        void __updateBestTree();
        void __restoreBestTree();
};

#endif