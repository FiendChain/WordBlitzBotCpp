#pragma once

#include "model.h"
#include "wordblitz.h"

class ValuesModel: public Model 
{
private:
    int m_pred;
public:
    ValuesModel(const char *filepath);
    void Parse() override;
    int GetPrediction() const { return m_pred; }
};

class CharacterModel: public Model
{
private:
    char m_char;
public:
    CharacterModel(const char *filepath);
    void Parse() override;
    char GetPrediction() const { return m_char; }
};

class BonusesModel: public Model 
{
private:
    wordblitz::CellModifier m_pred;
public:
    BonusesModel(const char *filepath);
    void Parse() override;
    wordblitz::CellModifier GetPrediction() const { return m_pred; }
};