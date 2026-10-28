/* sec_kernelzs — Perplexity'Spin

        perplexity := exp(S) / N
        coherence  := 1 - T
        axis       := ∇S projected from state lanes
        ω          := 0.5 + perplexity × 8

        spin(t+dt) = R(ω·dt, â_eff) spin(t)

        â_eff blends: coherent  → stable axis
                      decoherent → wandering axis
*/

typedef struct {
    float ax, ay, az;
    float sx, sy, sz;
    float perplexity;
    float coherence;
    float precession;
    float _pad;
} perplexity_spin_t;

static perplexity_spin_t pplx = {
    .sx = 1.0f, .sy = 0.0f, .sz = 0.0f,
    .perplexity = 1.0f,
    .coherence = 0.5f};

static void
perplexity_step(float t_ms)
{
    pplx.perplexity =
        qexp(thermo.entropy) / (float)FIELDZ_LANES;
    pplx.coherence =
        1.0f - thermo.temperature;
    pplx.precession =
        0.5f + pplx.perplexity * 8.0f;

    float gx = 0.0f, gy = 0.0f, gz = 0.0f;
    uint32_t i = 0;
    do {
        float s = field_shared.state[i];
        float a = (float)i * 0.098174f;
        gx += s * qcos(a);
        gy += s * qsin(a);
        gz += s * qcos(a * 0.5f + 1.0f);
        i++;
    } while (i < FIELDZ_LANES);

    vec3 axis = normalize3((vec3){gx, gy, gz});
    pplx.ax = axis.x;
    pplx.ay = axis.y;
    pplx.az = axis.z;

    vec3 eff;
    if (pplx.coherence > 0.5f)
        eff = axis;
    else
        eff = normalize3((vec3){
            axis.x + qsin(t_ms * 0.003f) *
                (1.0f - pplx.coherence),
            axis.y + qcos(t_ms * 0.0037f) *
                (1.0f - pplx.coherence),
            axis.z + qsin(t_ms * 0.0041f) *
                (1.0f - pplx.coherence)});

    vec3 sp = {pplx.sx, pplx.sy, pplx.sz};
    sp = rodrigues(sp, eff, pplx.precession * 0.016f);
    pplx.sx = sp.x;
    pplx.sy = sp.y;
    pplx.sz = sp.z;
}
