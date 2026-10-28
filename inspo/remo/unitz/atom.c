typedef struct atom
{
    float electron;
    nucleus_t nucleus;
    uint32_t proton;
    int32_t neutron;
    int halted;
} atom;

/* Materialize the challenged atom after the phase definitions are available. */
static inline atom
yield(discovery_t i)
{
    return whole(i);
}

static inline atom
whole(discovery_t i)
{
    atom a =
        **i;

    a.nucleus =
        nucleus(
            a.electron,
            a.proton,
            a.neutron);

    return a;
}