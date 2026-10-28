#include <stddef.h>
#include <stdint.h>

enum
{
    PFAZE_QUANTUM = 1u << 0,
    PFAZE_GRABBING = 1u << 1,
    PFAZE_STABLE = 1u << 2,
    PFAZE_JOINED = 1u << 3,
    PFAZE_F_ATOM = 1u << 4,
    PFAZE_PF_ATOM = 1u << 5,
    PFAZE_PH_ATOM = 1u << 6,
    PFAZE_LEGEND = 1u << 7
};

enum
{
    PFAZE_CF = 0x0001u,
    PFAZE_PF = 0x0004u,
    PFAZE_ZF = 0x0040u,
    PFAZE_SF = 0x0080u,
    PFAZE_OF = 0x0800u,
    PFAZE_CHANGED = 1u << 16,
    PFAZE_WAITING = 1u << 17,
    PFAZE_FAULT = 1u << 18
};

typedef enum
{
    PFAZE_OK,
    PFAZE_WAIT,
    PFAZE_DONE,
    PFAZE_STARVED,
    PFAZE_BAD_POINTER,
    PFAZE_BAD_STATE,
    PFAZE_BAD_INTENT,
    PFAZE_ENERGY_OVERFLOW,
    PFAZE_ENERGY_FAULT
} pfaze_result;

typedef struct
{
    int16_t x;
    int16_t y;
} pfaze_point;

typedef struct
{
    pfaze_point quantum;
    pfaze_point particle;
    uint32_t reach;
    uint32_t distance;
    uint32_t state;
    uint32_t history;
    uint32_t flags;
    uint32_t intent;
    uint64_t kinetic;
    uint64_t internal;
    uint64_t epoch;
    uint64_t hardware_flags;
} pfaze_cell;

typedef void (*pfaze_observer)(
    const pfaze_cell *before,
    const pfaze_cell *after,
    void *context);

typedef void (*pfaze_trap)(pfaze_result fault, void *context);

typedef struct
{
    pfaze_observer observe;
    pfaze_trap trap;
    void *context;
    uint32_t max_steps;
} pfaze_kernel;

typedef struct
{
    uint64_t value;
    uint32_t flags;
} pfaze_q;

static uint64_t pfaze_hardware_flags(void)
{
#if defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))
    uint64_t flags;
    __asm__ volatile("pushfq; popq %0" : "=r"(flags) : : "memory");
    return flags;
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
    uint32_t flags;
    __asm__ volatile("pushfl; popl %0" : "=r"(flags) : : "memory");
    return flags;
#else
    return 0;
#endif
}

static uint32_t pfaze_parity(uint8_t value)
{
    uint32_t odd = 0;
    uint32_t bit;

    for (bit = 0; bit < 8; ++bit)
    {
        odd ^= value & 1u;
        value >>= 1;
    }
    return odd == 0 ? PFAZE_PF : 0;
}

static pfaze_q pfaze_cmp_u32(uint32_t x, uint32_t y)
{
    uint32_t result = x - y;
    uint32_t flags = pfaze_parity((uint8_t)result);

    if (x < y)
        flags |= PFAZE_CF;
    if (result == 0)
        flags |= PFAZE_ZF;
    if ((result & UINT32_C(0x80000000)) != 0)
        flags |= PFAZE_SF;
    if (((x ^ y) & (x ^ result) & UINT32_C(0x80000000)) != 0)
        flags |= PFAZE_OF;
    return (pfaze_q){result, flags};
}

static pfaze_q pfaze_cmp_u64(uint64_t x, uint64_t y)
{
    uint64_t result = x - y;
    uint32_t flags = pfaze_parity((uint8_t)result);

    if (x < y)
        flags |= PFAZE_CF;
    if (result == 0)
        flags |= PFAZE_ZF;
    if ((result & (UINT64_C(1) << 63)) != 0)
        flags |= PFAZE_SF;
    if (((x ^ y) & (x ^ result) & (UINT64_C(1) << 63)) != 0)
        flags |= PFAZE_OF;
    return (pfaze_q){result, flags};
}

static int pfaze_ja(uint32_t flags)
{
    return (flags & (PFAZE_CF | PFAZE_ZF)) == 0;
}

static int pfaze_jae(uint32_t flags)
{
    return (flags & PFAZE_CF) == 0;
}

static int pfaze_jb(uint32_t flags)
{
    return (flags & PFAZE_CF) != 0;
}

static int pfaze_jbe(uint32_t flags)
{
    return (flags & (PFAZE_CF | PFAZE_ZF)) != 0;
}

static pfaze_q pfaze_xor_u32(uint32_t x, uint32_t y)
{
    uint32_t result = x ^ y;
    uint32_t flags = pfaze_parity((uint8_t)result);

    if (result == 0)
        flags |= PFAZE_ZF;
    if ((result & UINT32_C(0x80000000)) != 0)
        flags |= PFAZE_SF;
    return (pfaze_q){result, flags};
}

static void pfaze_xchg_u64(volatile uint64_t *cell, uint64_t *value)
{
    uint64_t temporary = *cell;

    *cell = *value;
    *value = temporary;
}

static uint32_t pfaze_xlat(uint32_t state)
{
    static const uint8_t table[16] = {
        [PFAZE_QUANTUM] = PFAZE_GRABBING,
        [PFAZE_GRABBING] = PFAZE_STABLE,
        [PFAZE_STABLE] = PFAZE_JOINED
    };

    if (state >= sizeof(table))
        return 0;
    return table[state];
}

static uint32_t pfaze_magnitude(int32_t value)
{
    if (value >= 0)
        return (uint32_t)value;
    return (uint32_t)(-(value + 1)) + 1u;
}

static uint64_t pfaze_product(uint32_t a, uint32_t b)
{
    uint64_t result = 0;
    uint64_t shifted = a;

    while (b != 0)
    {
        if ((b & 1u) != 0)
            result += shifted;
        shifted <<= 1;
        b >>= 1;
    }
    return result;
}

static uint32_t pfaze_sqrt(uint64_t value)
{
    uint64_t remainder = value;
    uint64_t root = 0;
    uint64_t bit = UINT64_C(1) << 62;

    while (bit > remainder)
        bit >>= 2;

    while (bit != 0)
    {
        if (remainder >= root + bit)
        {
            remainder -= root + bit;
            root = (root >> 1) + bit;
        }
        else
        {
            root >>= 1;
        }
        bit >>= 2;
    }
    return (uint32_t)root;
}

static uint32_t pfaze_distance(pfaze_point a, pfaze_point b)
{
    uint32_t dx = pfaze_magnitude((int32_t)b.x - a.x);
    uint32_t dy = pfaze_magnitude((int32_t)b.y - a.y);

    return pfaze_sqrt(
        pfaze_product(dx, dx) +
        pfaze_product(dy, dy));
}

static int pfaze_final(uint32_t state)
{
    const uint32_t final_states =
        PFAZE_F_ATOM |
        PFAZE_PF_ATOM |
        PFAZE_PH_ATOM |
        PFAZE_LEGEND;

    return (state & final_states) != 0 &&
           (state & ~final_states) == 0 &&
           (state & (state - 1u)) == 0;
}

static pfaze_result pfaze_fail(
    const pfaze_kernel *kernel,
    volatile pfaze_cell *cell,
    pfaze_result fault)
{
    if (cell)
        cell->flags |= PFAZE_FAULT;
    if (kernel && kernel->trap)
        kernel->trap(fault, kernel->context);
    return fault;
}

pfaze_result pfaze_tick(
    const pfaze_kernel *kernel,
    volatile pfaze_cell ***space)
{
    volatile pfaze_cell *cell;
    pfaze_cell before;
    pfaze_cell after;
    uint64_t energy;
    uint64_t incoming;
    pfaze_q q;

    if (!kernel || !space || !*space || !**space)
        return pfaze_fail(kernel, NULL, PFAZE_BAD_POINTER);

    cell = **space;
    before = *cell;
    cell->hardware_flags = pfaze_hardware_flags();
    cell->flags = 0;

    switch (cell->state)
    {
    case PFAZE_QUANTUM:
        cell->distance = pfaze_distance(cell->quantum, cell->particle);
        q = pfaze_cmp_u32(cell->distance, cell->reach);
        cell->flags = q.flags;
        if (pfaze_ja(cell->flags))
        {
            cell->flags |= PFAZE_WAITING;
            after = *cell;
            if (kernel->observe)
                kernel->observe(&before, &after, kernel->context);
            return PFAZE_WAIT;
        }
        if (!pfaze_jbe(cell->flags))
            return pfaze_fail(kernel, cell, PFAZE_BAD_STATE);
        cell->state = pfaze_xlat(cell->state);
        break;

    case PFAZE_GRABBING:
        q = pfaze_cmp_u64(
            UINT64_MAX - cell->internal,
            cell->kinetic);
        cell->flags = q.flags;
        if (pfaze_jb(cell->flags))
            return pfaze_fail(kernel, cell, PFAZE_ENERGY_OVERFLOW);
        if (!pfaze_jae(cell->flags))
            return pfaze_fail(kernel, cell, PFAZE_BAD_STATE);
        energy = cell->kinetic + cell->internal;
        incoming = 0;
        pfaze_xchg_u64(&cell->kinetic, &incoming);
        cell->internal += incoming;
        if (cell->internal != energy)
            return pfaze_fail(kernel, cell, PFAZE_ENERGY_FAULT);
        cell->state = pfaze_xlat(cell->state);
        break;

    case PFAZE_STABLE:
        cell->state = pfaze_xlat(cell->state);
        break;

    case PFAZE_JOINED:
        if (!pfaze_final(cell->intent))
            return pfaze_fail(kernel, cell, PFAZE_BAD_INTENT);
        cell->state = cell->intent;
        break;

    default:
        if (pfaze_final(cell->state))
            return PFAZE_DONE;
        return pfaze_fail(kernel, cell, PFAZE_BAD_STATE);
    }

    cell->history |= cell->state;
    q = pfaze_xor_u32(before.state, cell->state);
    cell->flags = q.flags;
    if ((q.flags & PFAZE_ZF) == 0)
        cell->flags |= PFAZE_CHANGED;
    cell->epoch++;
    after = *cell;
    if (kernel->observe)
        kernel->observe(&before, &after, kernel->context);
    return PFAZE_OK;
}

pfaze_result pfaze_run(
    const pfaze_kernel *kernel,
    volatile pfaze_cell ***space)
{
    pfaze_result result;
    uint32_t step;

    if (!kernel || kernel->max_steps == 0)
        return pfaze_fail(kernel, NULL, PFAZE_STARVED);

    for (step = 0; step < kernel->max_steps; ++step)
    {
        result = pfaze_tick(kernel, space);
        if (result != PFAZE_OK)
            return result;
    }

    return pfaze_fail(kernel, space && *space ? **space : NULL, PFAZE_STARVED);
}

#if defined(PFAZE_WASM)

#define PFAZE_WASM_EXPORT(name) __attribute__((export_name(name)))

static volatile pfaze_cell pfaze_wasm_cell;

static volatile pfaze_cell ***pfaze_wasm_space(
    volatile pfaze_cell **locus,
    volatile pfaze_cell ***binding)
{
    *locus = &pfaze_wasm_cell;
    *binding = locus;
    return binding;
}

PFAZE_WASM_EXPORT("pfaze_reset")
void pfaze_wasm_reset(
    int32_t quantum_x,
    int32_t quantum_y,
    int32_t particle_x,
    int32_t particle_y,
    uint32_t reach,
    uint32_t intent,
    uint64_t kinetic,
    uint64_t internal)
{
    pfaze_wasm_cell = (pfaze_cell){
        .quantum = {(int16_t)quantum_x, (int16_t)quantum_y},
        .particle = {(int16_t)particle_x, (int16_t)particle_y},
        .reach = reach,
        .state = PFAZE_QUANTUM,
        .history = PFAZE_QUANTUM,
        .intent = intent,
        .kinetic = kinetic,
        .internal = internal
    };
}

PFAZE_WASM_EXPORT("pfaze_tick")
uint32_t pfaze_wasm_tick(void)
{
    pfaze_kernel kernel = {0};
    volatile pfaze_cell *locus;
    volatile pfaze_cell **binding;

    return (uint32_t)pfaze_tick(
        &kernel,
        pfaze_wasm_space(&locus, &binding));
}

PFAZE_WASM_EXPORT("pfaze_run")
uint32_t pfaze_wasm_run(uint32_t max_steps)
{
    pfaze_kernel kernel = {
        .max_steps = max_steps
    };
    volatile pfaze_cell *locus;
    volatile pfaze_cell **binding;

    return (uint32_t)pfaze_run(
        &kernel,
        pfaze_wasm_space(&locus, &binding));
}

PFAZE_WASM_EXPORT("pfaze_cell")
uintptr_t pfaze_wasm_cell_address(void)
{
    return (uintptr_t)&pfaze_wasm_cell;
}

#endif

#if defined(PFAZE_KERNEL_TEST)

#include <assert.h>
#include <stdio.h>

typedef struct
{
    uint32_t observations;
    uint32_t traps;
    pfaze_result fault;
} pfaze_test;

static void test_observe(
    const pfaze_cell *before,
    const pfaze_cell *after,
    void *context)
{
    pfaze_test *test = context;

    if (before->state != after->state ||
        before->kinetic != after->kinetic ||
        before->internal != after->internal)
        assert((after->flags & PFAZE_CHANGED) != 0);
    test->observations++;
}

static void test_trap(pfaze_result fault, void *context)
{
    pfaze_test *test = context;

    test->traps++;
    test->fault = fault;
}

int main(void)
{
    pfaze_test test = {0};
    pfaze_kernel kernel = {
        .observe = test_observe,
        .trap = test_trap,
        .context = &test,
        .max_steps = 8
    };
    volatile pfaze_cell cell = {
        .quantum = {-768, 1024},
        .particle = {1280, -512},
        .reach = 2560,
        .state = PFAZE_QUANTUM,
        .history = PFAZE_QUANTUM,
        .intent = PFAZE_PF_ATOM,
        .kinetic = 100,
        .internal = 12
    };
    volatile pfaze_cell *locus = &cell;
    volatile pfaze_cell **binding = &locus;
    volatile pfaze_cell ***space = &binding;
    pfaze_result result;
    pfaze_q q;
    volatile uint64_t exchange_a = 10;
    uint64_t exchange_b = 20;

    q = pfaze_cmp_u32(5, 4);
    assert(q.value == 1 && pfaze_ja(q.flags) && pfaze_jae(q.flags));
    assert(!pfaze_jb(q.flags) && !pfaze_jbe(q.flags));
    q = pfaze_cmp_u32(4, 4);
    assert(q.value == 0 && !pfaze_ja(q.flags) && pfaze_jae(q.flags));
    assert(!pfaze_jb(q.flags) && pfaze_jbe(q.flags));
    q = pfaze_cmp_u32(3, 4);
    assert(!pfaze_ja(q.flags) && !pfaze_jae(q.flags));
    assert(pfaze_jb(q.flags) && pfaze_jbe(q.flags));
    q = pfaze_xor_u32(0xAA, 0xAA);
    assert(q.value == 0);
    assert((q.flags & (PFAZE_ZF | PFAZE_PF)) == (PFAZE_ZF | PFAZE_PF));
    assert((q.flags & (PFAZE_CF | PFAZE_OF)) == 0);
    pfaze_xchg_u64(&exchange_a, &exchange_b);
    assert(exchange_a == 20 && exchange_b == 10);
    assert(pfaze_xlat(PFAZE_QUANTUM) == PFAZE_GRABBING);

    result = pfaze_run(&kernel, space);
    assert(result == PFAZE_DONE);
    assert(cell.distance == 2560);
    assert(cell.kinetic == 0 && cell.internal == 112);
    assert(cell.state == PFAZE_PF_ATOM);
    assert(cell.history == (PFAZE_QUANTUM | PFAZE_GRABBING |
                            PFAZE_STABLE | PFAZE_JOINED |
                            PFAZE_PF_ATOM));
    assert(cell.epoch == 4);
    assert(test.observations == 4 && test.traps == 0);

    cell = (pfaze_cell){
        .quantum = {0, 0},
        .particle = {1024, 0},
        .reach = 512,
        .state = PFAZE_QUANTUM,
        .history = PFAZE_QUANTUM,
        .intent = PFAZE_F_ATOM,
        .kinetic = 4
    };
    assert(pfaze_run(&kernel, space) == PFAZE_WAIT);
    assert(cell.state == PFAZE_QUANTUM && cell.kinetic == 4);
    assert((cell.flags & PFAZE_WAITING) != 0);

    cell = (pfaze_cell){
        .state = PFAZE_GRABBING,
        .kinetic = 1,
        .internal = UINT64_MAX
    };
    assert(pfaze_run(&kernel, space) == PFAZE_ENERGY_OVERFLOW);
    assert(test.fault == PFAZE_ENERGY_OVERFLOW);

    cell = (pfaze_cell){
        .state = PFAZE_JOINED,
        .intent = PFAZE_F_ATOM | PFAZE_PH_ATOM
    };
    assert(pfaze_run(&kernel, space) == PFAZE_BAD_INTENT);
    assert(test.fault == PFAZE_BAD_INTENT);

    assert(pfaze_tick(&kernel, NULL) == PFAZE_BAD_POINTER);

    kernel.max_steps = 2;
    cell = (pfaze_cell){
        .quantum = {0, 0},
        .particle = {0, 0},
        .state = PFAZE_QUANTUM,
        .history = PFAZE_QUANTUM,
        .intent = PFAZE_LEGEND
    };
    assert(pfaze_run(&kernel, space) == PFAZE_STARVED);

    puts("pfaze kernel: 7 cases passed");
    return 0;
}

#endif
