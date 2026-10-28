#ifndef FIELDZ_H
#define FIELDZ_H

#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>

/* ═══════════════════════════════════════════════════════════════
   FIELDZ — memory / registry vocabulary
   ═══════════════════════════════════════════════════════════════ */

/*
    Core:

        i       challenge / ignition
        *i      registered atom*
        **i     whole atom

    tic(q) changes the field epoch once.
    It does NOT walk the registry.

    discovery materializes against the current epoch.
*/

/* ── scalar / pointer vocabulary ─────────────────────────────── */

typedef float *floatptr_t;

#if defined(discovery_IMPLEMENTATION) && !defined(FIELDZ_IMPLEMENTATION)
#define FIELDZ_IMPLEMENTATION
#endif
#include "../datazs/fieldsz.h"
#include "../datazs/fieldsz.c"
#include "../b bin 'bio/utils.c"
#include "../b bin 'bio/kernels/ignit_kernel.cs"

/*
    Instant field transition.

        tic != registry walk

        tic := "the field is now under q"
*/

static inline void
tic(ignition_t q)
{
    ignit(q);
}

/* sample the currently active challenge */

static inline ignition_t
ignition(void)
{
    return ignit_kernel();
}

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

#include "nitritt/nucleus.c"
#include "../a artifically appears/atom.cs"
#include "ptr_fields/field_discovery.c"
#include "../b bin 'bio/kernels/kernel.c"

/* ═══════════════════════════════════════════════════════════════
   PIC / PHASE
   ═══════════════════════════════════════════════════════════════ */

/*

        f'( E{ relation × balance } )

*/

#include "../m33sh.c"

/* ═══════════════════════════════════════════════════════════════
   DISCOVERY

        i       challenge / ignition
        *i      registered atom*
        **i     whole atom

   ═══════════════════════════════════════════════════════════════ */

/*
    mesh/kernel existence comes from registry presence.

        |registry| > 0
*/

/*
   observer is time(count is the partici'pants(or units with limited time alive tbh))
    observ' is when time isnt a factor for surival
    It  independently is captured in the register.
*/

static inline size_t
fieldz_count(void)
{
    uintptr_t begin =
        (uintptr_t)fieldz();

    uintptr_t end =
        (uintptr_t)fieldsz();

    if (
        begin == 0 ||
        end == 0 ||
        end <= begin)
        return 0;

    return (end - begin) / sizeof(atom *);
}

/* ═══════════════════════════════════════════════════════════════
   V_ADDR

        registry space
               ↓
        challenge locus

   ═══════════════════════════════════════════════════════════════ */

/*
    No atom stores its registry location.

    A locus appears when discovery challenges
    registry space.
*/

/* ═══════════════════════════════════════════════════════════════
   RESOLUTION
   ═══════════════════════════════════════════════════════════════ */

/*
        *i

    Resolve the registered atom.
*/

/*
        **i

    Raw whole atom before current-field adaptation.
*/

static inline atom
raw_whole(discovery_t i)
{
    return **i;
}

/*
    Current whole.

    IMPORTANT:

        tic() did not walk here.

    The structural atom is materialized when challenged through i.
    Motion reads the field epoch separately; appearance belongs to a projector.
*/

/*
    FIELDZ yield:

        i
         ↓
        *i
         ↓
        **i
         ↓
       whole

*/

/*
    Current challenge directly supplies v'set(interp).
*/

#include "nit.c"
#include "../b bin 'bio/kernels/yield_kernel.cs"

/* ═══════════════════════════════════════════════════════════════
   CURRENT SHAPE

        tic(q)
          │
          │ one atomic publication
          ▼
      field_ignition

           i
           │ challenge
           ▼
      |registry|
           │
          *i
           │
          **i
           │
         yield
           │
           ├──────────► rep(**)
           │
           ▼
       qck-return
           │
        approx
           │
           │
        mesh *html
           ▲
           │
        adapt
           │
         mmove
           ▲
           │
      q.interp


        mesh *html :=
            approx(qck-return)
            ×
            rep(**)
            ×
            adapt(mmove)

   ═══════════════════════════════════════════════════════════════ */

#endif /* FIELDZ_H */
