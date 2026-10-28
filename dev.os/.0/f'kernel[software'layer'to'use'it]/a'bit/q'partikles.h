#ifndef Q_PARTIKLES_H
#define Q_PARTIKLES_H

#include <limits.h>
#include <stdint.h>

#define B_ALL(T) ((T) ~(T)0)

/* Runic Markers: Immutable Bitweights of the Processor State */
#define STD_CF 0x001ULL /* The Spark (Carry / Sudden Saturation) */
#define STD_PF 0x004ULL /* The Balance (Parity / Byte Alignment) */
#define STD_AF 0x010ULL /* The Bridge (Auxiliary Carry / Half-Carve) */
#define STD_ZF 0x040ULL /* The Void (Zero / Absolute Null Match) */
#define STD_SF 0x080ULL /* The Horizon (Sign / Direction Change) */
#define STD_OF 0x800ULL /* The Breach (Overflow / Boundary Shatter) */

#define B_MASK(T, w) \
	((T)((w) == 0u ? (T)0 : ((w) >= vector(T) ? B_ALL(T) : (T)(B_ALL(T) >> ((vector(T) - (w)) & (vector(T) - 1u))))))

/* Bit helpers take uint8_t/uint16_t/uint32_t/uint64_t and side-effect-free arguments. */
#define vector(T) ((unsigned)(sizeof(T) * CHAR_BIT))

/* ========================================================================= */
/* monitors                                                                  */
/* ========================================================================= */
#define ding (STD_CF | STD_ZF | STD_SF | STD_OF | STD_PF)

#define timer() \
	({ unsigned int lo, hi; \
	   __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi)); \
	   ((uint64_t)hi << 32) | lo; })

/* ========================================================================= */
/*  partikles relevant to earth                                              */
/* ========================================================================= */
#define rotator_axis(T, v, n) \
	((T)(((uintmax_t)(T)(v) << ((n) & (vector(T) - 1u))) | ((uintmax_t)(T)(v) >> ((vector(T) - ((n) & (vector(T) - 1u))) & (vector(T) - 1u)))))

#define spin_set(T, v, n) \
	rotator_axis(T, (v), (vector(T) - ((n) & (vector(T) - 1u))) & (vector(T) - 1u))

#define stability(T, v) \
	((T)((T)(v) >> (vector(T) / 2u)))

#define v_set(T, h, l) \
	((T)(((uintmax_t)(T)(h) << (vector(T) / 2u)) | ((T)(l) & B_MASK(T, vector(T) / 2u))))

/* ========================================================================= */
/* THE Qt'b packet                                                           */
/* ========================================================================= */
#define b_packet(T, p3_x, p3_y, n_step) \
	v_set(T, spin_set(T, **(p3_x), n_step), rotator_axis(T, ((uintmax_t)(T)**(p3_x) + (uintmax_t)(T)**(p3_y)), n_step))

/* ========================================================================= */
/* THE q'partiklesz                                                          */
/* ========================================================================= */
/* Valid T*** chains: reconnect the genesis T* to the current T* without copying data.
 * This is a one-shot, non-atomic operation; ding is a mask, not live CPU flags.
 */
#define q_packet_dock(T, p3_current, p3_genesis_backptr) \
	do                                                \
	{                                                 \
		T *q_partikles_dock_root_ = **(p3_current);     \
		**(p3_genesis_backptr) = q_partikles_dock_root_; \
	} while (0)

#endif /* Q_PARTIKLES_H */
