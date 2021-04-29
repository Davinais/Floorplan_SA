#ifndef __FLOORPLANNER_H__
#define __FLOORPLANNER_H__

#include <list>
#include <map>
#include <memory>
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
        Floorplanner(std::fstream& in_blk, std::fstream& in_net): __tree_root(nullptr) {
            parseBlk(in_blk);
            parseNet(in_net);
        }
        ~Floorplanner() {}

        void floorplan();
        void getInitialFloorplan();
        void parseBlk(std::fstream& in_blk);
        void parseNet(std::fstream& in_net);

        PlaceStatus placeAt(BStarTreeNode *parent, BStarTreeNode *current, NodeChild child, bool check=true);
        size_t find_max_y(size_t x1, size_t x2);
        void update_contour(const std::shared_ptr<Block>& block);
        void clear_contour();

    private:
        int __outline_width;                // Outline Width
        int __outline_height;               // Outline Height
        int __num_blocks;                   // Blocks Num
        int __num_terminals;                // Terminals Num
        int __num_nets;                     // Nets Num

        std::vector<std::unique_ptr<Net>> __net_array;           // net array of the circuit
        std::vector<std::shared_ptr<Terminal>> __terminal_array; // module array of the circuit
        std::vector<std::shared_ptr<Block>> __block_array;       // module array of the circuit
        std::map<std::string, int> __name2block_idx;
        std::map<std::string, int> __name2terminal_idx;

        BStarTreeNode *__tree_root;
        BStarTreeNode *__best_tree_root;

        std::list<std::unique_ptr<Coordinate>> __horizontal_contour;
        // std::list<std::unique_ptr<Coordinate>> __vertical_contour;
};

#endif