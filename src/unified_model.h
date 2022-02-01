#pragma once

#include <memory>
#include "wordblitz_model.h"
#include "app_params.h"
#include "util/MSS.h"
#include "wordblitz.h"

class UnifiedModel
{
public:
    std::unique_ptr<BonusesModel>   m_model_bonuses;
    std::unique_ptr<CharacterModel> m_model_characters;
    std::unique_ptr<ValuesModel>    m_model_values;
private:
    std::shared_ptr<AppParams>      m_params;
public:
    UnifiedModel(
        std::unique_ptr<BonusesModel>   &model_bonuses,
        std::unique_ptr<CharacterModel> &model_characters,
        std::unique_ptr<ValuesModel>    &model_values,
        std::shared_ptr<AppParams>      &params);

    // expects an RGBA buffer  
    void Update(const uint8_t *buffer, const int width, const int height, const int row_stride);
};