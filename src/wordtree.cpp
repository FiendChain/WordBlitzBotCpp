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

char IndexToChar(uint8_t i) {
    return i + 'a';
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
            // our reference may be invalid after the emplace since the pool could resize
            auto &new_curr_node = pool.at(curr_node_index);
            new_curr_node.children[child_index] = pool_index;
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

bool RemoveWordTree(NodePool &pool, const char *word, const int length) {
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
        // doesn't exist
        } else {
            return false;
        }
    }

    auto &curr_node = pool.at(curr_node_index);

    // didnt remove
    if (!curr_node.is_leaf) {
        return false;
    }

    curr_node.is_leaf = false;
    return true;
}

bool TraverseWordTree(NodePool &pool, const std::basic_string<char> &s) {
    return TraverseWordTree(pool, s.c_str(), s.length());
}

bool InsertWordTree(NodePool &pool, const std::basic_string<char> &s) {
    return InsertWordTree(pool, s.c_str(), s.length());
}

bool RemoveWordTree(NodePool &pool, const std::basic_string<char> &s) {
    return RemoveWordTree(pool, s.c_str(), s.length());
}

// helper for writing it recursively
bool RecursiveWriteWordTree(
    NodePool &pool, Node &node,
    uint8_t *buffer, int &buffer_index, uint32_t &node_count) 
{
    node_count++;

    if (node.is_leaf) {
        buffer[buffer_index++] = '|';
    }

    bool wrote_children = false;

    for (uint8_t i = 0; i < MAX_BRANCHES; i++) {
        uint32_t child_index = node.children[i];
        if (child_index == 0) {
            continue;
        }
        buffer[buffer_index++] = IndexToChar(i);
        bool v = RecursiveWriteWordTree(pool, pool.at(child_index), buffer, buffer_index, node_count);
        // undo a write if the child didn't write anything
        if (!v) {
            buffer_index--;
        }
        wrote_children = v || wrote_children;
    }

    // if this isn't a leaf and it didnt write any children, unpop it
    if (!wrote_children && !node.is_leaf) {
        node_count--;
        return false;
    }

    buffer[buffer_index++] = '$';
    return true;
}

std::vector<uint8_t> WriteWordTree(NodePool &pool) {
    const int num_nodes = pool.size();
    // we take a conservative estimate of the worst case output file size
    // if each node takes a full traversal and is marked as a leaf
    // then it takes 3*n bytes, where n is the number of nodes
    std::vector<uint8_t> buffer;

    // the serialised format saves the number of nodes to the first 4 bytes
    buffer.resize(3*num_nodes + sizeof(uint32_t));

    Node& root = pool.at(0);
    int buffer_index = 0;
    uint32_t node_count = 0;
    RecursiveWriteWordTree(pool, root, buffer.data()+sizeof(node_count), buffer_index, node_count);

    buffer.resize(buffer_index + sizeof(uint32_t));
    // write the node count to the start of the buffer
    *reinterpret_cast<uint32_t*>(buffer.data()) = node_count;

    return buffer;
}

}