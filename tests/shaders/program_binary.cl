__kernel void transform(__global const uint* input, __global uint* output) {
    const size_t i = get_global_id(0);
    output[i] = input[i] * FACTOR + 17u;
}
