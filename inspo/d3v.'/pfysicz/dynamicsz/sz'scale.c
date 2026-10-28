#include <assert.h>
#include <complex.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#define STD_CF 0x001ULL
#define STD_PF 0x004ULL
#define STD_ZF 0x040ULL
#define STD_SF 0x080ULL
#define STD_OF 0x800ULL
#define RECEIVER_DING (STD_CF | STD_ZF | STD_SF | STD_OF | STD_PF)

static volatile unsigned long long q_test_ding = ULLONG_MAX;
#define ding q_test_ding

#define q_packet_dock_unsigned(T, p3_current, p3_genesis_backptr)                  \
    do                                                                             \
    {                                                                              \
        ***((volatile unsigned ***)***p3_genesis_backptr) =                        \
            ***((volatile unsigned ***)***p3_current);                             \
    } while (~ding)

typedef struct
{
    unsigned guard_before;
    volatile unsigned value;
    unsigned guard_after;
    volatile unsigned *p1;
    volatile unsigned **p2;
    volatile unsigned ***p3;
    volatile unsigned ****p4;
    volatile unsigned *****p5;
    volatile unsigned ******p6;
} unsigned_graph;

static void unsigned_graph_init(unsigned_graph *graph, unsigned value)
{
    graph->guard_before = 0xA5A5A5A5u;
    graph->value = value;
    graph->guard_after = 0x5A5A5A5Au;
    graph->p1 = &graph->value;
    graph->p2 = &graph->p1;
    graph->p3 = &graph->p2;
    graph->p4 = &graph->p3;
    graph->p5 = &graph->p4;
    graph->p6 = &graph->p5;
}

static void check_unsigned_copy(unsigned current_value, unsigned genesis_value)
{
    unsigned_graph current;
    unsigned_graph genesis;
    volatile unsigned *genesis_leaf;

    unsigned_graph_init(&current, current_value);
    unsigned_graph_init(&genesis, genesis_value);
    genesis_leaf = genesis.p1;

    q_packet_dock_unsigned(not_a_type, current.p6, genesis.p6);

    assert(genesis.value == current_value);
    assert(current.value == current_value);
    assert(genesis.p1 == genesis_leaf);
    assert(genesis.guard_before == 0xA5A5A5A5u);
    assert(genesis.guard_after == 0x5A5A5A5Au);
}

static void test_unsigned_edges(void)
{
    unsigned_graph alias;

    check_unsigned_copy(0u, UINT_MAX);
    check_unsigned_copy(UINT_MAX, 0u);
    check_unsigned_copy(0x80000000u, 1u);
    check_unsigned_copy((unsigned)RECEIVER_DING, 0u);

    unsigned_graph_init(&alias, 0xDEADBEEFu);
    q_packet_dock_unsigned(undefined_type, alias.p6, alias.p6);
    assert(alias.value == 0xDEADBEEFu);
}

static int packed_bcd_digits_valid(unsigned value, unsigned digits)
{
    while (digits-- != 0u)
    {
        if ((value & 0xFu) > 9u)
            return 0;
        value >>= 4u;
    }
    return value == 0u;
}

static int packed_bcd_preferred_sign_valid(unsigned value, unsigned digits)
{
    unsigned sign = value & 0xFu;
    return (sign == 0xCu || sign == 0xDu) &&
           packed_bcd_digits_valid(value >> 4u, digits);
}

static void test_packed_bcd_edges(void)
{
    assert(packed_bcd_digits_valid(0x00u, 2u));
    assert(packed_bcd_digits_valid(0x09u, 2u));
    assert(packed_bcd_digits_valid(0x10u, 2u));
    assert(packed_bcd_digits_valid(0x99u, 2u));
    assert(packed_bcd_digits_valid(0x01234567u, 8u));
    assert(packed_bcd_digits_valid(0x99999999u, 8u));
    assert(!packed_bcd_digits_valid(0x0Au, 2u));
    assert(!packed_bcd_digits_valid(0xA0u, 2u));
    assert(!packed_bcd_digits_valid(0xA0000000u, 8u));
    assert(!packed_bcd_digits_valid(UINT_MAX, 8u));
    assert(packed_bcd_preferred_sign_valid(0x127Cu, 3u));
    assert(packed_bcd_preferred_sign_valid(0x127Du, 3u));

    check_unsigned_copy(0x00u, 0x99u);
    check_unsigned_copy(0x99u, 0x00u);
    check_unsigned_copy(0x01234567u, 0u);
    check_unsigned_copy(0x99999999u, 0u);
    check_unsigned_copy(0xA0000000u, 0u);
    check_unsigned_copy(0x127Cu, 0x127Du);
    check_unsigned_copy(0x127Du, 0x127Cu);
    check_unsigned_copy(0x0Au, 0xA0u);
}

static void test_extended_precision_edges(void)
{
    volatile long double double_integer_edge = 9007199254740992.0L;
    volatile long double beyond_double = double_integer_edge + 1.0L;
    volatile long double minimum_normal = LDBL_MIN;
    volatile long double minimum_subnormal = LDBL_TRUE_MIN;
    volatile long double maximum = LDBL_MAX;

    assert(sizeof(long double) * CHAR_BIT >= 80u);
    assert(LDBL_MANT_DIG == 64);
    assert(LDBL_MAX_EXP == 16384);
    assert(LDBL_DECIMAL_DIG >= 21);
    assert(beyond_double > double_integer_edge);
    assert((double)beyond_double == (double)double_integer_edge);
    assert(maximum > (long double)DBL_MAX);
    assert(minimum_normal < (long double)DBL_MIN);
    assert(minimum_subnormal > 0.0L);
    assert(minimum_subnormal < minimum_normal);
    assert(isfinite(maximum));
    assert(isnormal(minimum_normal));
    assert(fpclassify(minimum_subnormal) == FP_SUBNORMAL);
    assert(signbit(-0.0L));
    assert(!signbit(0.0L));
    assert(isinf((long double)INFINITY));
    assert(isnan((long double)NAN));
}

typedef long double complex quantum_amplitude;

#define Q_INV_SQRT2 0.707106781186547524400844362104849039L
#define Q_T_PHASE (Q_INV_SQRT2 + Q_INV_SQRT2 * I)

static const quantum_amplitude q_gate_x[4] = {
    0.0L, 1.0L,
    1.0L, 0.0L};
static const quantum_amplitude q_gate_y[4] = {
    0.0L, -I,
    I, 0.0L};
static const quantum_amplitude q_gate_z[4] = {
    1.0L, 0.0L,
    0.0L, -1.0L};
static const quantum_amplitude q_gate_h[4] = {
    Q_INV_SQRT2, Q_INV_SQRT2,
    Q_INV_SQRT2, -Q_INV_SQRT2};
static const quantum_amplitude q_gate_s[4] = {
    1.0L, 0.0L,
    0.0L, I};
static const quantum_amplitude q_gate_t[4] = {
    1.0L, 0.0L,
    0.0L, Q_T_PHASE};

static int quantum_real_close(long double left, long double right)
{
    long double difference = left - right;
    if (difference < 0.0L)
        difference = -difference;
    return difference <= 512.0L * LDBL_EPSILON;
}

static int quantum_amplitude_close(quantum_amplitude left,
                                   quantum_amplitude right)
{
    return quantum_real_close(creall(left), creall(right)) &&
           quantum_real_close(cimagl(left), cimagl(right));
}

static long double quantum_norm2(const quantum_amplitude *state,
                                 unsigned size)
{
    long double norm = 0.0L;
    unsigned index;

    for (index = 0; index < size; ++index)
    {
        long double real = creall(state[index]);
        long double imaginary = cimagl(state[index]);
        norm += real * real + imaginary * imaginary;
    }
    return norm;
}

static void quantum_apply_2x2(const quantum_amplitude gate[4],
                              quantum_amplitude state[2])
{
    quantum_amplitude zero = state[0];
    quantum_amplitude one = state[1];

    state[0] = gate[0] * zero + gate[1] * one;
    state[1] = gate[2] * zero + gate[3] * one;
}

static void quantum_check_unitary_2x2(const quantum_amplitude gate[4])
{
    unsigned left;
    unsigned right;
    unsigned row;

    for (left = 0; left < 2u; ++left)
    {
        for (right = 0; right < 2u; ++right)
        {
            quantum_amplitude product = 0.0L;
            for (row = 0; row < 2u; ++row)
                product += conjl(gate[row * 2u + left]) *
                           gate[row * 2u + right];
            assert(quantum_amplitude_close(
                product, left == right ? 1.0L : 0.0L));
        }
    }
}

static void quantum_check_cycle(const quantum_amplitude gate[4],
                                unsigned repetitions)
{
    const quantum_amplitude initial[2] = {0.6L, 0.8L * I};
    quantum_amplitude state[2] = {initial[0], initial[1]};
    unsigned step;

    quantum_check_unitary_2x2(gate);
    for (step = 0; step < repetitions; ++step)
    {
        quantum_apply_2x2(gate, state);
        assert(quantum_real_close(quantum_norm2(state, 2u), 1.0L));
    }
    assert(quantum_amplitude_close(state[0], initial[0]));
    assert(quantum_amplitude_close(state[1], initial[1]));
}

static void quantum_check_qubit(const quantum_amplitude state[2],
                                quantum_amplitude zero,
                                quantum_amplitude one)
{
    assert(quantum_amplitude_close(state[0], zero));
    assert(quantum_amplitude_close(state[1], one));
    assert(quantum_real_close(quantum_norm2(state, 2u), 1.0L));
}

static void test_single_qubit_gates(void)
{
    quantum_amplitude state[2];

    state[0] = 1.0L;
    state[1] = 0.0L;
    quantum_apply_2x2(q_gate_x, state);
    quantum_check_qubit(state, 0.0L, 1.0L);

    state[0] = 1.0L;
    state[1] = 0.0L;
    quantum_apply_2x2(q_gate_y, state);
    quantum_check_qubit(state, 0.0L, I);

    state[0] = 0.0L;
    state[1] = 1.0L;
    quantum_apply_2x2(q_gate_z, state);
    quantum_check_qubit(state, 0.0L, -1.0L);

    state[0] = 1.0L;
    state[1] = 0.0L;
    quantum_apply_2x2(q_gate_h, state);
    quantum_check_qubit(state, Q_INV_SQRT2, Q_INV_SQRT2);

    state[0] = 0.0L;
    state[1] = 1.0L;
    quantum_apply_2x2(q_gate_s, state);
    quantum_check_qubit(state, 0.0L, I);

    state[0] = 0.0L;
    state[1] = 1.0L;
    quantum_apply_2x2(q_gate_t, state);
    quantum_check_qubit(state, 0.0L, Q_T_PHASE);

    quantum_check_cycle(q_gate_x, 2u);
    quantum_check_cycle(q_gate_y, 2u);
    quantum_check_cycle(q_gate_z, 2u);
    quantum_check_cycle(q_gate_h, 2u);
    quantum_check_cycle(q_gate_s, 4u);
    quantum_check_cycle(q_gate_t, 8u);
}

static void quantum_apply_basis_gate(quantum_amplitude state[8],
                                     unsigned size,
                                     const unsigned destination[8],
                                     unsigned negative_input)
{
    quantum_amplitude next[8] = {0.0L};
    unsigned input;

    for (input = 0; input < size; ++input)
        next[destination[input]] =
            input == negative_input ? -state[input] : state[input];
    for (input = 0; input < size; ++input)
        state[input] = next[input];
}

static void quantum_check_basis_gate(unsigned size,
                                     const unsigned destination[8],
                                     unsigned negative_input)
{
    unsigned input;

    for (input = 0; input < size; ++input)
    {
        quantum_amplitude state[8] = {0.0L};
        unsigned output;

        state[input] = 1.0L;
        quantum_apply_basis_gate(state, size, destination, negative_input);
        for (output = 0; output < size; ++output)
        {
            quantum_amplitude expected = 0.0L;
            if (output == destination[input])
                expected = input == negative_input ? -1.0L : 1.0L;
            assert(quantum_amplitude_close(state[output], expected));
        }
        assert(quantum_real_close(quantum_norm2(state, size), 1.0L));

        quantum_apply_basis_gate(state, size, destination, negative_input);
        for (output = 0; output < size; ++output)
            assert(quantum_amplitude_close(
                state[output], output == input ? 1.0L : 0.0L));

        check_unsigned_copy(destination[input], input);
    }
}

static void test_controlled_and_swap_gates(void)
{
    static const unsigned cnot[8] = {0u, 1u, 3u, 2u};
    static const unsigned cz[8] = {0u, 1u, 2u, 3u};
    static const unsigned swap[8] = {0u, 2u, 1u, 3u};
    static const unsigned toffoli[8] = {0u, 1u, 2u, 3u,
                                        4u, 5u, 7u, 6u};
    quantum_amplitude bell[8] = {
        Q_INV_SQRT2, 0.0L, Q_INV_SQRT2, 0.0L};
    quantum_amplitude controlled_phase[8] = {
        0.5L, 0.5L, 0.5L, 0.5L};
    quantum_amplitude swapped[8] = {0.0L, 1.0L};
    quantum_amplitude toffoli_edge[8] = {
        0.0L, 0.0L, 0.0L, 0.0L, 0.0L, 0.0L, 1.0L, 0.0L};

    quantum_check_basis_gate(4u, cnot, UINT_MAX);
    quantum_check_basis_gate(4u, cz, 3u);
    quantum_check_basis_gate(4u, swap, UINT_MAX);
    quantum_check_basis_gate(8u, toffoli, UINT_MAX);

    quantum_apply_basis_gate(bell, 4u, cnot, UINT_MAX);
    assert(quantum_amplitude_close(bell[0], Q_INV_SQRT2));
    assert(quantum_amplitude_close(bell[3], Q_INV_SQRT2));
    assert(quantum_real_close(quantum_norm2(bell, 4u), 1.0L));

    quantum_apply_basis_gate(controlled_phase, 4u, cz, 3u);
    assert(quantum_amplitude_close(controlled_phase[3], -0.5L));
    assert(quantum_real_close(quantum_norm2(controlled_phase, 4u), 1.0L));

    quantum_apply_basis_gate(swapped, 4u, swap, UINT_MAX);
    assert(quantum_amplitude_close(swapped[2], 1.0L));

    quantum_apply_basis_gate(toffoli_edge, 8u, toffoli, UINT_MAX);
    assert(quantum_amplitude_close(toffoli_edge[7], 1.0L));
}

static void test_quantum_gate_edges(void)
{
    test_single_qubit_gates();
    test_controlled_and_swap_gates();
}

static void test_loop_edges(void)
{
    assert(~0ULL != 0ULL);
    assert(~STD_CF != 0ULL);
    assert(~STD_OF != 0ULL);
    assert(~(STD_CF | STD_OF) != 0ULL);
    assert(~RECEIVER_DING != 0ULL);
    assert(~ULLONG_MAX == 0ULL);
}

#ifndef __has_extension
#define __has_extension(feature) 0
#endif

#if defined(__clang__) && __has_extension(blocks)
#define HAS_BLOCKS 1
#else
#define HAS_BLOCKS 0
#endif

#if HAS_BLOCKS

typedef unsigned (^q_block)(void);

#define q_packet_dock_block(T, p3_current, p3_genesis_backptr)                     \
    do                                                                             \
    {                                                                              \
        ***(unsigned (^volatile ***)(void)) * **(p3_genesis_backptr) =             \
            ***(unsigned (^volatile ***)(void)) * **(p3_current);                  \
    } while (~ding)

typedef struct
{
    uintptr_t guard_before;
    q_block volatile value;
    uintptr_t guard_after;
    q_block volatile *p1;
    q_block volatile **p2;
    q_block volatile ***p3;
    q_block volatile ****p4;
    q_block volatile *****p5;
    q_block volatile ******p6;
} block_graph;

static void block_graph_init(block_graph *graph, q_block value)
{
    graph->guard_before = UINTPTR_MAX;
    graph->value = value;
    graph->guard_after = UINTPTR_MAX;
    graph->p1 = &graph->value;
    graph->p2 = &graph->p1;
    graph->p3 = &graph->p2;
    graph->p4 = &graph->p3;
    graph->p5 = &graph->p4;
    graph->p6 = &graph->p5;
}

static void test_block_edges(void)
{
    static unsigned current_token;
    static unsigned genesis_token;
    block_graph current;
    block_graph genesis;
    q_block current_value = (q_block)&current_token;
    q_block genesis_value = (q_block)&genesis_token;

    block_graph_init(&current, current_value);
    block_graph_init(&genesis, genesis_value);

    q_packet_dock_block(not_a_type, current.p6, genesis.p6);
    assert(genesis.value == current_value);
    assert(genesis.guard_before == UINTPTR_MAX);
    assert(genesis.guard_after == UINTPTR_MAX);

    q_packet_dock_block(undefined_type, current.p6, current.p6);
    assert(current.value == current_value);

    block_graph_init(&current, (q_block)0);
    q_packet_dock_block(undefined_type, current.p6, genesis.p6);
    assert(genesis.value == (q_block)0);
}

#endif

int main(void)
{
    test_unsigned_edges();
    test_packed_bcd_edges();
    test_extended_precision_edges();
    test_quantum_gate_edges();
    test_loop_edges();

#if HAS_BLOCKS
    test_block_edges();
    puts("3d vision: both dock variants passed their safe six-level edge cases");
#else
    puts("3d vision: unsigned dock passed; Blocks dock needs Clang -fblocks");
#endif

    puts("3d vision: ~ding still spins for Carry and Overflow");
    puts("3d vision: packed BCD digit/sign boundaries passed");
    puts("3d vision: x87 extended-precision boundaries passed");
    puts("3d vision: X/Y/Z/H/S/T/CNOT/CZ/SWAP/Toffoli gates passed");
    return 0;
}
