#include "unified_model.h"
#include <chrono>
#include <assert.h>
#include <functional>
#include <stdexcept>

UnifiedModel::UnifiedModel(
        std::unique_ptr<BonusesModel>   &model_bonuses,
        std::unique_ptr<CharacterModel> &model_characters,
        std::unique_ptr<ValuesModel>    &model_values,
        std::shared_ptr<AppParams>      &params)
: m_model_bonuses(std::move(model_bonuses)),
  m_model_characters(std::move(model_characters)),
  m_model_values(std::move(model_values)),
  m_params(params)
{

}

typedef std::function<void (const uint8_t *, const int, const int, const int, wordblitz::Cell &)> GridIteratorCallback;

void check_and_throw(bool v, const char *message) {
    if (!v) {
        throw std::runtime_error(message);
    }
}

void IterateBufferUsingCropper(
    const uint8_t *buffer, const int width, const int height, const int row_stride,
    const float xscale, const float yscale,
    wordblitz::Grid &grid, GridCropper &cropper, GridIteratorCallback callback)
{
    const int sqrt_size = grid.sqrt_size;
    for (int x = 0; x < sqrt_size; x++) {
        for (int y = 0; y < sqrt_size; y++) {
            const int i = grid.GetIndex(x, y);
            auto &cell = grid.GetCell(i);

            const int x_start = cropper.offset.x + x*cropper.spacing.x;
            const int y_start = cropper.offset.y + y*cropper.spacing.y;
            const auto size = cropper.size;

            // rescale to fit cropper into actual image
            const int x_rescale = (int)(xscale * (float)x_start);
            const int y_rescale = (int)(yscale * (float)y_start);
            const int width_rescale = (int)(xscale * (float)size.x);
            const int height_rescale = (int)(yscale * (float)size.y);

            // we do this because the bmp buffer is starts from the bottom-left
            // however our ui widget renders it from top-left
            // thus we need to flip the y coordinate
            const int y_flip = height-y_rescale-1-height_rescale;

            check_and_throw(x_rescale >= 0, "Cropper grid goes past left");
            check_and_throw(y_rescale >= 0, "Cropper grid goes past top");
            check_and_throw((x_rescale + width_rescale) < width, "Cropper grid goes past right");
            check_and_throw((y_rescale + height_rescale) < height, "Cropper grid goes past bottom");

            const uint8_t *data = buffer + y_flip*row_stride + x_rescale*4;
            callback(data, width_rescale, height_rescale, row_stride, cell);
        }
    }
}

void UnifiedModel::Update(const uint8_t *buffer, const int width, const int height, const int row_stride) {
    const uint8_t *data = buffer;
    auto &p = *m_params;

    const float xscale = (float)width / (float)p.inter_buffer_size.x;
    const float yscale = (float)height / (float)p.inter_buffer_size.y;

    IterateBufferUsingCropper(
        buffer, width, height, row_stride,
        xscale, yscale,
        p.grid, p.cropper_bonuses, 
        [this](
            const uint8_t *data, 
            const int width, const int height, const int row_stride, 
            wordblitz::Cell &cell) 
        {
            m_model_bonuses->CopyDataToInput(data, width, height, row_stride);
            m_model_bonuses->Parse();
            cell.modifier = m_model_bonuses->GetPrediction();
        });

    IterateBufferUsingCropper(
        buffer, width, height, row_stride,
        xscale, yscale,
        p.grid, p.cropper_characters, 
        [this](
            const uint8_t *data, 
            const int width, const int height, const int row_stride, 
            wordblitz::Cell &cell) 
        {
            m_model_characters->CopyDataToInput(data, width, height, row_stride);
            m_model_characters->Parse();
            cell.c = m_model_characters->GetPrediction();
        });

    IterateBufferUsingCropper(
        buffer, width, height, row_stride,
        xscale, yscale,
        p.grid, p.cropper_values, 
        [this](
            const uint8_t *data, 
            const int width, const int height, const int row_stride, 
            wordblitz::Cell &cell) 
        {
            m_model_values->CopyDataToInput(data, width, height, row_stride);
            m_model_values->Parse();
            cell.value = m_model_values->GetPrediction();
        });
}