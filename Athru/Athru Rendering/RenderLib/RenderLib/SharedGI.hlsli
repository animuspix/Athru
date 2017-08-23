
// A write-only buffer containing parallel primary contributions (not really possible
// to simultaneously increment a single GI vector across multiple parallel threads)
// for each light path defined under "traceables"
RWBuffer<float4> giCalcBufWritable : register(u4);

// A read-only buffer containing parallel primary contributions (not really possible
// to simultaneously increment a single GI vector across multiple parallel threads)
// for each light path defined under "traceables"
Buffer<float4> giCalcBufReadable : register(t1);

// How many samples to take for each traceable point during
// global illumination
#define GI_SAMPLES_PER_RAY 256

// The total number of primary samples processed in each frame
#define GI_SAMPLE_TOTAL ((DISPLAY_WIDTH * 64) * GI_SAMPLES_PER_RAY)