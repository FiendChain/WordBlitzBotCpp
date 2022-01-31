#include "wordtree.h"
#include <stdexcept>
#include <cassert>

namespace wordtree {

uint8_t FindIndex(char c) {
    uint8_t i = c - 'a';
    if ((i < 0) || (i >= MAX_BRANCHES)) {
        throw std::runtime_error("Unknown character provided");
    }
    return i;
}

void ReadWordTree(const char *buffer, const int buffer_size, NodePool &pool, const int max_stack_size) {
    uint32_t node_count = *(uint32_t*)(buffer);

    pool.resize(node_count);
    uint32_t pool_index = 0;

    uint32_t *node_stack = new uint32_t[max_stack_size+1];
    // the index where the most recent node is
    int stack_index = 0;

    uint32_t curr_node_index = pool_index++;
    node_stack[stack_index] = curr_node_index;

    for (int i = 4; i < buffer_size; i++) {
        char c = buffer[i];
        auto &node = pool.at(curr_node_index);
        // end of leaf
        if (c == '$') {
            // finished making tree
            if (stack_index == 0) {
                break;
            // pop the stack
            } else {
                curr_node_index = node_stack[--stack_index];
            }
            continue;
        }

        // if intermediate node is an endpoint
        if (c == '|') {
            node.is_leaf = true;
            continue;
        }

        node.is_leaf = false;

        if (pool_index >= node_count) {
            throw std::runtime_error("Node index exceed node count");
        }

        if (stack_index >= max_stack_size) {
            throw std::runtime_error("Stack depth exceeded provided value");
        }

        uint8_t child_index = FindIndex(c);
        node.children[child_index] = pool_index;
        curr_node_index = pool_index;
        node_stack[++stack_index] = curr_node_index;
        pool_index++;
    }

    assert(pool_index == node_count);

    delete[] node_stack;
}

bool TraverseWordTree(NodePool &pool, const char *word, const int length) {
    uint32_t curr_node_index = 0;
    for (int i = 0; i < length; i++) {
        const char c = word[i];
        auto &curr_node = pool[curr_node_index];
        uint8_t child_index = FindIndex(c);
        curr_node_index = curr_node.children[child_index];
        if (curr_node_index == 0) {
            return false;
        }
    }
    auto &curr_node = pool[curr_node_index];
    return curr_node.is_leaf;
}

bool InsertWordTree(NodePool &pool, const char *word, const int length) {
    uint32_t curr_node_index = 0;
    uint32_t pool_index = pool.size();
    for (int i = 0; i < length; i++) {
        char c = word[i];
        auto &curr_node = pool.at(curr_node_index);
        uint8_t child_index = FindIndex(c);
        uint32_t next_node_index = curr_node.children[child_index];
        // already exists
        if (next_node_index != 0) {
            curr_node_index = next_node_index;
        // allocate
        } else {
            pool.emplace_back();
            curr_node.children[child_index] = pool_index;
            curr_node_index = pool_index++;
        }
    }

    auto &curr_node = pool.at(curr_node_index);
    if (curr_node.is_leaf) {
        return false;
    }
    curr_node.is_leaf = true;
    return true;
}

bool TraverseWordTree(NodePool &pool, const std::basic_string<char> &s) {
    return TraverseWordTree(pool, s.c_str(), s.length());
}

bool InsertWordTree(NodePool &pool, const std::basic_string<char> &s) {
    return InsertWordTree(pool, s.c_str(), s.length());
}


}