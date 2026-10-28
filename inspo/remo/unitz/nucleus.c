/* ═══════════════════════════════════════════════════════════════
   NUCLEUS / VECTORSET
   ═══════════════════════════════════════════════════════════════ */

/*

        *nucleus(
            electron,
            proton,
            neutron
        )
              ↓
          vectorset

*/

typedef struct nucleus_t
{
    float electron;
    uint32_t proton;
    int32_t neutron;
} nucleus_t;

typedef nucleus_t *vectorset;

static inline nucleus_t nucleus(float electron, uint32_t proton, int32_t neutron)
{
    return (nucleus_t){electron, proton, neutron};
}
