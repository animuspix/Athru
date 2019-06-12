// Four-wide vector hash, modified from Nimitz's WebGL2 Quality Hashes Collection
// (https://www.shadertoy.com/view/Xt3cDn)
// Named as a mix between the basis hash (XXHash32-derived, https://github.com/Cyan4973/xxHash)
// and the output transformation (MINSTD, http://random.mat.sbg.ac.at/results/karl/server/node4.html)
uint4 xxminstd32(uint i)
{
    const uint PRIME32_2 = 2246822519U, PRIME32_3 = 3266489917U;
	const uint PRIME32_4 = 668265263U, PRIME32_5 = 374761393U;
	uint h32 = i + PRIME32_5;
	h32 = PRIME32_4 * ((h32 << 17) | (h32 >> (32 - 17))); //Initial testing suggests this line could be omitted for extra perf
    h32 = PRIME32_2 * (h32 ^ (h32 >> 15));
    h32 = PRIME32_3 * (h32 ^ (h32 >> 13));
    h32 ^= (h32 >> 16);
    return uint4(h32, h32 * 16807U, h32 * 48271U, h32 * 69621U); //see: http://random.mat.sbg.ac.at/results/karl/server/node4.html
}

// Lightly modified integer hash from Thomas Mueller, found here:
// https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key/12996028#12996028
uint tmhash(uint x)
{
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    return (x >> 16) ^ x;
}

// Modified integer hash from iq, found in Nimitz's WebGL2 Quality Hashes Collection
// (https://www.shadertoy.com/view/Xt3cDn)
// Original from:
// https://www.shadertoy.com/view/4tXyWN
uint ihashIII(uint i)
{
    i = 1103515245U * ((i >> 1U) ^ i);
    i = 1103515245U * (i ^ (i >> 3U));
    return i ^ (i >> 16);
}

// Short initializer for Philox streams
void strmBuilder(uint ndx, out uint4 ctr, out uint2 key)
{
    ctr = xxminstd32(ndx); // Seed state by hashing indices with xxminstd32 (see above)
    key = uint2(ihashIII(ndx), tmhash(ndx)); // Partially seed keys with a different hash (not Wang or xxminstd32)
}