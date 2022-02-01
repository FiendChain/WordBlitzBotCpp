#pragma once

#include "wordblitz.h"

struct Vec2D {
    int x;
    int y;
};

struct GridCropper {
    Vec2D offset = {0, 0};
    Vec2D size = {30, 30};
    Vec2D spacing = {60, 60};
};

struct AppParams {
    const int sqrt_grid_size;
    const int grid_size;
    GridCropper cropper_bonuses;
    GridCropper cropper_values;
    GridCropper cropper_characters;
    Vec2D inter_buffer_size;
    wordblitz::Grid grid;

    AppParams(const int _sqrt_grid_size)
    : sqrt_grid_size(_sqrt_grid_size),
      grid_size(_sqrt_grid_size*_sqrt_grid_size),
      grid(_sqrt_grid_size)
    {}
};