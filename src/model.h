#pragma once

#include "tensorflow/lite/c/c_api.h"
#include "tensorflow/lite/c/common.h"

#include "wordblitz.h"

template <typename T>
struct RGBA {
    T r, g, b, a;
};

template <typename T>
struct RGB {
    T r, g, b;
};

class Model
{
public:
    struct Vec2D {
        int x;
        int y;
    };
private:
    TfLiteModel *m_model;
    TfLiteInterpreterOptions *m_options;
    TfLiteInterpreter *m_interp;

    RGB<float> *m_input_buffer;
    float *m_output_buffer;
    int m_output_size;

    int m_width;
    int m_height;
    int m_num_pixels;
    int m_channels;
public:
    Model(const char *filepath);
    ~Model();
    // need to do a mapping to uint8_t to float
    bool CopyDataToInput(const uint8_t *data, const int width, const int height);
    void Parse();
    inline RGB<float> *GetInputBuffer() { return m_input_buffer; }
    inline Vec2D GetInputSize() const { return {m_width, m_height}; }
    inline int GetOutputSize() const { return m_output_size; }
    void Print();
    void RunBenchmark(const int n=100);
};