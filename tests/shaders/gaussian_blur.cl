// Simple Gaussian blur kernels (grayscale and RGBA)
// Usage:
//  - Set global size to (width, height)
//  - Provide radius (e.g., 3) and sigma (e.g., 1.0f to 2.0f)

inline float gaussian(float x, float sigma) {
    return exp(-(x * x) / (2.0f * sigma * sigma));
}

inline int clampi(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// Grayscale image: in/out are length width*height, one float per pixel
__kernel void gaussian_blur(__global const float* in,
                                 __global float* out,
                                 const int width,
                                 const int height,
                                 const int radius,
                                 const float sigma) {
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    if (x >= width || y >= height) return;

    float accum = 0.0f;
    float wsum = 0.0f;

    for (int dy = -radius; dy <= radius; ++dy) {
        const int yy = clampi(y + dy, 0, height - 1);
        const float wy = gaussian((float)dy, sigma);
        for (int dx = -radius; dx <= radius; ++dx) {
            const int xx = clampi(x + dx, 0, width - 1);
            const float wx = gaussian((float)dx, sigma);
            const float w = wx * wy;
            accum += in[yy * width + xx] * w;
            wsum += w;
        }
    }

    out[y * width + x] = (wsum > 0.0f) ? (accum / wsum) : in[y * width + x];
}

