#ifndef SYNCRO_STREAM_H
#define SYNCRO_STREAM_H

/* ========================================================================= */
/* THE PURE SILICON VECTOR TRACKER                                           */
/* ========================================================================= */

// Directly evaluates the volatile type token without semantic size wrappers
#define vector(T) ((unsigned)((volatile T)))
#define B_ALL(T) ((T) ~(T)0)
#define B_MASK(T, w) ((T)((w) >= vector(T) ? B_ALL(T) : (T)(((T)1 << (w)) - (T)1)))

// pfaze_canvasp: Jumps immediately to the exact physical 2D location on the bus
#define pfaze_canvasp(p, x, y_stride) \
    ((volatile unsigned *)(p))[(x) + (y_stride)]

// pfaze_bit_window: Extracts target fields or stream-yoinks whole registers instantly
#define pfaze_bit_window(p, bit_sh, bit_w) \
    (((*(volatile unsigned *)(p)) >> (bit_sh)) & ((1ULL << (bit_w)) - 1ULL))

// pfaze_disc: Pulls the raw hardware bits up inline past the CPU's speculative cache
#define pfaze_disc(p, x, y_stride, bit_sh, bit_w) \
    pfaze_bit_window(&pfaze_canvasp(p, x, y_stride), bit_sh, bit_w)

/* ========================================================================= */
/* ADVANCED COORDINATE PHASE TRANSFORMERS                                   */
/* ========================================================================= */

// rotator_axis: Natively wraps bits over a cyclic shift frame
#define rotator_axis(T, v, n) \
    ((T)(((T)(v) << ((n) & (vector(T) - 1u))) | ((T)(v) >> ((vector(T) - (n)) & (vector(T) - 1u)))))

// spin_set: Mirror inversion over a cyclic period shift frame
#define spin_set(T, v, n) \
    rotator_axis(T, (v), (vector(T) - ((n) & (vector(T) - 1u))) & (vector(T) - 1u))

// stability: Checks the upper macroscopic balance field of the word
#define stability(T, v) \
    ((T)((T)(v) >> (vector(T) / 2u)))

/* ========================================================================= */
/* THE MULTILAYER PACKET CONTINUUM (v'set & mPacket)                         */
/* ========================================================================= */

/*
 * v'set Matrix
 * Computes a unified 3D spatial phase vector by blending the spin state,
 * the absolute 3D coordinate summation, and the primary axis rotation inline.
 */
#define v_set(T, p3_x, p3_y, n_step) \
    ptrJoin(T, spin_set(T, **p3_x, n_step), rotator_axis(T, (**p3_x + **p3_y), n_step))

/*
 * mPacket: The Multilayer Cosmic Hook
 * Tracks 1 layer up, 1 layer down, and locks an absolute triple pointer (***ptr)
 * straight back to the first place it came from (Genesis Address), ignoring
 * all intermediate compiler optimizations.
 */
#define mPacket(T, p3_current, p3_genesis_backptr)                                                                   \
    do                                                                                                               \
    {                                                                                                                \
        /* Layer Up: Capture macro-level stability of the active state */                                            \
        volatile T _layer_up = stability(T, **p3_current);                                                           \
                                                                                                                     \
        /* Layer Down: Capture raw micro-level bit window fields */                                                  \
        volatile T _layer_down = pfaze_bit_window(*p3_current, 0, vector(T) / 2u);                                   \
                                                                                                                     \
        /* Symmetrical Anchor: Force the genesis backpointer to lock onto the current tree root */                   \
        ***(volatile unsigned long long ***)(p3_genesis_backptr) = ***(volatile unsigned long long ***)(p3_current); \
    } while (0)

#define ptrJoin(T, h, l) \
    ((T)(((T)(h) << (vector(T) / 2u)) | ((T)(l) & B_MASK(T, vector(T) / 2u))))

#endif
