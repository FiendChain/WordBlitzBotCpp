#pragma once

#include "wordtree.h"
#include <vector>

namespace wordblitz {

struct Cursor {
    int x;
    int y;
};

struct SearchResult {
    std::vector<Cursor> path;
    std::string word;
};

enum CellModifier {
    MOD_NONE, MOD_2L, MOD_3L, MOD_2W, MOD_3W
};

struct Cell {
    char &c;
    int &value;
    CellModifier &modifier;
};

enum TraceStatus {
    INCOMPLETE, IN_PROGRESS, COMPLETE
};

struct TraceResult {
    std::vector<Cursor> path;
    std::string word;
    int value;
    TraceStatus status;
};

struct Grid {
    char *characters;
    int *values;
    CellModifier *modifiers;
    const int sqrt_size;
    const int size;
    Grid(int _sqrt_size)
    : sqrt_size(_sqrt_size), size(_sqrt_size*_sqrt_size)
    {
        int size = sqrt_size*sqrt_size;
        characters = new char[size]{0};
        values = new int[size]{0};
        modifiers = new CellModifier[size]{MOD_NONE};
    }

    const Cell GetCell(int i) const {
        return {characters[i], values[i], modifiers[i]};
    }

    Cell GetCell(int i) {
        return {characters[i], values[i], modifiers[i]};
    }

    int GetIndex(const int x, const int y) const {
        return x + sqrt_size*y;
    }
};

std::vector<SearchResult> SearchWordTree(wordtree::NodePool &pool, const char *grid, const int sqrt_size);
int GetPathValue(const Grid &grid, std::vector<Cursor> &path);
std::vector<TraceResult> GetTraceFromSearch(Grid &grid, std::vector<SearchResult> &searches);

}
