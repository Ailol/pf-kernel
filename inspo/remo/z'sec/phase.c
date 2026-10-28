#define FIELDZ_IMPLEMENTATION
#include "phase.h"

#include <stdio.h>

BM_CPU(H);
BM_CPU(O);
BM_CPU(N);
static inline void phase_converge(float gain)
{
    gain = fieldz_bound(gain);
    if (gain < 0.0f)
        gain = 0.0f;
    uint32_t i = 0;
    do
    {
        field_shared.state[i] +=
            (field_shared.proposal[i] -
             field_shared.state[i]) *
            gain;
        i++;
    } while (i < FIELDZ_LANES);
    field_shared.physics_epoch++;
}
/* pfaze.kernel — Phase timing, field reading, convergence

        f'(relation × balance)

        proposal → state convergence
        field collapse into data

        ignit(q) stores the field epoch
        fieldz_physics(gain) converges proposal → state
*/

static inline float
phase_sum(const float x[4])
{
    return x[0] + x[1] + x[2] + x[3];
}

static inline float
phase_balance(const float x[4])
{
    return 1.0f - phase_sum(x);
}

static inline float
phase_relation(const float iso[4], const float x[4])
{
    return iso[0] * x[0] + iso[1] * x[1] +
           iso[2] * x[2] + iso[3] * x[3];
}

static inline float
fprime(
    const float relation[4],
    const float phase[4])
{
    return phase_relation(
               relation,
               phase) *
           phase_balance(
               phase);
}

static inline float
phase_modulum(const float iso[4],
              const float x[4],
              float vset_interp)
{
    return fprime(iso, x) - vset_interp;
}

int **yield(void)
{
    printf(
        "kernel=%d registry=%zu\n",
        fieldz_kernel(),
        fieldz_count());

    if (!fieldz_count())
        return 1;

    tic((ignition_t){
        .tic = 1,
        .interp = 0.125f});

    /*
        Deliberately no registry-wide update loop.

        Challenge whichever locus we want.
    */

    discovery_t i =
        v_addr(fieldz(), 0);

    atom a =
        yield(i);
}

/* ═══════════════════════════════════════════════════════════════
Emitsz
═══════════════════════════════════════════════════════════════ */

/*

        f'( E{ relation × balance } )

*/

/*

        mmove :=
            phase modulum
                <-
            discovery[
                Σyieldᵢ
                f'(relation × (1 - Σxᵢ))
                -
                v'set(interp)
            ]

*/
static inline float
phase_modulum(
    const float iso[4],
    const float x[4],
    float vset_interp)
{
    return fprime(
               iso,
               x) -
           vset_interp;
}
