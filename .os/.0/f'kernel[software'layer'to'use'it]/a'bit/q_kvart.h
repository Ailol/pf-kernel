#ifndef Q_KVART_H
#define Q_KVART_H

#include <assert.h>
#include <math.h>
#include <stddef.h>

typedef struct
{
    float quartil[3];
    float state[3];
} q_kvart;

/* Zero-initialize q_kvart. Vectors contain dimensions components; colors are RGB.
 * kvartil is displacement per step. All input/output arrays must be disjoint.
 * retention controls memory; response controls visible fading/regeneration.
 */
static inline void q_kvart_step(
    q_kvart *q, float *position, const float *kvartil,
    const float *emission_direction, size_t dimensions,
    const float emission[3], float retention, float threshold, float response)
{
    double dot = 0.0, movement_squared = 0.0, emission_squared = 0.0;
    float match = 0.0f;

    assert(q && position && kvartil && emission_direction && emission);
    assert(dimensions > 0);
    assert(retention >= 0.0f && retention < 1.0f);
    assert(threshold > 0.0f && threshold < 1.0f);
    assert(response > 0.0f && response <= 1.0f);

    for (size_t axis = 0; axis < dimensions; ++axis)
    {
        assert(isfinite(position[axis]) && isfinite(kvartil[axis]));
        assert(isfinite(emission_direction[axis]));
        dot += (double)kvartil[axis] * emission_direction[axis];
        movement_squared += (double)kvartil[axis] * kvartil[axis];
        emission_squared += (double)emission_direction[axis] * emission_direction[axis];
    }
    if (movement_squared > 0.0 && emission_squared > 0.0)
        match = (float)fmin(1.0, fmax(0.0,
            dot / (sqrt(movement_squared) * sqrt(emission_squared))));

    for (size_t channel = 0; channel < 3; ++channel)
    {
        assert(emission[channel] >= 0.0f && emission[channel] <= 1.0f);
        assert(q->quartil[channel] >= 0.0f && q->quartil[channel] <= 1.0f);
        assert(q->state[channel] >= 0.0f && q->state[channel] <= 1.0f);
        q->quartil[channel] = fminf(1.0f,
            retention * q->quartil[channel] + match * emission[channel]);
        float visible = fmaxf(0.0f,
            (q->quartil[channel] - threshold) / (1.0f - threshold));
        q->state[channel] = (1.0f - response) * q->state[channel] + response * visible;
    }
    for (size_t axis = 0; axis < dimensions; ++axis)
        position[axis] += kvartil[axis];
}

#endif
