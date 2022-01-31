#include "buffer_graphics.h"
#include <algorithm>

void DrawCenteredRectInBuffer(
    int cx, int cy, int wx, int wy,
    const RGBA<uint8_t> pen_color, const int pen_width,
    RGBA<uint8_t> *buffer, int width, int height, int row_stride)
{
    int px_start = std::clamp(cx-wx, 0          , width-pen_width);
    int px_end   = std::clamp(cx+wx, pen_width-1, width-1);
    int py_start = std::clamp(cy-wy, 0          , height-pen_width);
    int py_end   = std::clamp(cy+wy, pen_width-1, height-1);

    // draw each of the line
    for (int x = px_start; x <= px_end; x++) {
        for (int j = 0; j < pen_width; j++) {
            buffer[x + (py_start+j)*row_stride] = pen_color;
            buffer[x + (py_end  -j)*row_stride] = pen_color;
        }
    }
    for (int y = py_start; y <= py_end; y++) {
        for (int j = 0; j < pen_width; j++) {
            buffer[px_start+j + y*row_stride] = pen_color;
            buffer[px_end  -j + y*row_stride] = pen_color;
        }
    }
}

void DrawRectInBuffer(
    int x_start, int y_start, int x_end, int y_end,
    const RGBA<uint8_t> pen_color, const int pen_width,
    RGBA<uint8_t> *buffer, int width, int height, int row_stride)
{
    int px_start = std::clamp(x_start, 0          , width-pen_width);
    int px_end   = std::clamp(x_end  , pen_width-1, width-1);
    int py_start = std::clamp(y_start, 0          , height-pen_width);
    int py_end   = std::clamp(y_end  , pen_width-1, height-1);

    // draw each of the line
    for (int x = px_start; x <= px_end; x++) {
        for (int j = 0; j < pen_width; j++) {
            buffer[x + (py_start+j)*row_stride] = pen_color;
            buffer[x + (py_end  -j)*row_stride] = pen_color;
        }
    }
    for (int y = py_start; y <= py_end; y++) {
        for (int j = 0; j < pen_width; j++) {
            buffer[px_start+j + y*row_stride] = pen_color;
            buffer[px_end  -j + y*row_stride] = pen_color;
        }
    }
}