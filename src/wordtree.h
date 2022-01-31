#pragma once

#include <stdint.h>
#include <string>
#include <vector>

namespace wordtree {

constexpr int MAX_BRANCHES = 26;

struct Node;

typedef std::vector<Node> NodePool;

uint8_t FindIndex(char c);

struct Node {
    bool is_leaf = false;
    uint32_t children[MAX_BRANCHES] = {0};
};



void ReadWordTree(const char *buffer, const int buffer_size, NodePool &pool, const int max_stack_size);
bool TraverseWordTree(NodePool &pool, const char *word, const int length);
bool InsertWordTree(NodePool &pool, const char *word, const int length);

bool TraverseWordTree(NodePool &pool, const std::basic_string<char> &s);
bool InsertWordTree(NodePool &pool, const std::basic_string<char> &s);


}