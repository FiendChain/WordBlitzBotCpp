#include "wordblitz_model.h"
#include <stdexcept>
#include <fmt/core.h>

int argmax(const float *arr, const int N) {
    float max_val = arr[0];
    int max_index = 0; 
    for (int i = 1; i < N; i++) {
        const float v = arr[i];
        if (v > max_val) {
            max_val = v;
            max_index = i;
        }
    }
    return max_index;
}

ValuesModel::ValuesModel(const char *filepath)
: Model(filepath)
{
    if (GetOutputSize() != 22) {
        throw std::runtime_error(fmt::format(
            "Value models got incorrect output size ({}), expected ({})",
            GetOutputSize(), 22
        ));
    }
    m_pred = 0;
}

void ValuesModel::Parse() {
    Model::Parse();
    bool is_single_digit = argmax(m_output_buffer, 2) == 1;
    int left  = argmax(m_output_buffer+2   , 10);
    int right = argmax(m_output_buffer+2+10, 10);
    if (is_single_digit) {
        m_pred = right;
    } else {
        m_pred = left*10 + right;
    }
}

CharacterModel::CharacterModel(const char *filepath)
: Model(filepath)
{
    if (GetOutputSize() != 26) {
        throw std::runtime_error(fmt::format(
            "Character model got incorrect output size ({}), expected ({})",
            GetOutputSize(), 26
        ));
    }
    m_char = 0;
}

void CharacterModel::Parse() {
    Model::Parse();
    int i = argmax(m_output_buffer, 26);
    m_char = 'a' + i;
}

BonusesModel::BonusesModel(const char *filepath)
: Model(filepath)
{
    if (GetOutputSize() != 5) {
        throw std::runtime_error(fmt::format(
            "Bonuses model got incorrect output size ({}), expected ({})",
            GetOutputSize(), 5
        ));
    }
    m_pred = wordblitz::CellModifier::MOD_NONE;
}

void BonusesModel::Parse() {
    Model::Parse();
    int i = argmax(m_output_buffer, 5);
    switch (i) {
    case 0: m_pred = wordblitz::CellModifier::MOD_NONE; break;
    case 1: m_pred = wordblitz::CellModifier::MOD_2L; break;
    case 2: m_pred = wordblitz::CellModifier::MOD_2W; break;
    case 3: m_pred = wordblitz::CellModifier::MOD_3L; break;
    case 4: m_pred = wordblitz::CellModifier::MOD_3W; break;
    default:
        m_pred = wordblitz::CellModifier::MOD_NONE; 
        break;
    }
}