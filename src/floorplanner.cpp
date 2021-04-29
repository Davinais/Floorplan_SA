#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <vector>
#include "module.h"
#include "floorplanner.h"
using namespace std;

void Floorplanner::parseBlk(fstream& in_blk) {
    string str;
    in_blk >> str;
    if (str == "Outline:") {
        in_blk >> str;
        __outline_width = stoi(str);
        in_blk >> str;
        __outline_height = stoi(str);
    }
    in_blk >> str;
    if (str == "NumBlocks:") {
        in_blk >> str;
        __num_blocks = stoi(str);
    }
    in_blk >> str;
    if (str == "NumTerminals:") {
        in_blk >> str;
        __num_terminals = stoi(str);
    }

    string module_name, x_str, y_str;
    int terminal_idx = 0;
    int block_idx = 0;
    __terminal_array.reserve(__num_terminals);
    __block_array.reserve(__num_blocks);
    while(in_blk >> module_name) {
        in_blk >> str;
        if (str == "terminal") {
            in_blk >> x_str >> y_str;
            __terminal_array.emplace_back(shared_ptr<Terminal>(new Terminal(module_name, stoi(x_str), stoi(y_str))));
            __name2terminal_idx.insert({module_name, terminal_idx});
            terminal_idx++;
        }
        else {
            in_blk >> y_str;
            size_t width = stoi(str), height = stoi(y_str);
            // Swap to make sure width > height
            if (width < height) {
                size_t tmp = width;
                width = height;
                height = tmp;
            }
            __block_array.emplace_back(shared_ptr<Block>(new Block(module_name, width, height, block_idx)));
            __name2block_idx.insert({module_name, block_idx});
            block_idx++;
        }
    }
}

void Floorplanner::parseNet(fstream& in_net) {
    string str;
    in_net >> str;
    if (str == "NumNets:") {
        in_net >> str;
        __num_nets = stoi(str);
    }

    while(in_net >> str) {
        if (str == "NetDegree:") {
            int net_degree = 0;
            in_net >> str;
            net_degree = stoi(str);

            unique_ptr<Net> net(new Net());
            // net->getTermList().reserve(net_degree);

            string blk_name;
            while(--net_degree >= 0) {
                in_net >> blk_name;
                auto block_it = __name2block_idx.find(blk_name);
                // Ignore terminal now
                if(block_it != __name2block_idx.end()) {
                    net->addTerm(__block_array[block_it->second].get());
                }
                else {
                    auto terminal_it = __name2terminal_idx.find(blk_name);
                    assert(terminal_it != __name2terminal_idx.end());
                    net->addTerm(__terminal_array[terminal_it->second].get());
                }
            }
            __net_array.emplace_back(move(net));
        }
    }
}

PlaceStatus Floorplanner::placeAt(BStarTreeNode *parent, BStarTreeNode *current, NodeChild child, bool check/*=true*/) {
    assert(current != nullptr);
    bool rotate = false;
    auto curr_block = __block_array[current->get_blk_id()];
    if (parent == nullptr) {
        if (curr_block->getWidth(rotate) <= __outline_width && curr_block->getWidth(rotate) <= __outline_height) {
            __tree_root = current;
            curr_block->setPos(0, 0, curr_block->getWidth(rotate), curr_block->getHeight(rotate));
            update_contour(curr_block);
            return PlaceStatus::SUCCESS;
        }
        else {
            rotate = !rotate;
            if (curr_block->getWidth(rotate) <= __outline_width && curr_block->getWidth(rotate) <= __outline_height) {
                __tree_root = current;
                curr_block->setPos(0, 0, curr_block->getWidth(rotate), curr_block->getHeight(rotate));
                update_contour(curr_block);
                return PlaceStatus::ROTATED;
            }
            else {
                return PlaceStatus::FAIL;
            }
        }
    }
    else {
        auto parent_block = __block_array[parent->get_blk_id()];
        size_t x1 = (child == NodeChild::LEFT) ? parent_block->getX2() : parent_block->getX1();
        size_t x2 = x1 + curr_block->getWidth(rotate);
        size_t y1 = find_max_y(x1, x2);
        size_t y2 = y1 + curr_block->getHeight(rotate);
        if (!check || (x2 <= __outline_width && y2 <= __outline_height)) {
            parent->insert(current, child);
            curr_block->setPos(x1, y1, x2, y2);
            update_contour(curr_block);
            return PlaceStatus::SUCCESS;
        }
        else {
            rotate = !rotate;
            x2 = x1 + curr_block->getWidth(rotate);
            y1 = find_max_y(x1, x2);
            y2 = y1 + curr_block->getHeight(rotate);
            if (x2 <= __outline_width && y2 <= __outline_height) {
                parent->insert(current, child);
                curr_block->setPos(x1, y1, x2, y2);
                update_contour(curr_block);
                return PlaceStatus::ROTATED;
            }
            else {
                return PlaceStatus::FAIL;
            }
        }
    }
}

size_t Floorplanner::find_max_y(size_t x1, size_t x2) {
    size_t max_y = 0;
    for(auto& contour : __horizontal_contour) {
        if (contour->x >= x1 && contour->x < x2) {
            max_y = max(max_y, contour->y);
        }
        else if (contour->x >= x2) {
            break;
        }
    }
    return max_y;
}

void Floorplanner::update_contour(const shared_ptr<Block>& block) {
    size_t x1 = block->getX1();
    size_t x2 = block->getX2();
    size_t y1 = block->getY1();
    size_t y2 = block->getY2();
    if (__horizontal_contour.empty()) {
        __horizontal_contour.emplace_back(unique_ptr<Coordinate>(new Coordinate {x1, y2}));
        __horizontal_contour.emplace_back(unique_ptr<Coordinate>(new Coordinate {x2, 0}));
    }
    else {
        size_t curr_x_in_contour = 0;
        size_t curr_y_in_contour = 0, prev_y_in_contour = 0;
        bool reach_end = true, inserted = false;
        for(auto co_it = __horizontal_contour.begin(); co_it != __horizontal_contour.end();) {
            curr_x_in_contour = (*co_it)->x;
            curr_y_in_contour = (*co_it)->y;
            if (curr_x_in_contour >= x1 && curr_x_in_contour <= x2) {
                if (y2 > curr_y_in_contour) {
                    co_it = __horizontal_contour.erase(co_it);
                    if (!inserted) {
                        __horizontal_contour.emplace(co_it, unique_ptr<Coordinate>(new Coordinate {x1, y2}));
                        inserted = true;
                    }
                }
                else {
                    reach_end = false;
                    break;
                }
            }
            else if (curr_x_in_contour > x2) {
                __horizontal_contour.emplace(co_it, unique_ptr<Coordinate>(new Coordinate {x2, prev_y_in_contour}));
                reach_end = false;
                break;
            }
            else {
                co_it++;
            }
            prev_y_in_contour = curr_y_in_contour;
        }
        if (reach_end) {
            __horizontal_contour.emplace_back(unique_ptr<Coordinate>(new Coordinate {x2, 0}));
        }
    }
}

void Floorplanner::clear_contour() {
    __horizontal_contour.clear();
}

void Floorplanner::floorplan() {
    getInitialFloorplan();
}

void Floorplanner::getInitialFloorplan() {
    list<shared_ptr<Block>> blocks(__block_array.begin(), __block_array.end());
    blocks.sort(
        [] (const shared_ptr<Block> &l, const shared_ptr<Block> &r) {
            return l->getWidth() > r->getWidth();
        }
    );

    // Initial the B*-tree to a left-skewed tree
    PlaceStatus place_ret;
    place_ret = placeAt(nullptr, blocks.front()->getNode(), NodeChild::LEFT);
    blocks.pop_front();
    if (place_ret == PlaceStatus::FAIL) {
        cerr << "The largest macro cannot be contained, exited." << endl;
        exit(1);
    }

    BStarTreeNode *level_start = __tree_root;
    BStarTreeNode *prev_level_start = nullptr;
    while(!blocks.empty()) {
        if (level_start == nullptr) {
            level_start = blocks.front()->getNode();
            place_ret = placeAt(prev_level_start, level_start, NodeChild::RIGHT, false);
            blocks.pop_front();
        }

        auto curr_block = level_start;
        auto block_it = blocks.begin();
        while(block_it != blocks.end()) {
            place_ret = placeAt(curr_block, (*block_it)->getNode(), NodeChild::LEFT);
            if (place_ret != PlaceStatus::FAIL) {
                curr_block = (*block_it)->getNode();
                block_it = blocks.erase(block_it);
            }
            else {
                block_it++;
            }
        }
        prev_level_start = level_start;
        level_start = level_start->get_right();
    }
}
