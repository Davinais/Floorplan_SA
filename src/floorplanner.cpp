#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <random>
#include <stack>
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
        curr_block->setRotate(rotate);
        if (curr_block->getWidth() <= __outline_width && curr_block->getWidth() <= __outline_height) {
            __tree_root = current;
            curr_block->setPos(0, 0, curr_block->getWidth(), curr_block->getHeight());
            update_contour(curr_block);
            return PlaceStatus::SUCCESS;
        }
        else {
            curr_block->setRotate(!rotate);
            if (curr_block->getWidth() <= __outline_width && curr_block->getWidth() <= __outline_height) {
                __tree_root = current;
                curr_block->setPos(0, 0, curr_block->getWidth(), curr_block->getHeight());
                update_contour(curr_block);
                return PlaceStatus::ROTATED;
            }
            else {
                curr_block->setRotate(rotate);
                return PlaceStatus::FAIL;
            }
        }
    }
    else {
        auto parent_block = __block_array[parent->get_blk_id()];
        size_t x1 = (child == NodeChild::LEFT) ? parent_block->getX2() : parent_block->getX1();
        size_t x2 = x1 + curr_block->getWidth();
        size_t y1 = find_max_y(x1, x2);
        size_t y2 = y1 + curr_block->getHeight();
        if (!check || (x2 <= __outline_width && y2 <= __outline_height)) {
            parent->insert(current, child);
            curr_block->setPos(x1, y1, x2, y2);
            update_contour(curr_block);
            return PlaceStatus::SUCCESS;
        }
        else {
            curr_block->setRotate(!rotate);
            x2 = x1 + curr_block->getWidth();
            y1 = find_max_y(x1, x2);
            y2 = y1 + curr_block->getHeight();
            if (x2 <= __outline_width && y2 <= __outline_height) {
                parent->insert(current, child);
                curr_block->setPos(x1, y1, x2, y2);
                update_contour(curr_block);
                return PlaceStatus::ROTATED;
            }
            else {
                curr_block->setRotate(rotate);
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
    if (Block::getMaxX() < x2) {
        Block::setMaxX(x2);
    }
    if (Block::getMaxY() < y2) {
        Block::setMaxY(y2);
    }
}

void Floorplanner::clear_contour() {
    __horizontal_contour.clear();
}

void Floorplanner::remove_block_from_tree(const shared_ptr<Block>& block) {
    auto node = block->getNode();

    uniform_int_distribution<int> dist(0, 1);
    int rand_num = dist(__rng);
    assert(__tree_root->get_parent() == nullptr);
    while (node->get_left() != nullptr && node->get_right() != nullptr) {
        if (rand_num == 0) {
            swap_block(block, __block_array[node->get_left()->get_blk_id()]);
        }
        else {
            swap_block(block, __block_array[node->get_right()->get_blk_id()]);
        }
        rand_num = dist(__rng);
    }
    if (node == __tree_root) {
        __tree_root = (node->get_left() != nullptr) ? node->get_left() : node->get_right();
    }
    node->isolate();
    assert(__tree_root->get_parent() == nullptr);
}

void Floorplanner::random_insert(const shared_ptr<Block>& block) {
    uniform_int_distribution<int> dist_idx(0, __num_blocks-1);
    uniform_int_distribution<int> dist_child(0, 1);

    // If dist_idx get the index of the block itself, insert at root
    auto node = block->getNode();
    int ins_idx = dist_idx(__rng);
    while(node->get_blk_id() == ins_idx) {
        ins_idx = dist_idx(__rng);
    }
    NodeChild child = (dist_child(__rng) == 0) ? NodeChild::LEFT : NodeChild::RIGHT;
    if (node->get_blk_id() == ins_idx) {
        __tree_root->replace_parent(node);
        if (__tree_root->get_left() != nullptr) {
            __tree_root->get_left()->replace_parent(node);
        }
        node->replace_child(__tree_root, NodeChild::RIGHT);
        node->replace_child(__tree_root->get_left(), NodeChild::LEFT);
        __tree_root->replace_child(nullptr, NodeChild::LEFT);
        BStarTreeNode::check_valid(node);
        BStarTreeNode::check_valid(__tree_root);
        __tree_root = node;
    }
    else {
        auto ins_node = __block_array[ins_idx]->getNode();
        if (child == NodeChild::LEFT) {
            if(ins_node->get_left() != nullptr) {
                ins_node->get_left()->replace_parent(node);
                node->replace_child(ins_node->get_left(), child);
            }
        }
        else {
            if(ins_node->get_right() != nullptr) {
                ins_node->get_right()->replace_parent(node);
                node->replace_child(ins_node->get_right(), child);
            }
        }
        node->replace_parent(ins_node);
        ins_node->replace_child(node, child);
        BStarTreeNode::check_valid(node);
        BStarTreeNode::check_valid(ins_node);
    }
}

void Floorplanner::random_operations() {
    uniform_int_distribution<int> dist_idx(0, __num_blocks-1);
    uniform_int_distribution<int> dist_ops(0, 2);
    int idx_1 = dist_idx(__rng);
    int idx_2 = dist_idx(__rng);
    switch(dist_ops(__rng)) {
        case 0:
            __block_array[idx_1]->rotate();
            break;
        case 1:
            remove_block_from_tree(__block_array[idx_1]);
            random_insert(__block_array[idx_1]);
            break;
        case 2:
        default:
            while (idx_1 == idx_2) {
                idx_2 = dist_idx(__rng);
            }
            swap_block(__block_array[idx_1], __block_array[idx_2]);
            break;
    }
}

void Floorplanner::update_all_blocks() {
    clear_contour();
    Block::setMaxX(0);
    Block::setMaxY(0);

    stack<BStarTreeNode *> workspace;
    size_t blk_num = 0;
    if (__tree_root != nullptr) {
        workspace.push(__tree_root);
    }
    while(!workspace.empty()) {
        BStarTreeNode *curr = workspace.top();
        workspace.pop();
        blk_num++;
        if (curr->get_right() != nullptr) {
            workspace.push(curr->get_right());
        }
        if (curr->get_left() != nullptr) {
            workspace.push(curr->get_left());
        }
        auto& curr_block = __block_array[curr->get_blk_id()];
        if (curr->get_parent() != nullptr) {
            auto& parent_block = __block_array[curr->get_parent()->get_blk_id()];
            size_t x1 = (curr->get_parent()->get_left() == curr) ? parent_block->getX2() : parent_block->getX1();
            size_t x2 = x1 + curr_block->getWidth();
            size_t y1 = find_max_y(x1, x2);
            size_t y2 = y1 + curr_block->getHeight();
            curr_block->setPos(x1, y1, x2, y2);
        }
        else {
            curr_block->setPos(0, 0, curr_block->getWidth(), curr_block->getHeight());
        }
        update_contour(curr_block);
    }
    assert(blk_num == __num_blocks);
}

void Floorplanner::floorplan() {
    getInitialFloorplan();
    __updateBestTree();
    update_all_blocks();
    cout << "[Init] Max(" << Block::getMaxX() << ", " << Block::getMaxY() << ")" << endl;
    simNormValue();
    __restoreBestTree();
    simulateAnnealing();
    __restoreBestTree();
    update_all_blocks();
}

double Floorplanner::calcHPWL() {
    double hpwl = 0.0;
    for(auto& net : __net_array) {
        hpwl += net->calcHPWL();
    }
    return hpwl;
}

double Floorplanner::calcTreeCost() {
    double penalty_area = checkLegal() ? 0.0 : 2.0;
    double penalty_ratio = checkLegal() ? 0.0 : 2.0;
    double wl_concern = checkLegal() ? 1.0 : 0.0;
    double cost =
        ((__alpha + penalty_area) * ((Block::getMaxX()*Block::getMaxY())/__area_norm)) +
        (wl_concern * (1 - __alpha) * (calcHPWL()/__hpwl_norm)) +
        (penalty_ratio * abs((Block::getMaxX()/(double)Block::getMaxY()) - __outline_ratio));
    return cost;
}

void Floorplanner::simNormValue() {
    const int sim_time = 50;
    __area_norm = 0.0;
    __hpwl_norm = 0.0;
    for(int i =0; i < sim_time; i++) {
        random_operations();
        update_all_blocks();
        // cout << "[" << i << "] Max(" << Block::getMaxX() << ", " << Block::getMaxY() << ")" << endl;
        __area_norm += (double)(Block::getMaxX() * Block::getMaxY());
        __hpwl_norm += calcHPWL();
    }
    __area_norm /= sim_time;
    __hpwl_norm /= sim_time;
    cout << "[Final] A_norm: " << __area_norm << " / HPWL_norm: " << __hpwl_norm << endl; 
}

void Floorplanner::simulateAnnealing() {
    const int sim_time = __num_blocks * 5;
    const double reduce_rate = 0.999;
    const double frozen_temp = 0.01;
    const double heat_temperature = 1.0;

    int heat_time = 0;
    double temperature = heat_temperature;

    uniform_real_distribution<double> dist_prob(0.0, 1.0);
    __best_tree_cost = calcTreeCost();

    double new_cost, delta;
    double prev_cost = __best_tree_cost;
    while (temperature > frozen_temp) {
        __restoreBestTree();
        for (int i = 0; i < sim_time; i++) {
            random_operations();
            update_all_blocks();
            new_cost = calcTreeCost();
            delta = new_cost - prev_cost;
            if (delta <= 0) {
                prev_cost = new_cost;
                __updatePrevTree();
                if (new_cost < __best_tree_cost) {
                    cout << "Best: " << new_cost << " / Valid=" << checkLegal() << endl;
                    __best_tree_cost = new_cost;
                    __updateBestTree();
                }
            }
            else if (dist_prob(__rng) < exp(-delta/temperature)) {
                prev_cost = new_cost;
                __updatePrevTree();
            }
            else {
                __restorePrevTree();
            }
        }
        if (!checkLegal() && heat_time < 5) {
            temperature = heat_temperature;
            heat_time++;
        }
        else {
            temperature *= reduce_rate;
        }
    }
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

void Floorplanner::__updatePrevTree() {
    for(auto& block : __block_array) {
        block->updatePrevNode();
    }
    __prev_tree_root = __block_array[__tree_root->get_blk_id()]->getPrevNode();
}

void Floorplanner::__restorePrevTree() {
    for(auto& block : __block_array) {
        block->restorePrevNode();
    }
    __tree_root = __block_array[__prev_tree_root->get_blk_id()]->getNode();
}

void Floorplanner::__updateBestTree() {
    for(auto& block : __block_array) {
        block->updateBestNode();
    }
    __best_tree_root = __block_array[__tree_root->get_blk_id()]->getBestNode();
}

void Floorplanner::__restoreBestTree() {
    for(auto& block : __block_array) {
        block->restoreBestNode();
    }
    __tree_root = __block_array[__best_tree_root->get_blk_id()]->getNode();
}

void Floorplanner::printSummary(fstream& out_log, double exe_time) {
    double area = Block::getMaxX()*Block::getMaxY();
    double hpwl = calcHPWL();
    out_log << __alpha*area + (1-__alpha)*hpwl << endl;
    out_log << hpwl << endl;
    out_log << area << endl;
    out_log << Block::getMaxX() << " " << Block::getMaxY() << endl;
    out_log << exe_time << endl;
    for(auto& block : __block_array) {
        out_log << block->getName() << " "
                << block->getX1() << " "
                << block->getY1() << " "
                << block->getX2() << " "
                << block->getY2() << endl;
    }
}

void Floorplanner::plot(string file_name) {
	string plot_dir = "plot";
    string cmd;
    int ret;

    cmd = "mkdir -p " + plot_dir;
    ret = system(cmd.c_str());
	char data_path[] = "/tmp/gnuplotXXXXXX";
    int fd_data = mkstemp(data_path);
    ofstream data_file(data_path);

	char gp_path[] = "/tmp/gnuplotXXXXXX";
    int fd_gp = mkstemp(gp_path);
    ofstream gp_file(gp_path);
    
    data_file << __outline_width << " " << __outline_height << endl;
    gp_file << "set arrow from 0, " << __outline_height << " to "
            << __outline_width << ", " << __outline_height << " nohead lc 3 lw 5" << endl;
    gp_file << "set arrow from " << __outline_width << ", 0 to "
            << __outline_width << ", " << __outline_height << " nohead lc 3 lw 5" << endl;
    
    int i = 0;
    string fillcolor;
    for(auto& block : __block_array) {
        size_t x1 = block->getX1();
        size_t x2 = block->getX2();
        size_t y1 = block->getY1();
        size_t y2 = block->getY2();
        fillcolor = (x2 > __outline_width || y2 > __outline_height) ? "light-red" : "light-green" ;
        data_file << x1 << " " << y1 << endl
                  << x2 << " " << y2 << endl;
        gp_file << "set object "  <<  i+1  <<  " rect from " << x1 << "," << y1
                << " to " << x2 << "," << y2 << " fc rgb '" << fillcolor
                << "' fs solid border lc rgb 'black' lw 3" << endl;
        gp_file << "set label '" << block->getName() << "' at " << (x1+x2)/2.0
                << ", " << (y1+y2)/2.0 << " front center font ',32'" << endl;
        i++;
    }

    gp_file << "set term png size 2000, 2000" <<endl;
    gp_file << "set output '" << plot_dir << "/" <<file_name << "'" << endl;
    gp_file << "plot '" << data_path << "' using 1:2 with points" << endl;
    data_file.close();
    gp_file.close();

    cmd = "gnuplot " + string(gp_path);
    ret = system(cmd.c_str());
    cout<< "Plot saved in " << plot_dir << "/" << file_name << endl;
    remove(data_path);
    remove(gp_path);
}
