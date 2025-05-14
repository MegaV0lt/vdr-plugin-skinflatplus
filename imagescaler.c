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
// #include <memory>

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

ImageScaler::~ImageScaler() = default;  // No need for manual memory management

/**
 * Computes the normalized sinc function, sin(x)/x.
 *
 * For small values of x, a Taylor series approximation is used to avoid
 * division by zero and improve numerical stability.
 *
 * @param x The input value for which the sinc function is calculated.
 * @return The value of sin(x)/x, or an approximation for small x.
 */
inline float sincf(float x) {
    if (fabsf(x) < 0.05f)
        return 1.0f - (1.0f / 6.0f) * x * x;  // Taylor series approximation to avoid 0/0

    return sin(x) / x;
}

/**
 * Calculates horizontal and vertical filter coefficients for image scaling
 *
 * The image is upscaled by a factor of dst_size/src_size, which is a
 * rational number. The filter coefficients are calculated using the
 * sinc-low pass filter and a cosine window. The filter coefficients are
 * normalized to sum up to 2048.
 *
 * The function stores the filter coefficients in the array filters, which
 * has size dst_size. The coefficients are stored in the order
 * h0, h1, h2, h3 for each filter. The offset of the first unused pixel is
 * stored in the member m_offset of the Filter struct.
 *
 * The function also sets an end marker in the last element of the array
 * filters, which is used to detect the end of the array.
 *
 * @param filters pointer to an array of Filter structs
 * @param dst_size destination image size
 * @param src_size source image size
 */
static void CalculateFilters(ImageScaler::Filter *filters, int dst_size, int src_size) {
    const float fc {(dst_size >= src_size) ? 1.0f : (dst_size * 1.0f / src_size)};

    constexpr float PI {3.14159265359f};
    constexpr float SCALE_FACTOR {2048.0};
    const int d {2 * dst_size};  // Sample position denominator
    int e {0}, offset {0};       // Init outside of loop
    float sub_offset {0.0f}, norm {0.0f}, t {0.0f};
    std::array<float, 4> h_arr {0.0f, 0.0f, 0.0f, 0.0f};
    for (int i {0}; i < dst_size; ++i) {
        e = (2 * i + 1) * src_size - dst_size;  // Sample position enumerator
        offset = e / d;                         // Truncated sample position
        // Exact sample position is (float) e/d = offset + sub_offset
        /* const float */  // sub_offset = (static_cast<float>(e - offset * d) / static_cast<float>(d));
        sub_offset = (e - offset * d) * (1.0f / d);

        // Calculate filter coefficients
        for (uint j {0}; j < 4; ++j) {
            t = PI * (sub_offset + (1 - j));
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
        norm = SCALE_FACTOR / (h_arr[0] + h_arr[1] + h_arr[2] + h_arr[3]);

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

/**
 * Sets the parameters for the image scaling operation.
 *
 * This function initializes the destination image buffer, stride, and dimensions,
 * as well as the source image dimensions. It recalculates the horizontal and vertical
 * filter coefficients if the image dimensions have changed. The function uses a 
 * unique_ptr for managing memory allocation required for filter coefficients and 
 * temporary buffer storage.
 *
 * @param dst_image Pointer to the destination image buffer.
 * @param dst_stride The stride (row length in memory) of the destination image.
 * @param dst_width The width of the destination image.
 * @param dst_height The height of the destination image.
 * @param src_width The width of the source image.
 * @param src_height The height of the source image.
 */
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

    // Narrowing conversion
    const size_t hor_filters_size = (m_dst_width + 1) * sizeof(Filter);  // Reserve one extra position for end marker
    const size_t ver_filters_size = (m_dst_height + 1) * sizeof(Filter);
    const size_t buffer_size = 4 * m_dst_width * sizeof(TmpPixel);

    // Use a std::unique_ptr to manage memory
    m_memory = std::unique_ptr<char[]>(new char[hor_filters_size + ver_filters_size + buffer_size]);

    m_hor_filters = reinterpret_cast<Filter *>(m_memory.get());
    m_ver_filters = reinterpret_cast<Filter *>(m_memory.get() + hor_filters_size);
    m_buffer      = reinterpret_cast<TmpPixel *>(m_memory.get() + hor_filters_size + ver_filters_size);

    CalculateFilters(m_hor_filters, m_dst_width , m_src_width);
    CalculateFilters(m_ver_filters, m_dst_height, m_src_height);
}

// Shift range to 0..255 and clamp overflows
static unsigned shift_clamp(int x) {
    // x = (x + 2^21) >> 22;
    // But that's equivalent to this:
    x = (x + 2097152) >> 22;
    return std::clamp(x, 0, 255);
}

/**
 * Packs a TmpPixel into a single 32-bit unsigned integer.
 *
 * This function takes a TmpPixel, representing a pixel with four color components,
 * and packs it into a single 32-bit unsigned integer. The color components are
 * shifted into place and clamped to the range 0..255 to avoid overflows.
 *
 * @param pixel The TmpPixel to pack.
 * @return A 32-bit unsigned integer representing the packed pixel.
 */
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
    ++m_src_y;

    if (m_dst_y > m_dst_height) {
        esyslog("ImageScaler::NextSourceLine: Buffer overrun detected! m_dst_y (%d) > m_dst_height (%d)", m_dst_y,
                m_dst_height);
        return;  // Protect against buffer overrun
    }

    int16_t h0 {0}, h1 {0}, h2 {0}, h3 {0};  // Init outside of loop
    while (m_ver_filters[m_dst_y].m_offset == m_src_y) {
        h0 = m_ver_filters[m_dst_y].m_coeff[0];
        h1 = m_ver_filters[m_dst_y].m_coeff[1];
        h2 = m_ver_filters[m_dst_y].m_coeff[2];
        h3 = m_ver_filters[m_dst_y].m_coeff[3];
        const TmpPixel *src {m_buffer};  // Pointer to the buffer containing filtered input lines
        unsigned *dst {m_dst_image + m_dst_stride * m_dst_y};  // Pointer to the destination image line

        for (unsigned i {0}; i < m_dst_width; ++i) {
            const TmpPixel t(src[0] * h0 + src[1] * h1 + src[2] * h2 + src[3] * h3);
            src += 4;
            dst[i] = PackPixel(t);
        }

        ++m_dst_y;
    }
}
