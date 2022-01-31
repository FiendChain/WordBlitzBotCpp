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
    std::shared_ptr<util::MSS>      m_mss;
public:
    UnifiedModel(
        std::unique_ptr<BonusesModel>   &model_bonuses,
        std::unique_ptr<CharacterModel> &model_characters,
        std::unique_ptr<ValuesModel>    &model_values,
        std::shared_ptr<AppParams>      &params,
        std::shared_ptr<util::MSS>      &mss);
    
    void Update();
};