#ifndef MODULE_H
#define MODULE_H

#include <vector>
#include <string>
#include "bstartree.h"

class Terminal
{
public:
    // constructor and destructor
    Terminal(std::string& name, size_t x, size_t y) :
        _name(name), _x1(x), _y1(y), _x2(x), _y2(y) { }
    ~Terminal()  { }

    // basic access methods
    const std::string getName()  { return _name; }
    const size_t getX1()    { return _x1; }
    const size_t getX2()    { return _x2; }
    const size_t getY1()    { return _y1; }
    const size_t getY2()    { return _y2; }

    // set functions
    void setName(std::string& name) { _name = name; }
    void setPos(size_t x1, size_t y1, size_t x2, size_t y2) {
        _x1 = x1;   _y1 = y1;
        _x2 = x2;   _y2 = y2;
    }

protected:
    std::string      _name; // module name
    size_t      _x1;        // min x coordinate of the terminal
    size_t      _y1;        // min y coordinate of the terminal
    size_t      _x2;        // max x coordinate of the terminal
    size_t      _y2;        // max y coordinate of the terminal
};


class Block : public Terminal
{
public:
    // constructor and destructor
    Block(std::string& name, size_t w, size_t h, int id) :
        Terminal(name, 0, 0), _w(w), _h(h), __rotate(false), __node(id), __best_node(id) {
            setPos(0, 0, w, h);
        }
    ~Block() { }

    // basic access methods
    const size_t getWidth()  { return __rotate? _h: _w; }
    const size_t getHeight() { return __rotate? _w: _h; }
    const bool getRotate()  { return __rotate; }
    const size_t getArea()  { return _h * _w; }
    static size_t getMaxX() { return _maxX; }
    static size_t getMaxY() { return _maxY; }

    BStarTreeNode *getNode() { return &__node; }
    BStarTreeNode *getBestNode() { return &__best_node; }

    // set functions
    void setWidth(size_t w)         { _w = w; }
    void setHeight(size_t h)        { _h = h; }
    void setRotate(bool rotate)     { __rotate = rotate; }
    static void setMaxX(size_t x)   { _maxX = x; }
    static void setMaxY(size_t y)   { _maxY = y; }

    void rotate()                   { __rotate = !__rotate; }
    void setNode(BStarTreeNode node) { __node = node; }
    void restoreBestNode() {
        __rotate = __best_rotate;
        __node = __best_node;
    }
    void updateBestNode() {
        __best_rotate = __rotate;
        __best_node = __node;
    }

private:
    size_t          _w;         // width of the block
    size_t          _h;         // height of the block
    bool            __rotate;
    bool            __best_rotate;
    static size_t   _maxX;      // maximum x coordinate for all blocks
    static size_t   _maxY;      // maximum y coordinate for all blocks

    BStarTreeNode __node;
    BStarTreeNode __best_node;
};


class Net
{
public:
    // constructor and destructor
    Net()   { }
    ~Net()  { }

    // basic access methods
    const std::vector<Terminal*> getTermList()   { return _termList; }

    // modify methods
    void addTerm(Terminal* term) { _termList.push_back(term); }

    // other member functions
    double calcHPWL();

private:
    std::vector<Terminal*>   _termList;  // list of terminals the net is connected to
};

#endif  // MODULE_H
