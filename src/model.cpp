#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image_resize.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include <stdio.h>
#include <chrono>
#include <thread>
#include <stdexcept>

#include <fmt/core.h>

#include "tensorflow/lite/c/c_api.h"
#include "tensorflow/lite/c/common.h"

#include "model.h"

static void PrintTfLiteModelSummary(TfLiteInterpreter *interpreter);
static void PrintTfLiteTensorSummary(const TfLiteTensor *tensor);

Model::Model(const char *filepath)
{
    // load model
    m_model = TfLiteModelCreateFromFile(filepath);
    m_options = TfLiteInterpreterOptionsCreate();
    // const uint32_t num_threads = std::thread::hardware_concurrency();
    // NOTE: disable threading since the overhead due to multithread is too high
    const uint32_t num_threads = 1;
    TfLiteInterpreterOptionsSetNumThreads(m_options, num_threads);
    // Create the interpreter.
    m_interp = TfLiteInterpreterCreate(m_model, m_options);
    // Allocate tensors and populate the input tensor data.
    TfLiteInterpreterAllocateTensors(m_interp);

    // verify input size matches
    TfLiteTensor* input_tensor = TfLiteInterpreterGetInputTensor(m_interp, 0);
    if (input_tensor->dims->size != 4) {
        throw std::runtime_error(fmt::format(
            "Model expected input tensor shape of dimension 4, got {}",
            input_tensor->dims->size
        ));
    }

    size_t input_size = 1; 
    {
        for (int i = 0; i < input_tensor->dims->size; i++) {
            input_size *= input_tensor->dims->data[i];
        }
    }

    // format of input tensor shape is: (1,height,width,channels)
    m_height = input_tensor->dims->data[1];
    m_width = input_tensor->dims->data[2];
    m_channels = input_tensor->dims->data[3];
    m_num_pixels = m_height * m_width;

    if (m_channels != 3) {
        throw std::runtime_error(fmt::format(
            "Model expected model with 3 channels, got ({},{},{})", 
            m_width, m_height, m_channels));
    }

    // verify output size matches
    const TfLiteTensor* output_tensor = TfLiteInterpreterGetOutputTensor(m_interp, 0);
    m_output_size = 1; 
    {
        for (int i = 0; i < output_tensor->dims->size; i++) {
            m_output_size *= output_tensor->dims->data[i];
        }
    }

    // allocate buffer after all checks completed
    m_input_buffer = new RGB<float>[m_num_pixels]{0.0f,0.0f,0.0f};
    m_resize_buffer = new RGBA<uint8_t>[m_num_pixels]{0,0,0,0};
    m_output_buffer = new float[m_output_size]{0.0f};
}

Model::~Model() {
    // Dispose of the model and interpreter objects.
    TfLiteInterpreterDelete(m_interp);
    TfLiteInterpreterOptionsDelete(m_options);
    TfLiteModelDelete(m_model);
    delete[] m_input_buffer;
    delete[] m_resize_buffer;
    delete[] m_output_buffer;
}

void Model::Print() {
    PrintTfLiteModelSummary(m_interp);
}

bool Model::CopyDataToInput(const uint8_t *data, const int width, const int height, const int row_stride) {
    // resize image here
    stbir_resize_uint8(
        data, width, height, row_stride,
        (uint8_t*)m_resize_buffer, m_width, m_height, 0,
        4);

    for (int y = 0; y < m_height; y++) {
        for (int x = 0; x < m_width; x++) {
            const int i = x + y*m_width;

            // model requires the image to be flipped
            // channels are also the same order
            const int j = x + (m_height-y-1)*m_width;

            m_input_buffer[i].r = ((float)m_resize_buffer[j].r) / 255.0f;
            m_input_buffer[i].g = ((float)m_resize_buffer[j].g) / 255.0f;
            m_input_buffer[i].b = ((float)m_resize_buffer[j].b) / 255.0f;
        }
    }
    return true;
}

void Model::Parse() {
    TfLiteTensor* input_tensor = TfLiteInterpreterGetInputTensor(m_interp, 0);
    const TfLiteTensor* output_tensor = TfLiteInterpreterGetOutputTensor(m_interp, 0);
    // Copy and run
    const int num_channels = 3;
    TfLiteTensorCopyFromBuffer(input_tensor, m_input_buffer, m_num_pixels*m_channels*sizeof(float));
    TfLiteInterpreterInvoke(m_interp);
    // Extract the output tensor data.
    TfLiteTensorCopyToBuffer(
        output_tensor, 
        m_output_buffer, sizeof(float)*m_output_size);
}

void Model::RunBenchmark(const int n) {
    // Time the average duration of an invoke
    float ms_avg_invoke = 0.0f;
    for (int i = 0; i < n; i++) {
        auto timer_start = std::chrono::high_resolution_clock::now();
        TfLiteInterpreterInvoke(m_interp);
        auto timer_end = std::chrono::high_resolution_clock::now();
        long long ms_start = std::chrono::time_point_cast<std::chrono::milliseconds>(timer_start).time_since_epoch().count();
        long long ms_end = std::chrono::time_point_cast<std::chrono::milliseconds>(timer_end).time_since_epoch().count();
        long long ms_dt = ms_end-ms_start;
        ms_avg_invoke += (float)(ms_dt);
    }
    ms_avg_invoke /= (float)(n);
    printf("%.2fms per invoke (N=%d)\n", ms_avg_invoke, n);
}

void PrintTfLiteModelSummary(TfLiteInterpreter *interpreter) {
    int input_tensor_count = TfLiteInterpreterGetInputTensorCount(interpreter);
    int output_tensor_count = TfLiteInterpreterGetOutputTensorCount(interpreter);
    printf("inputs=%d, output=%d\n", input_tensor_count, output_tensor_count);

    for (int i = 0; i < input_tensor_count; i++) {
        const TfLiteTensor* input_tensor = TfLiteInterpreterGetInputTensor(interpreter, i);
        printf("inp_tensor[%d]: ", i);
        PrintTfLiteTensorSummary(input_tensor);
    }

    for (int i = 0; i < output_tensor_count; i++) {
        const TfLiteTensor* output_tensor = TfLiteInterpreterGetOutputTensor(interpreter, i);
        printf("out_tensor[%d]: ", i);
        PrintTfLiteTensorSummary(output_tensor);
    }
}

void PrintTfLiteTensorSummary(const TfLiteTensor *tensor) {
    printf("%s (", tensor->name);
    // size of tensor
    for (int j = 0; j < tensor->dims->size; j++) {
        int dim = tensor->dims->data[j];
        if (j != tensor->dims->size-1) {
            printf("%d,", dim);
        } else {
            printf("%d) ", dim);
        }
    }
    // type of tensor
    TfLiteType t = TfLiteTensorType(tensor);
    printf("%s ", TfLiteTypeGetName(t));
    // quantisation?
    if (t == kTfLiteUInt8) {
        TfLiteQuantizationParams qparams = TfLiteTensorQuantizationParams(tensor);
        printf("[scale=%.2f, zero_point=%d]\n", qparams.scale, qparams.zero_point);
    } else {
        printf("\n");
    }
}

