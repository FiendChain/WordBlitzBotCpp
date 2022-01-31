#include "wordblitz.h"
#include <stdint.h>
#include <unordered_map>
#include <algorithm>

using namespace wordtree;

namespace wordblitz {

static void RecursiveSearchWordTree(
    NodePool &pool, uint32_t parent_node_index,
    const char *grid, bool *tracker, 
    int x, int y,
    std::vector<SearchResult> &results,
    char *word_stack, Cursor *cursor_stack,
    int depth,
    const int sqrt_size) 
{
    // helper functions
    const auto GetIndex = [&sqrt_size](const int x, const int y) -> int {
        return x + y*sqrt_size;
    };

    // try to set the current cell
    const int cell_index = GetIndex(x, y);
    auto &cell = tracker[cell_index];
    char c = grid[cell_index];
    if (cell) {
        return;
    }

    // check if this character goes into the tree
    Node &parent = pool.at(parent_node_index);
    int child_index = FindIndex(c);
    uint32_t node_index = parent.children[child_index];
    // doesn't exist
    if (node_index == 0) {
        return;
    }

    // push
    cell = true;
    word_stack[depth] = c;
    cursor_stack[depth] = {x, y};
    Node &node = pool.at(node_index);
    if (node.is_leaf) {
        SearchResult r = {
            {cursor_stack, cursor_stack+depth+1},
            {word_stack, word_stack+depth+1}
        };
        results.emplace_back(r);
    }

    // set the cell and start depth first search
    for (int xoff = -1; xoff <= 1; xoff++) {
        for (int yoff = -1; yoff <= 1; yoff++) {
            if ((xoff == 0) && (yoff == 0)) {
                continue;
            }
            int xn = x + xoff;
            int yn = y + yoff;
            // ignore if outside of bounds
            if ((xn < 0) || (xn >= sqrt_size) || 
                (yn < 0) || (yn >= sqrt_size))
            {
                continue;
            }
            // perform search
            RecursiveSearchWordTree(
                pool, node_index, 
                grid, tracker,
                xn, yn, 
                results,
                word_stack, cursor_stack,
                depth+1, 
                sqrt_size);
        }
    }

    // pop
    cell = false;
}

std::vector<SearchResult> SearchWordTree(NodePool &pool, const char *grid, const int sqrt_size) {
    std::vector<SearchResult> results;
    const int size = sqrt_size*sqrt_size;

    char *word_stack = new char[64]{0};
    bool *tracker = new bool[size]{false};
    Cursor *cursor_stack = new Cursor[size]{{-1,-1}};

    // search the tree
    for (int x = 0; x < sqrt_size; x++) {
        for (int y = 0; y < sqrt_size; y++) {
            RecursiveSearchWordTree(
                pool, 0,
                grid, tracker,
                x, y, 
                results,
                word_stack, cursor_stack,
                0, 
                sqrt_size);
        }
    }

    delete[] tracker;
    delete[] word_stack;
    delete[] cursor_stack;
    return results;
}

int GetPathValue(const Grid &grid, std::vector<Cursor> &path) {
    int multiplier = 1;
    int total_value = 0;

    for (auto &c: path) {
        const int i = grid.GetIndex(c.x, c.y);
        const auto cell = grid.GetCell(i);

        switch (cell.modifier) {
        case CellModifier::MOD_NONE:
            total_value += cell.value;
            break;
        case CellModifier::MOD_2L:
            total_value += 2*cell.value;
            break;
        case CellModifier::MOD_3L:
            total_value += 3*cell.value;
            break;
        case CellModifier::MOD_2W:
            multiplier *= 2;
            break;
        case CellModifier::MOD_3W:
            multiplier *= 3;
            break;
        default:
            break;
        }
    }

    return (multiplier * total_value) + path.size();
}

std::vector<TraceResult> GetTraceFromSearch(Grid &grid, std::vector<SearchResult> &searches) {
    std::unordered_map<std::string, TraceResult> unique_traces;
    for (auto &search: searches) {
        auto &path = search.path;
        auto &word = search.word;
        int value = GetPathValue(grid, path);
        // default place
        if (unique_traces.find(word) == unique_traces.end()) {
            unique_traces.insert({word, {path, word, value, TraceStatus::INCOMPLETE}});
            continue;
        }

        // get the trace if it exists, and replace if we have a better value
        auto &prev_trace = unique_traces.at(word);
        if (prev_trace.value >= value) {
            continue;
        }

        unique_traces.insert({word, {path, word, value, TraceStatus::INCOMPLETE}});
    }

    // create a vector of value sorted results
    std::vector<TraceResult> traces;
    for (auto &[word, trace]: unique_traces) {
        traces.emplace_back(std::move(trace));
    }

    std::sort(traces.begin(), traces.end(), [](const TraceResult &a, const TraceResult &b) {
        return a.value > b.value;
    });

    return traces;
}

}