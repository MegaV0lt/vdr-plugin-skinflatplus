
#include "imagescaler.h"

#include <cstdlib>
#include <cmath>

ImageScaler::ImageScaler() :
    m_memory(NULL),
    m_hor_filters(NULL),
    m_ver_filters(NULL),
    m_buffer(NULL),
    m_dst_image(NULL),
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
    if (m_memory) free(m_memory);
}

// sin(x)/(x)
static float sincf(float x) {
    if (fabsf(x) < 0.05f) return 1.0f - (1.0f/6.0f)*x*x;  // taylor series approximation to avoid 0/0
    return sin(x)/x;
}

static void CalculateFilters(ImageScaler::Filter *filters, int dst_size, int src_size) {
    const float fc = dst_size >= src_size ? 1.0f : ((float) dst_size)/((float) src_size);

    for (int i = 0; i < dst_size; ++i) {
        const int    d          = 2 * dst_size;                       // sample position denominator
        const int    e          = (2 * i + 1) * src_size - dst_size;  // sample position enumerator
        int          offset     =  e / d;                             // truncated sample position
        const float sub_offset =
            ((float)(e - offset * d)) / ((float)d);  // exact sample position is (float) e/d = offset + sub_offset

        // calculate filter coefficients
        float  h[4];
        for (int j = 0; j < 4; ++j) {
            const float t = 3.14159265359f * (sub_offset + (1 - j));
            h[j] = sincf(fc * t) * cosf(0.25f * t);  // sinc-lowpass and cos-window
        }

        // ensure that filter does not reach out off image bounds:
        while (offset < 1) {
            h[0] += h[1];
            h[1] = h[2];
            h[2] = h[3];
            h[3] = 0.0f;
            ++offset;
        }

        while (offset+3 > src_size) {
            h[3] += h[2];
            h[2] = h[1];
            h[1] = h[0];
            h[0] = 0.0f;
            --offset;
        }

        // coefficients are normalized to sum up to 2048
        const float  norm = 2048.0f / (h[0] + h[1] + h[2] + h[3]);

        --offset;  // offset of fist used pixel

        filters[i].m_offset = offset + 4;  // store offset of first unused pixel

        for (int j = 0; j < 4; ++j) {
            const float t = norm * h[j];
            filters[i].m_coeff[(offset + j) & 3] =
                static_cast<int>((t > 0.0f) ? (t + 0.5f) : (t - 0.5f));  // consider ring buffer index permutations
        }
    }

    // set end marker
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

    // if image dimensions do not change we can keep the old filter coefficients
    if ((src_width == m_src_width) && (src_height == m_src_height) && (dst_width == m_dst_width) &&
        (dst_height == m_dst_height))
        return;

    m_dst_width  = dst_width;
    m_dst_height = dst_height;
    m_src_width  = src_width;
    m_src_height = src_height;

    if (m_memory) free(m_memory);

    const unsigned hor_filters_size = (m_dst_width + 1) * sizeof(Filter);  // reserve one extra position for end marker
    const unsigned ver_filters_size = (m_dst_height + 1) * sizeof(Filter);
    const unsigned buffer_size = 4 * m_dst_width * sizeof(TmpPixel);

    char *p = (char *) malloc(hor_filters_size + ver_filters_size + buffer_size);

    m_memory = p;

    m_hor_filters = (Filter   *) p;  p += hor_filters_size;
    m_ver_filters = (Filter   *) p;  p += ver_filters_size;
    m_buffer      = (TmpPixel *) p;

    CalculateFilters(m_hor_filters, m_dst_width , m_src_width);
    CalculateFilters(m_ver_filters, m_dst_height, m_src_height);
}

// shift range to 0..255 and clamp overflows
static unsigned shift_clamp(int x) {
    x = (x + (1 << 21)) >> 22;
    if (x < 0)
        return 0;
    if (x > 255)
        return 255;
    return x;
}

void ImageScaler::NextSourceLine() {
    m_dst_x = 0;
    m_src_x = 0;
    m_src_y++;

    while (m_ver_filters[m_dst_y].m_offset == m_src_y) {
        const int        h0  = m_ver_filters[m_dst_y].m_coeff[0];
        const int        h1  = m_ver_filters[m_dst_y].m_coeff[1];
        const int        h2  = m_ver_filters[m_dst_y].m_coeff[2];
        const int        h3  = m_ver_filters[m_dst_y].m_coeff[3];
        const TmpPixel  *src = m_buffer;
        unsigned        *dst = m_dst_image + m_dst_stride * m_dst_y;

        for (unsigned i = 0; i < m_dst_width; ++i) {
            const ImageScaler::TmpPixel t(src[0] * h0 + src[1] * h1 + src[2] * h2 + src[3] * h3);
            src += 4;
            dst[i] = shift_clamp(t[0]) | (shift_clamp(t[1]) << 8) | (shift_clamp(t[2]) << 16)
                     | (shift_clamp(t[3]) << 24);
        }

        m_dst_y++;
    }
}
