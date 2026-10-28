#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "q'partikles.h"

int main(void)
{
    printf("q'partikles test harness\n");
    printf("========================\n\n");

    printf("=== Runic Markers ===\n");
    printf("CF  (Spark)   : 0x%03llX\n", STD_CF);
    printf("PF  (Balance) : 0x%03llX\n", STD_PF);
    printf("AF  (Bridge)  : 0x%03llX\n", STD_AF);
    printf("ZF  (Void)    : 0x%03llX\n", STD_ZF);
    printf("SF  (Horizon) : 0x%03llX\n", STD_SF);
    printf("OF  (Breach)  : 0x%03llX\n", STD_OF);
    printf("ding (union)  : 0x%03llX\n", (unsigned long long)ding);

    printf("\n=== vector() ===\n");
    printf("uint8_t  : %u\n", vector(uint8_t));
    printf("uint16_t : %u\n", vector(uint16_t));
    printf("uint32_t : %u\n", vector(uint32_t));
    printf("uint64_t : %u\n", vector(uint64_t));

    printf("\n=== B_MASK ===\n");
    printf("B_MASK(uint8_t,  4) : 0x%02X\n", (unsigned)B_MASK(uint8_t, 4));
    printf("B_MASK(uint32_t,16) : 0x%08X\n", B_MASK(uint32_t, 16));

    printf("\n=== rotator_axis / spin_set ===\n");
    uint32_t val = 0xDEADBEEF;
    printf("original        : 0x%08X\n", val);
    printf("rotator_axis(7) : 0x%08X\n", rotator_axis(uint32_t, val, 7));
    printf("spin_set(7)     : 0x%08X\n", spin_set(uint32_t, val, 7));

    printf("\n=== stability ===\n");
    printf("stability(0xCAFEBABE) : 0x%04X\n", stability(uint32_t, 0xCAFEBABE));

    printf("\n=== v_set ===\n");
    uint32_t combined = v_set(uint32_t, 0xCAFE, 0xBABE);
    printf("v_set(0xCAFE, 0xBABE) : 0x%08X\n", combined);
    assert(combined == UINT32_C(0xCAFEBABE));

#define CHECK_BITS(T) do { \
    T bits = (T)(B_ALL(T) ^ (T)0x5A); \
    assert(B_MASK(T, 0) == 0); \
    assert(B_MASK(T, vector(T)) == B_ALL(T)); \
    assert(B_MASK(T, vector(T) + 1) == B_ALL(T)); \
    assert(rotator_axis(T, 1, 1) == 2); \
    assert(spin_set(T, 2, 1) == 1); \
    assert(v_set(T, B_ALL(T), B_ALL(T)) == B_ALL(T)); \
    assert(stability(T, B_ALL(T)) == B_MASK(T, vector(T) / 2)); \
    for (unsigned shift = 0; shift <= 2 * vector(T); ++shift) \
        assert(spin_set(T, rotator_axis(T, bits, shift), shift) == bits); \
} while (0)
    CHECK_BITS(uint8_t);
    CHECK_BITS(uint16_t);
    CHECK_BITS(uint32_t);
    CHECK_BITS(uint64_t);
#undef CHECK_BITS

    printf("\n=== b_packet ===\n");
    uint32_t x = 0x12345678, y = 0xABCDEF01;
    uint32_t *px = &x, *py = &y;
    uint32_t pkt = b_packet(uint32_t, &px, &py, 3);
    printf("b_packet result : 0x%08X\n", pkt);
    assert(pkt == UINT32_C(0x8ACF2BCD));

    printf("\n=== q_packet_dock ===\n");
    uint32_t cur = 0xFACEFEED, gen = 0x00000000;
    uint32_t *pc = &cur, **ppc = &pc, ***pppc = &ppc;
    uint32_t *pg = &gen, **ppg = &pg, ***pppg = &ppg;
    printf("before: genesis=0x%08X current=0x%08X\n", gen, cur);
    q_packet_dock(uint32_t, pppc, pppg);
    printf("after:  genesis root=0x%08X current=0x%08X\n", *pg, cur);
    assert(pg == pc && ppg == &pg && ppc == &pc);
    assert(gen == 0 && cur == UINT32_C(0xFACEFEED));
    *pg = 7;
    assert(cur == 7 && gen == 0);
    q_packet_dock(uint32_t, pppg, pppg);
    assert(pg == pc);
    uint32_t ***sources[] = {pppc};
    uint32_t ***destinations[] = {pppg};
    unsigned source_index = 0, destination_index = 0;
    q_packet_dock(uint32_t, sources[source_index++], destinations[destination_index++]);
    assert(source_index == 1 && destination_index == 1 && pg == pc);

    printf("\n=== timer ===\n");
    uint64_t t0 = timer();
    volatile uint32_t sink = 0;
    for (uint32_t i = 0; i < 1000000; i++) sink += i;
    (void)sink;
    uint64_t t1 = timer();
    printf("delta: %llu cycles\n", (unsigned long long)(t1 - t0));

    printf("\nall tests complete.\n");
    return 0;
}
