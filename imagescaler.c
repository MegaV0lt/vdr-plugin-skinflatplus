/*
 * Skin flatPlus: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "./imagescaler.h"

#include <vdr/tools.h>

#include <array>
#include <cstdlib>
#include <cmath>

ImageScaler::ImageScaler() :
    m_memory(nullptr),
    m_hor_filters(nullptr),
    m_ver_filters(nullptr),
    m_buffer(nullptr),
    m_dst_image(nullptr),
    m_dst_stride(0),
    m_dst_width(0),
    m_dst_height(0),
    m_src_width(0),
    m_src_height(0),
    m_src_x(0),
    m_src_y(0),
    m_dst_x(0),
    m_dst_y(0) {
}

ImageScaler::~ImageScaler() {
    if (m_memory) {
        free(m_memory);
        m_memory = nullptr;
    }
}

// sin(x)/(x)
static float sincf(float x) {
    if (fabsf(x) < 0.05f)
        return 1.0f - (1.0f / 6.0f) * x * x;  // Taylor series approximation to avoid 0/0

    return sin(x) / x;
}

static void CalculateFilters(ImageScaler::Filter *filters, int dst_size, int src_size) {
    const float fc {(dst_size >= src_size) ? 1.0f : (dst_size * 1.0f / src_size)};

    const int d {2 * dst_size};  // Sample position denominator
    int e {0}, offset {0};       // Init outside of loop
    float sub_offset {0.0f}, norm {0.0f}, t {0.0f};
    std::array<float, 4> h_arr {0.0f, 0.0f, 0.0f, 0.0f};
    for (int i {0}; i < dst_size; ++i) {
        // d = 2 * dst_size;                       // Sample position denominator
        e = (2 * i + 1) * src_size - dst_size;  // Sample position enumerator
        offset = e / d;                         // Truncated sample position
        // Exact sample position is (float) e/d = offset + sub_offset
        /* const float */  // sub_offset = (static_cast<float>(e - offset * d) / static_cast<float>(d));
        sub_offset = (e - offset * d) * (1.0f / d);

        // Calculate filter coefficients
        for (uint j {0}; j < 4; ++j) {
            t = 3.14159265359f * (sub_offset + (1 - j));
            h_arr[j] = sincf(fc * t) * cosf(0.25f * t);  // Sinc-low pass and cos-window
        }

        // Ensure that filter does not reach out off image bounds:
        while (offset < 1) {
            h_arr[0] += h_arr[1];
            h_arr[1] = h_arr[2];
            h_arr[2] = h_arr[3];
            h_arr[3] = 0.0f;
            ++offset;
        }

        while (offset + 3 > src_size) {
            h_arr[3] += h_arr[2];
            h_arr[2] = h_arr[1];
            h_arr[1] = h_arr[0];
            h_arr[0] = 0.0f;
            --offset;
        }

        // Coefficients are normalized to sum up to 2048
        norm = 2048.0f / (h_arr[0] + h_arr[1] + h_arr[2] + h_arr[3]);

        --offset;  // Offset of fist used pixel

        filters[i].m_offset = offset + 4;  // Store offset of first unused pixel

        for (uint j {0}; j < 4; ++j) {
            t = norm * h_arr[j];
            filters[i].m_coeff[(offset + j) & 3] =
                static_cast<int>((t > 0.0f) ? (t + 0.5f) : (t - 0.5f));  // Consider ring buffer index permutations
        }
    }

    // Set end marker
    filters[dst_size].m_offset = (unsigned) -1;
}

void ImageScaler::SetImageParameters(unsigned *dst_image, unsigned dst_stride, unsigned dst_width, unsigned dst_height,
                                     unsigned src_width, unsigned src_height) {
    m_src_x = 0;
    m_src_y = 0;
    m_dst_x = 0;
    m_dst_y = 0;

    m_dst_image  = dst_image;
    m_dst_stride = dst_stride;

    // If image dimensions do not change we can keep the old filter coefficients
    if ((src_width == m_src_width) && (src_height == m_src_height) && (dst_width == m_dst_width) &&
        (dst_height == m_dst_height))
        return;

    m_dst_width  = dst_width;
    m_dst_height = dst_height;
    m_src_width  = src_width;
    m_src_height = src_height;

    if (m_memory) {
        // Free memory only if it was allocated before
        free(m_memory);
        m_memory = nullptr;
    }

    // Narrowing conversion
    const size_t hor_filters_size = (m_dst_width + 1) * sizeof(Filter);  // Reserve one extra position for end marker
    const size_t ver_filters_size = (m_dst_height + 1) * sizeof(Filter);
    const size_t buffer_size = 4 * m_dst_width * sizeof(TmpPixel);

    char *p = reinterpret_cast<char *>(malloc(hor_filters_size + ver_filters_size + buffer_size));
    if (!p) exit(EXIT_FAILURE);  // Unable to allocate memory!

    m_memory = p;

    m_hor_filters = reinterpret_cast<Filter *>(p);
    m_ver_filters = reinterpret_cast<Filter *>(p + hor_filters_size);
    m_buffer      = reinterpret_cast<TmpPixel *>(p + hor_filters_size + ver_filters_size);

    CalculateFilters(m_hor_filters, m_dst_width , m_src_width);
    CalculateFilters(m_ver_filters, m_dst_height, m_src_height);
}

// Shift range to 0..255 and clamp overflows
static unsigned shift_clamp(int x) {
    // x = (x + 2^21) >> 22;
    // But that's equivalent to this:
    x = (x + 2097152) >> 22;
    return (x < 0) ? 0 : (x > 255) ? 255 : x;
}

inline unsigned ImageScaler::PackPixel(const TmpPixel &pixel) {
    return shift_clamp(pixel[0]) |
           (shift_clamp(pixel[1]) << 8) |
           (shift_clamp(pixel[2]) << 16) |
           (shift_clamp(pixel[3]) << 24);
}

/**
 * Processes the next source line for vertical image scaling
 * 
 * Applies vertical filtering coefficients to calculate output pixels
 * using a 4-tap filter. Each output pixel is computed as weighted sum
 * of 4 input pixels using pre-calculated coefficients.
 * 
 * The filter coefficients (h0-h3) are optimized for minimal ringing
 * while preserving sharpness.
 */
void ImageScaler::NextSourceLine() {
    m_dst_x = 0;
    m_src_x = 0;
    m_src_y++;

    if (m_dst_y > m_dst_height) {
        esyslog("ImageScaler::NextSourceLine: m_dst_y (%d) > m_dst_height (%d)", m_dst_y, m_dst_height);
        return;  // Protect against buffer overrun
    }

    int h0 {0}, h1 {0}, h2 {0}, h3 {0};  // Init outside of loop
    while (m_ver_filters[m_dst_y].m_offset == m_src_y) {
        /* const int */       h0  = m_ver_filters[m_dst_y].m_coeff[0];
        /* const int */       h1  = m_ver_filters[m_dst_y].m_coeff[1];
        /* const int */       h2  = m_ver_filters[m_dst_y].m_coeff[2];
        /* const int */       h3  = m_ver_filters[m_dst_y].m_coeff[3];
        const TmpPixel       *src = m_buffer;
        unsigned             *dst = m_dst_image + m_dst_stride * m_dst_y;

        for (unsigned i {0}; i < m_dst_width; ++i) {
            const TmpPixel t(src[0] * h0 + src[1] * h1 + src[2] * h2 + src[3] * h3);
            src += 4;
            dst[i] = PackPixel(t);
        }

        m_dst_y++;
    }
}
