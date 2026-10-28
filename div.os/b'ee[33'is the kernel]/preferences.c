/* ========================================================================= */
/* THE 2D CANVAS *hangs on the wall*  */
/* ========================================================================= */
#define pfaze_canvasp(p, x, y_stride) \
    ((volatile unsigned *)(p))[(x) + (y_stride)]

#define pfaze_bit_window(p, bit_sh, bit_w) \
    (((*(volatile unsigned *)(p)) >> (bit_sh)) & ((1ULL << (bit_w)) - 1ULL))

#define pfaze_disc(p, x, y_stride, bit_sh, bit_w) \
    pfaze_bit_window(&pfaze_canvasp(p, x, y_stride), bit_sh, bit_w)

#if defined(VSET_WASM)
__attribute__((export_name("vset_phase"))) unsigned vset_phase(unsigned channel_x, unsigned channel_y, unsigned step)
{
    unsigned *x = &channel_x;
    unsigned *y = &channel_y;

    return b_packet(unsigned, &x, &y, step);
}
#endif
