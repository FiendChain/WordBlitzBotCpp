#include "unified_model.h"
#include <chrono>
#include <assert.h>

UnifiedModel::UnifiedModel(
        std::unique_ptr<BonusesModel>   &model_bonuses,
        std::unique_ptr<CharacterModel> &model_characters,
        std::unique_ptr<ValuesModel>    &model_values,
        std::shared_ptr<AppParams>      &params,
        std::shared_ptr<util::MSS>      &mss)
: m_model_bonuses(std::move(model_bonuses)),
  m_model_characters(std::move(model_characters)),
  m_model_values(std::move(model_values)),
  m_params(params),
  m_mss(mss)
{

}

void UnifiedModel::Update() {
    auto bitmap = m_mss->GetBitmap();
    auto buffer_size = bitmap.GetSize();
    auto buffer_max_size = m_mss->GetMaxSize();
    const int input_stride = sizeof(uint8_t)*4*buffer_max_size.x;

    auto &sec = bitmap.GetBitmap();
    BITMAP &bmp = sec.dsBm;
    uint8_t *buffer = (uint8_t *)(bmp.bmBits);

    int rows_skip = buffer_max_size.y - buffer_size.y;
    int rows_offset = rows_skip*input_stride;

    // points to the actual screen data
    uint8_t *data = buffer + rows_offset;
    auto &p = *m_params;

    // update bonuses model
    for (int x = 0; x < p.sqrt_grid_size; x++) {
        for (int y = 0; y < p.sqrt_grid_size; y++) {
            const int i = p.grid.GetIndex(x, y);
            auto &cell = p.grid.GetCell(i);

            const int x_start = p.cropper_bonuses.offset.x + x*p.cropper_bonuses.spacing.x;
            const int y_start = p.cropper_bonuses.offset.y + y*p.cropper_bonuses.spacing.y;
            const auto size = p.cropper_bonuses.size;

            // we do this because the bmp buffer is starts from the bottom-left
            // however our ui widget renders it from top-left
            // thus we need to flip the y coordinate
            const int y_flip = buffer_size.y-y_start-1-size.y;

            assert(x_start >= 0);
            assert(y_start >= 0);
            assert((x_start + size.x) < buffer_size.x);
            assert((y_start + size.y) < buffer_size.y);

            uint8_t *data_offset = data + y_flip*input_stride + x_start*4;
            m_model_bonuses->CopyDataToInput(data_offset, size.x, size.y, input_stride);
            m_model_bonuses->Parse();
            cell.modifier = m_model_bonuses->GetPrediction();
        }
    }

    // update characters model
    for (int x = 0; x < p.sqrt_grid_size; x++) {
        for (int y = 0; y < p.sqrt_grid_size; y++) {
            const int i = p.grid.GetIndex(x, y);
            auto &cell = p.grid.GetCell(i);

            const int x_start = p.cropper_characters.offset.x + x*p.cropper_characters.spacing.x;
            const int y_start = p.cropper_characters.offset.y + y*p.cropper_characters.spacing.y;
            const auto size = p.cropper_characters.size;

            // we do this because the bmp buffer is starts from the bottom-left
            // however our ui widget renders it from top-left
            // thus we need to flip the y coordinate
            const int y_flip = buffer_size.y-y_start-1-size.y;

            assert(x_start >= 0);
            assert(y_start >= 0);
            assert((x_start + size.x) < buffer_size.x);
            assert((y_start + size.y) < buffer_size.y);

            uint8_t *data_offset = data + y_flip*input_stride + x_start*4;
            m_model_characters->CopyDataToInput(data_offset, size.x, size.y, input_stride);
            m_model_characters->Parse();
            cell.c = m_model_characters->GetPrediction();
        }
    }

    // update values model
    for (int x = 0; x < p.sqrt_grid_size; x++) {
        for (int y = 0; y < p.sqrt_grid_size; y++) {
            const int i = p.grid.GetIndex(x, y);
            auto &cell = p.grid.GetCell(i);

            const int x_start = p.cropper_values.offset.x + x*p.cropper_values.spacing.x;
            const int y_start = p.cropper_values.offset.y + y*p.cropper_values.spacing.y;
            const auto size = p.cropper_values.size;

            // we do this because the bmp buffer is starts from the bottom-left
            // however our ui widget renders it from top-left
            // thus we need to flip the y coordinate
            const int y_flip = buffer_size.y-y_start-1-size.y;


            assert(x_start >= 0);
            assert(y_start >= 0);
            assert((x_start + size.x) < buffer_size.x);
            assert((y_start + size.y) < buffer_size.y);

            uint8_t *data_offset = data + y_flip*input_stride + x_start*4;
            m_model_values->CopyDataToInput(data_offset, size.x, size.y, input_stride);
            m_model_values->Parse();
            cell.value = m_model_values->GetPrediction();
        }
    }
}