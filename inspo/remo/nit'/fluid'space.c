//
// # THE IMPERIAL GRID
// ## #Non -'the well'abric &i'perial Phase Matrix
//
//
//       [ THE IMPERIAL System ]                      [ f'set(earth)  ]
//
// p3_genesis_anchor active_matrix_p3
//          │                                            │
//          ▼                                            ▼
//    ┌───────────┐                                ┌───────────┐
//    │ Level 2  │                                │ Level 2  │
//    └─────┬─────┘                                └─────┬─────┘
//          ▼                                            ▼
//    ┌───────────┐                                ┌───────────┐
//    │ Level 1  │                                │ Level 1  │
//    └─────┬─────┘                                └─────┬─────┘
//          ▼                                            ▼
//    ┌───────────┐ ◄────────────────────────────┐ ┌───────────┐
//    │ NULL      │ Direct "Velcro" Snap         │ │ 3 Stone   │ (Raw Elastic Block)
//    │ (Void)    │ (1 - Cycle Root Swap)        └─┤ 2 Clicks  │ Memory Canvas Grid
//    └───────────┘                                └───────────┘ [X] + [Y_Stride]
//

#ifndef IMPERIAL_GRID_H
#define IMPERIAL_GRID_H

/* ========================================================================= */
/* 1. FLUID CAPACITY CARRIERS (NON-MEASURED VECTOR DEPTH)                    */
/* ========================================================================= */
#define vector(T) ((unsigned)(sizeof(volatile T) * 8u))
#define B_ALL(T) ((T) ~(T)0)
#define B_MASK(T, w) ((T)((w) >= vector(T) ? B_ALL(T) : (T)(((T)1 << (w)) - (T)1)))

/* ========================================================================= */
/* 2. THE 2D CANVAS GATEWAYS (SLICING WITHOUT MEASURING)                     */
/* ========================================================================= */
#define pfaze_canvasp(p, x, y_stride) \
    ((volatile unsigned *)(p))[(x) + (y_stride)]

#define pfaze_bit_window(p, bit_sh, bit_w) \
    (((*(volatile unsigned *)(p)) >> (bit_sh)) & ((1ULL << (bit_w)) - 1ULL))

#define pfaze_disc(p, x, y_stride, bit_sh, bit_w) \
    pfaze_bit_window(&pfaze_canvasp(p, x, y_stride), bit_sh, bit_w)

/* ========================================================================= */
/* 3. COORDINATE FLUID TRANSFORMERS (CLICKS & AXIS SPINS)                    */
/* ========================================================================= */
#define rotator_axis(T, v, n) \
    ((T)(((T)(v) << ((n) & (vector(T) - 1u))) | ((T)(v) >> ((vector(T) - (n)) & (vector(T) - 1u)))))

#define spin_set(T, v, n) \
    rotator_axis(T, (v), (vector(T) - ((n) & (vector(T) - 1u))) & (vector(T) - 1u))

#define stability(T, v) \
    ((T)((T)(v) >> (vector(T) / 2u)))

#define ptrJoin(T, h, l) \
    ((T)(((T)(h) << (vector(T) / 2u)) | ((T)(l) & B_MASK(T, vector(T) / 2u))))

/* ========================================================================= */
/* 4. THE f'set of location(where the generator is)                     */
/* ========================================================================= */
#define v_set(T, p3_x, p3_y, n_step) \
    ptrJoin(T, spin_set(T, **p3_x, n_step), rotator_axis(T, (**p3_x + **p3_y), n_step))

#define mPacket(T, p3_current, p3_genesis_backptr)                                                                   \
    do                                                                                                               \
    {                                                                                                                \
        volatile T _layer_up = stability(T, **p3_current);                                                           \
        volatile T _layer_down = pfaze_bit_window(*p3_current, 0, vector(T) / 2u);                                   \
        ***(volatile unsigned long long ***)(p3_genesis_backptr) = ***(volatile unsigned long long ***)(p3_current); \
    } while (0)

/* ========================================================================= */
/* 4. THE  q'set of infinite continuum (where the loop catches)              */
/* ========================================================================= */
#define q_set(T, p3_x, p3_y, n_step) \
    ptrJoin(T, spin_set(T, **p3_x, n_step), rotator_axis(T, (**p3_x + **p3_y), n_step))

#define mPacket(T, p3_current, p3_genesis_backptr)                                               \
    do                                                                                           \
    {                                                                                            \
        volatile T _layer_up = stability(T, **p3_current);                                       \
        volatile T _layer_down = pfaze_bit_window(*p3_current, 0, vector(T) / 2u);               \
        ***(volatile unsigned ***)(p3_genesis_backptr) = ***(volatile unsigned ***)(p3_current); \
    } while (0)

#endif
