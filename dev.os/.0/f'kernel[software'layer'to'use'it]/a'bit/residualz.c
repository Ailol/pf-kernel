#include <math.h>
#include "q'partikles.h"
void q_packet_adapt(float state[64], const float gain[64], const float suggestion[64], uint64_t iterations) { while (iterations-- != 0u) for (unsigned lane = 0; lane < 64u; ++lane) state[lane] = fmaf(state[lane], gain[lane], suggestion[lane]); }
