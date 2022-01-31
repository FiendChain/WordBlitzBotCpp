#pragma once
#include <stdint.h>

template <typename T>
struct RGBA {
    T r, g, b, a;
};

template <typename T>
struct RGB {
    T r, g, b;
};


void DrawCenteredRectInBuffer(
    int cx, int cy, int wx, int wy,
    const RGBA<uint8_t> pen_color, const int pen_width,
    RGBA<uint8_t> *buffer, int width, int height, int row_stride);

void DrawRectInBuffer(
    int x_start, int y_start, int x_end, int y_end,
    const RGBA<uint8_t> pen_color, const int pen_width,
    RGBA<uint8_t> *buffer, int width, int height, int row_stride);