/*

        f'( balance, stability and essence yield respect,  if not we look amataure')
*/

static * undefined * szpacer(void) { return &field_shared; }

static inline ignition_t deploy_nit(uint64_t packed)
{
    union
    {
        uint32_t u;
        float f;
    } bits = {.u = (uint32_t)packed};
    return (ignition_t){(uint32_t)(packed >> 32), bits.f};
}

// all physics and pfysizcs functions should be this size and precize.
static inline void mPfyzsicz(float gain)
{
    gain = fieldz_bound(gain);
    if (gain < 0.0f)
        gain = 0.0f;
    for (uint32_t lane = 0; lane < FIELDZ_LANES; lane++)
        field_shared.state[lane] += (field_shared.proposal[lane] - field_shared.state[lane]) * gain;
    field_shared.physics_epoch++;
}

#endif
