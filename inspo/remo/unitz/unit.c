/* WASM field runtime, assembled through qck/m3sh.c. */

#define FIELDZ_IMPLEMENTATION
#include "phase.h"
#include "../b bin 'bio/bit.h"
#include "../../BinCh/chain.c"

static uint8_t chain_bytes[BC_BYTES];
static bit_chain chain = {.observation = {
                              .abi = BC_MAGIC, .target = BC_TARGET, .failed = UINT32_MAX}};
static uint32_t chain_run(void)
{
    if (!bc_run(&chain))
        return 0;
    for (uint32_t i = 0; i < BC_LANES; i++)
        field_shared.proposal[i] = fieldz_bound(chain.strip[i]);
    return chain.observation.runs;
}

/* ═══════════════════════════════════════════════════════════════
   REGISTERED ATOMS
   ═══════════════════════════════════════════════════════════════ */

static atom H = {
    .electron = 0.10f, .nucleus = {0}, .proton = 1, .neutron = 0, .halted = 0};

static atom O = {
    .electron = 0.80f, .nucleus = {0}, .proton = 8, .neutron = 8, .halted = 0};

static atom N = {
    .electron = 0.70f, .nucleus = {0}, .proton = 7, .neutron = 7, .halted = 0};

BM_CPU(H);
BM_CPU(O);
BM_CPU(N);

/* ═══════════════════════════════════════════════════════════════
   MATH — compact sin/cos for WASM (no libc)

        qsqrt → f32.sqrt
        qabs  → f32.abs
        qsin  → 7th-order Taylor, Cody-Waite reduction
        qcos  → qsin(x + π/2)
   ═══════════════════════════════════════════════════════════════ */

static const float _PI = 3.14159265359f;
static const float _HPI = 1.57079632679f;
static const float _TAU = 6.28318530718f;
static const float _ITAU = 0.15915494309f;

static inline float qsqrt(float x) { return __builtin_sqrtf(x); }
static inline float qabs(float x) { return __builtin_fabsf(x); }

static float
qsin(float x)
{
    x *= _ITAU;
    x -= (float)(int)(x + (x > 0.0f ? 0.5f : -0.5f));
    x *= _TAU;

    if (x > _HPI)
        x = _PI - x;
    else if (x < -_HPI)
        x = -_PI - x;

    float x2 = x * x;
    return x * (1.0f + x2 *
                           (-0.16666667f + x2 *
                                               (0.00833333f + x2 *
                                                                  -0.00019841f)));
}

static inline float
qcos(float x) { return qsin(x + _HPI); }

static float
qln(float x)
{
    if (x <= 0.0f)
        return -20.0f;
    float y = (x - 1.0f) / (x + 1.0f);
    float y2 = y * y;
    return 2.0f * y * (1.0f + y2 * (0.333333f + y2 * (0.2f + y2 * 0.142857f)));
}

static float
qexp(float x)
{
    if (x > 20.0f)
        return 1e8f;
    if (x < -20.0f)
        return 0.0f;
    float y = 1.0f + x * 0.00390625f;
    y *= y;
    y *= y;
    y *= y;
    y *= y;
    y *= y;
    y *= y;
    y *= y;
    y *= y;
    return y;
}

/* ═══════════════════════════════════════════════════════════════
   SO(3) ROTATION OPERATOR

        R(θ,â)v = v cos θ + (â × v) sin θ + â(â·v)(1 - cos θ)

        [R(θ,â)]ij = (δij - ai·aj) cos θ
                    - εijk·ak sin θ
                    + ai·aj

   Generator: Jk = i ∂R/∂θ|θ=0
   Algebra:   [Ji, Jj] = i εijk Jk
   ═══════════════════════════════════════════════════════════════ */

typedef struct
{
    float x, y, z;
} vec3;

static inline float
dot3(vec3 a, vec3 b)
{
    return a.x * b.x +
           a.y * b.y +
           a.z * b.z;
}

static inline vec3
cross3(vec3 a, vec3 b)
{
    return (vec3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x};
}

static inline vec3
normalize3(vec3 v)
{
    float n = qsqrt(dot3(v, v));
    if (n < 1e-8f)
        return (vec3){0, 0, 1};
    float inv = 1.0f / n;
    return (vec3){v.x * inv, v.y * inv, v.z * inv};
}

static vec3
rodrigues(vec3 v, vec3 axis, float theta)
{
    float c = qcos(theta);
    float s = qsin(theta);
    float k = dot3(axis, v);
    vec3 cr = cross3(axis, v);

    return (vec3){
        v.x * c + cr.x * s + axis.x * k * (1.0f - c),
        v.y * c + cr.y * s + axis.y * k * (1.0f - c),
        v.z * c + cr.z * s + axis.z * k * (1.0f - c)};
}

/* ═══════════════════════════════════════════════════════════════
   SPIN STATE — Larmor precession

        dS/dt = γ S × B
        S(t+dt) = R(ω·dt, B̂) S(t)
   ═══════════════════════════════════════════════════════════════ */

static struct
{
    vec3 s;
    float omega;
    float t;
} spin_state = {
    .s = {1.0f, 0.0f, 0.0f},
    .omega = 3.2f,
    .t = 0.0f};

static void
spin_step(float t_ms, float interp)
{
    if (spin_state.t == 0.0f)
    {
        spin_state.t = t_ms;
        return;
    }

    float dt = (t_ms - spin_state.t) * 0.001f;
    if (dt > 0.05f)
        dt = 0.05f;
    if (dt <= 0.0f)
    {
        spin_state.t = t_ms;
        return;
    }

    float turn = t_ms * 0.00017f;
    vec3 b_hat = normalize3((vec3){
        0.0f, qsin(turn), qcos(turn)});

    float gyro = 0.14f + interp * 0.08f;
    float theta = (spin_state.omega + gyro * 4.0f) * dt;

    spin_state.s = rodrigues(spin_state.s, b_hat, theta);
    spin_state.t = t_ms;
}

/* ═══════════════════════════════════════════════════════════════
   PHASE REFERENCE
   ═══════════════════════════════════════════════════════════════ */

typedef struct
{
    float xi3;
    float gyro_radius;
    float spin[3];
    float field[3];
} phase_ref_t;

static phase_ref_t phase_ref;

static void
update_phase_ref(float t_ms, float interp)
{
    phase_ref.xi3 = interp * 2.0f - 1.0f;
    phase_ref.gyro_radius = 0.14f + interp * 0.08f;
    phase_ref.spin[0] = spin_state.s.x;
    phase_ref.spin[1] = spin_state.s.y;
    phase_ref.spin[2] = spin_state.s.z;

    float turn = t_ms * 0.00017f;
    phase_ref.field[0] = 0.0f;
    phase_ref.field[1] = qsin(turn);
    phase_ref.field[2] = qcos(turn);
}

/* ═══════════════════════════════════════════════════════════════
   THERMODYNAMICS

        S = -Σ pᵢ ln(pᵢ)          entropy
        H = U + P·V                enthalpy
        T = S / S_max              temperature (normalized)
        F = H - T·S                free energy
   ═══════════════════════════════════════════════════════════════ */

typedef struct
{
    float entropy;
    float enthalpy;
    float temperature;
    float free_energy;
} thermo_t;

static thermo_t thermo;

static void
compute_thermo(void)
{
    float total = 0.0f;
    uint32_t i = 0;
    do
    {
        total += qabs(field_shared.state[i]);
        i++;
    } while (i < FIELDZ_LANES);

    if (total < 1e-6f)
    {
        thermo = (thermo_t){
            .entropy = 0.0f,
            .enthalpy = 0.0f,
            .temperature = 0.5f,
            .free_energy = 0.0f};
        return;
    }

    float inv = 1.0f / total;
    float S = 0.0f;
    i = 0;
    do
    {
        float p = qabs(field_shared.state[i]) * inv;
        if (p > 1e-8f)
            S -= p * qln(p);
        i++;
    } while (i < FIELDZ_LANES);

    float max_S = qln((float)FIELDZ_LANES);
    thermo.entropy = S;
    thermo.enthalpy = total +
                      phase_ref.gyro_radius * S;
    thermo.temperature =
        max_S > 0.0f ? S / max_S : 0.5f;
    thermo.free_energy =
        thermo.enthalpy -
        thermo.temperature * S;
}

/* ═══════════════════════════════════════════════════════════════
   PERPLEXITY'SPIN

        perplexity := exp(S) / N
        coherence  := 1 - T
        axis       := ∇S projected from state lanes
        ω          := 0.5 + perplexity × 8

        spin(t+dt) = R(ω·dt, â_eff) spin(t)

        â_eff blends: coherent → stable axis
                      decoherent → wandering axis
   ═══════════════════════════════════════════════════════════════ */

typedef struct
{
    float ax, ay, az;
    float sx, sy, sz;
    float perplexity;
    float coherence;
    float precession;
    float _pad;
} perplexity_spin_t;

static perplexity_spin_t pplx = {
    .sx = 1.0f, .sy = 0.0f, .sz = 0.0f, .perplexity = 1.0f, .coherence = 0.5f};

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
    do
    {
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

/* ═══════════════════════════════════════════════════════════════
   UNTI — irreducible core

        gather  := attract (entropy high, enthalpy low)
        lose    := emit    (enthalpy high, entropy low)
        split   := fission (both high — phase transition)
        stable  := ground  (both low)

        mass evolves: gather → heavier, lose → lighter
        quantum numbers (shell, m) from perplexity'spin
   ═══════════════════════════════════════════════════════════════ */

#define UNTI_MAX 8u

typedef struct
{
    float px, py, pz;
    float mx, my, mz;
    float mass;
    float charge;
    uint32_t state;
    uint32_t shell;
    int32_t m;
    uint32_t active;
} unti_t;

static unti_t unti[UNTI_MAX];
static uint32_t unti_count;
static uint32_t unti_epoch;

static void
unti_init(void)
{
    uint32_t reg = (uint32_t)fieldz_count();
    if (!reg)
        return;

    unti_count = reg < UNTI_MAX ? reg : UNTI_MAX;
    uint32_t i = 0;
    do
    {
        atom a = yield(v_addr(fieldz(), i));
        float angle = (float)i * _TAU / (float)unti_count;
        unti[i].px = qcos(angle) * 0.3f;
        unti[i].py = qsin(angle) * 0.3f;
        unti[i].pz = 0.0f;
        unti[i].mx = 0.0f;
        unti[i].my = 0.0f;
        unti[i].mz = 0.0f;
        unti[i].mass = 0.1f + a.electron * 0.5f;
        unti[i].charge = a.electron;
        unti[i].state = 3;
        unti[i].shell = i + 1;
        unti[i].m = 0;
        unti[i].active = 1;
        i++;
    } while (i < unti_count);

    unti_epoch = field_shared.model_epoch;
}

static void
unti_step(float t_ms)
{
    if (!unti_count ||
        unti_epoch != field_shared.model_epoch)
    {
        unti_init();
        if (!unti_count)
            return;
    }

    float max_S = qln((float)FIELDZ_LANES);
    float S_n = max_S > 0.0f
                    ? thermo.entropy / max_S
                    : 0.0f;
    float H_n = thermo.enthalpy /
                ((float)FIELDZ_LANES + 1.0f);

    uint32_t i = 0;
    do
    {
        if (!unti[i].active)
        {
            i++;
            continue;
        }

        if (S_n > 0.7f && H_n < 0.4f)
            unti[i].state = 0;
        else if (H_n > 0.6f && S_n < 0.3f)
            unti[i].state = 1;
        else if (S_n > 0.6f && H_n > 0.5f)
            unti[i].state = 2;
        else
            unti[i].state = 3;

        float dm = 0.0f;
        if (unti[i].state == 0)
            dm = 0.001f * pplx.coherence;
        else if (unti[i].state == 1)
            dm = -0.0005f * (1.0f - pplx.coherence);

        unti[i].mass += dm;
        if (unti[i].mass < 0.01f)
            unti[i].mass = 0.01f;
        if (unti[i].mass > 2.0f)
            unti[i].mass = 2.0f;

        float orbit_r =
            0.1f + (float)unti[i].shell * 0.12f;
        float orbit_spd =
            0.0003f / (float)unti[i].shell;

        unti[i].m = (int32_t)(pplx.sz * (float)unti[i].shell + 0.5f);
        if (unti[i].m > (int32_t)unti[i].shell)
            unti[i].m = (int32_t)unti[i].shell;
        if (unti[i].m < -(int32_t)unti[i].shell)
            unti[i].m = -(int32_t)unti[i].shell;

        float ph = t_ms * orbit_spd *
                   (1.0f + (float)unti[i].m * 0.3f);
        float spread =
            1.0f + thermo.temperature * 0.3f;
        unti[i].px = qcos(ph) * orbit_r * spread;
        unti[i].py = qsin(ph) * orbit_r * spread;
        unti[i].pz = (float)unti[i].m * 0.05f;

        i++;
    } while (i < unti_count);
}

/* ═══════════════════════════════════════════════════════════════
   FOUNDATION OF EFFECT — L3 gate layer

        gates(field_state) → effect[] → particle modulation

        {&: AND}   arm×arm co-activation
        {|: OR}    max arm energy
        {¬: NOT}   inverse of OR
        {⊨: THRESH} center coherence gate
        {Σ: SUM}   total field energy
        {Δ: CMP}   arm asymmetry
   ═══════════════════════════════════════════════════════════════ */

static float fnd_gates[8];
static float fnd_effect[FIELDZ_LANES];

static void
foundation_step(void)
{
    float arm_e[4];
    arm_e[0] = 0;
    arm_e[1] = 0;
    arm_e[2] = 0;
    arm_e[3] = 0;

    uint32_t i = 0;
    do
    {
        uint32_t region = i / 16u;
        arm_e[region] +=
            qabs(field_shared.state[i & 63]);
        i++;
    } while (i < FIELDZ_LANES);

    arm_e[0] /= 16.0f;
    arm_e[1] /= 16.0f;
    arm_e[2] /= 16.0f;
    arm_e[3] /= 16.0f;

    /* {&: AND} co-activation gates */
    fnd_gates[0] = arm_e[0] * arm_e[1];
    fnd_gates[1] = arm_e[1] * arm_e[2];
    fnd_gates[2] = arm_e[2] * arm_e[0];

    /* {|: OR} max arm */
    float or_v = arm_e[0];
    if (arm_e[1] > or_v)
        or_v = arm_e[1];
    if (arm_e[2] > or_v)
        or_v = arm_e[2];
    fnd_gates[3] = or_v;

    /* {⊨: THRESH} center coherence */
    fnd_gates[4] = arm_e[3] > 0.3f ? 1.0f : arm_e[3] / 0.3f;

    /* {¬: NOT} silence detector */
    fnd_gates[5] = 1.0f - or_v;

    /* {Σ: SUM} total energy */
    fnd_gates[6] = (arm_e[0] + arm_e[1] +
                    arm_e[2] + arm_e[3]) *
                   0.25f;

    /* {Δ: CMP} max arm asymmetry */
    float d01 = qabs(arm_e[0] - arm_e[1]);
    float d12 = qabs(arm_e[1] - arm_e[2]);
    float d20 = qabs(arm_e[2] - arm_e[0]);
    fnd_gates[7] = d01;
    if (d12 > fnd_gates[7])
        fnd_gates[7] = d12;
    if (d20 > fnd_gates[7])
        fnd_gates[7] = d20;

    /* effect[]: per-point modulation */
    i = 0;
    do
    {
        uint32_t region = i / 16u;
        float gate = region < 3u
                         ? arm_e[region] *
                               (0.4f + fnd_gates[3] * 0.6f)
                         : fnd_gates[4];
        fnd_effect[i] = 0.3f + gate * 0.7f;
        i++;
    } while (i < FIELDZ_LANES);
}

/* ═══════════════════════════════════════════════════════════════
   EM — electromagnetic stabilization

        E = -∇φ       Coulomb restoring (spring to equilibrium)
        B = ∇ × A     neighbor coupling (curl of displacement)
        F = q(v × B)  Lorentz deflection on particle orbits

        unity := strip state → coupling strength; no display attributes
   ═══════════════════════════════════════════════════════════════ */

static float em_q[FIELDZ_LANES];
static float em_p[FIELDZ_LANES];
static float em_bz[FIELDZ_LANES];
static float em_unity;

static void
em_step(void)
{
    float u = (field_shared.state[62] + 1.0f) * 0.5f;
    if (u < 0.0f)
        u = 0.0f;
    if (u > 1.0f)
        u = 1.0f;
    em_unity = u;

    float thresh = fnd_gates[4];
    float restore = 0.04f + u * 0.12f *
                                (0.5f + thresh * 0.5f);
    float couple = u * 0.02f *
                   (0.3f + fnd_gates[6] * 0.7f);

    uint32_t i = 0;
    do
    {
        float fq = field_shared.state[i & 63] * 0.003f;
        float fp = field_shared.state[(i + 32) & 63] * 0.003f;

        em_q[i] = em_q[i] * (1.0f - restore) + fq;
        em_p[i] = em_p[i] * (1.0f - restore) + fp;

        uint32_t prev = (i + FIELDZ_LANES - 1) % FIELDZ_LANES;
        uint32_t next = (i + 1) % FIELDZ_LANES;

        float dq = (em_q[prev] + em_q[next]) * 0.5f - em_q[i];
        float dp = (em_p[prev] + em_p[next]) * 0.5f - em_p[i];
        em_q[i] += dq * couple;
        em_p[i] += dp * couple;

        em_bz[i] = (em_q[next] - em_q[prev] -
                    em_p[next] + em_p[prev]) *
                   0.25f;

        i++;
    } while (i < FIELDZ_LANES);
}

/* ═══════════════════════════════════════════════════════════════
   VIEWPORT — JS writes here
   ═══════════════════════════════════════════════════════════════ */

typedef struct
{
    float cx, cy;
    float span;
    float pointer_q, pointer_p;
    uint32_t pointer_active;
} viewport_t;

static viewport_t viewport;

/* ═══════════════════════════════════════════════════════════════
   PARTICLE KERNEL

   q'partikles sample the bio'chain geometry in normalized scene units.
   This preview uses classical motion; appearance belongs to the projector.
   ═══════════════════════════════════════════════════════════════ */

#define POPULATION 10000u

static float p_radius0[POPULATION];
static float p_velocity0[POPULATION];
static float positions[POPULATION * 4]; /* x, y, z, cycle — no appearance */
static uint8_t groups[POPULATION];      /* motion layer */
static float strip_rim[257 * 3];        /* one continuous boundary */
static float strip_mesh[129 * 17 * 8];  /* position, normal, surface coordinates */
static uint16_t strip_indices[128 * 16 * 6];

typedef struct
{
    float x, y, z, charge, cycle;
} atom_motion_t;

static atom_motion_t atom_motion[8];
static uint32_t cfg_registry;
static uint32_t cfg_model_epoch;
static bio_chain bio;
static bio_recipe bio_staging;
static uint32_t mesh_epoch, mesh_recipe_epoch;

static void bio_ready(void)
{
    if (bio.abi != BIO_ABI)
        bio_apply(&bio, &bio_default);
}

static vec3 surface_point(float t, float v)
{
    bio_vec p = bio_point(&bio, t, v);
    return (vec3){p.x, p.y, p.z};
}

static float strip_cycle(float u)
{
    float cycle = u / (_TAU * 2.0f);
    return cycle - __builtin_floorf(cycle);
}

static void configure_particles(void)
{
    uint32_t registry = (uint32_t)fieldz_count();
    if (!registry)
        return;
    for (uint32_t n = 0; n < POPULATION; n++)
    {
        atom a = yield(v_addr(fieldz(), n % registry));
        float raw = (float)(n + 1) * 0.61803398875f;
        float transverse = (raw - (float)(int)raw) * 2.0f - 1.0f;
        p_radius0[n] = transverse;
        p_velocity0[n] = a.halted ? 0.0f : 0.00010f + a.electron * 0.00008f;
        groups[n] = (uint8_t)(n % 3u);
    }
    cfg_registry = registry;
    cfg_model_epoch = field_shared.model_epoch;
}

static void configure_surface(void)
{
    float laps = bio_closed(&bio.recipe) && bio.recipe.section == 0 && bio.reversed ? 2.0f : 1.0f;
    for (uint32_t i = 0; i <= 256; i++)
    {
        vec3 p = surface_point((float)i / 256.0f * laps, 1.0f);
        strip_rim[i * 3] = p.x;
        strip_rim[i * 3 + 1] = p.y;
        strip_rim[i * 3 + 2] = p.z;
    }
    for (uint32_t i = 0; i <= 128; i++)
    {
        float u = (float)i / 128.0f;
        for (uint32_t j = 0; j <= 16; j++)
        {
            float v = (float)j / 16.0f * 2.0f - 1.0f;
            vec3 p = surface_point(u, v), a = surface_point(u + 0.0005f, v), b = surface_point(u - 0.0005f, v);
            vec3 c = surface_point(u, v + 0.002f), d = surface_point(u, v - 0.002f);
            vec3 normal = normalize3(cross3((vec3){a.x - b.x, a.y - b.y, a.z - b.z},
                                            (vec3){c.x - d.x, c.y - d.y, c.z - d.z}));
            uint32_t k = (i * 17 + j) * 8;
            strip_mesh[k] = p.x;
            strip_mesh[k + 1] = p.y;
            strip_mesh[k + 2] = p.z;
            strip_mesh[k + 3] = normal.x;
            strip_mesh[k + 4] = normal.y;
            strip_mesh[k + 5] = normal.z;
            strip_mesh[k + 6] = (float)i / 128.0f;
            strip_mesh[k + 7] = (float)j / 16.0f;
            if (i < 128 && j < 16)
            {
                uint32_t index = (i * 16 + j) * 6, vertex = i * 17 + j;
                strip_indices[index] = (uint16_t)vertex;
                strip_indices[index + 1] = (uint16_t)(vertex + 17);
                strip_indices[index + 2] = (uint16_t)(vertex + 1);
                strip_indices[index + 3] = (uint16_t)(vertex + 1);
                strip_indices[index + 4] = (uint16_t)(vertex + 17);
                strip_indices[index + 5] = (uint16_t)(vertex + 18);
            }
        }
    }
}

static void evolve(float t_ms)
{
    uint32_t registry = (uint32_t)fieldz_count();
    if (!registry)
        return;
    float laps = bio_closed(&bio.recipe) && bio.reversed ? 2.0f : 1.0f;
    for (uint32_t a = 0; a < registry && a < 8; a++)
    {
        atom at = yield(v_addr(fieldz(), a));
        float cycle = strip_cycle((float)a / (float)registry * _TAU * 2.0f +
                                  (at.halted ? 0.0f : t_ms * (.00010f + at.electron * .00008f)));
        vec3 p = surface_point(cycle * laps, (at.electron - .45f) * 2.0f);
        atom_motion[a] = (atom_motion_t){p.x, p.y, p.z, at.electron, cycle};
    }
    uint32_t count = (uint32_t)bio.recipe.density;
    for (uint32_t n = 0; n < count; n++)
    {
        float start = (float)n / (float)count * _TAU * 2.0f;
        float cycle = strip_cycle(start + t_ms * p_velocity0[n]);
        vec3 p = surface_point(cycle * laps, p_radius0[n]);
        positions[n * 4] = p.x;
        positions[n * 4 + 1] = p.y;
        positions[n * 4 + 2] = p.z;
        positions[n * 4 + 3] = cycle;
    }
}

/* ═══════════════════════════════════════════════════════════════
   DISPATCH
   ═══════════════════════════════════════════════════════════════ */

static atom whole_cache;

enum
{
    Q_TIC = 1,
    Q_KERNEL = 2,
    Q_COUNT = 3,
    Q_YIELD = 4,
    Q_MMOVE = 5,
    Q_REF = 6,
    Q_SPACE = 7,
    Q_MODEL = 8,
    Q_PHYSICS = 9,
    Q_FRAME = 10,
    Q_VIEWPORT = 11,
    Q_POSITIONS = 12,
    Q_GROUPS = 13,
    Q_ATOMS = 14,
    Q_SPIN = 15,
    Q_THERMO = 16,
    Q_PERPLEXITY = 17,
    Q_UNTI = 18,
    Q_UNTI_COUNT = 19,
    Q_EM_UNITY = 20,
    Q_GATES = 21,
    Q_BYTES = 22,
    Q_COMPILE = 23,
    Q_RUN = 24,
    Q_OBSERVE = 25,
    Q_STRIP = 26,
    Q_RIM = 27,
    Q_MESH = 28,
    Q_INDICES = 29,
    Q_INDEX_COUNT = 30,
    Q_BIO_BYTES = 31,
    Q_BIO_APPLY = 32,
    Q_BIO_RECIPE = 33,
    Q_BIO_OBSERVE = 34,
    Q_MESH_EPOCH = 35,
    Q_POPULATION = 36,
    Q_BIO_MOTION = 37
};

__attribute__((export_name("q")))
uint32_t
q(uint32_t op, uint32_t a, uint32_t b)
{
    switch (op)
    {

    case Q_TIC:
        tic((ignition_t){
            .tic = a,
            .interp = (float)b / 1000.0f});
        return a;

    case Q_KERNEL:
        return fieldz_kernel();

    case Q_COUNT:
        return (uint32_t)fieldz_count();

    case Q_YIELD:
    {
        if (a >= fieldz_count())
            return 0;
        whole_cache = yield(v_addr(fieldz(), a));
        return (uint32_t)(uintptr_t)&whole_cache;
    }

    case Q_REF:
        return (uint32_t)(uintptr_t)&phase_ref;

    case Q_SPACE:
        return (uint32_t)(uintptr_t)fieldz_space();

    case Q_MODEL:
    {
        union
        {
            uint32_t u;
            float f;
        } v = {.u = b};
        if (!bc_finite(v.f))
            return 0;
        if (!fieldz_model(a, v.f))
            return 0;
        chain.strip[a] = field_shared.proposal[a];
        return 1;
    }

    case Q_PHYSICS:
        fieldz_physics((float)a / 1000.0f);
        return field_shared.physics_epoch;

        /* ── FRAME ──────────────────────────────────────────── */

    case Q_FRAME:
    {
        bio_ready();
        float t = (float)a;
        uint32_t tick = (uint32_t)(t / 16.0f);
        float interp = 0.5f + qsin(t * 0.00035f) * 0.5f;

        tic((ignition_t){.tic = tick, .interp = interp});
        chain_run();
        fieldz_physics(0.08f);
        spin_step(t, interp);
        update_phase_ref(t, interp);
        compute_thermo();
        perplexity_step(t);
        unti_step(t);
        foundation_step();
        em_step();
        bio_step(&bio, a, field_shared.state);

        uint32_t reg = (uint32_t)fieldz_count();

        if (cfg_registry != reg ||
            cfg_model_epoch != field_shared.model_epoch)
            configure_particles();

        if (mesh_recipe_epoch != bio.epoch || bio.recipe.drive != 0 ||
            bio.recipe.amplitude != 0 || bio.recipe.response != 0)
        {
            configure_surface();
            mesh_recipe_epoch = bio.epoch;
            mesh_epoch++;
        }
        evolve(t);
        return reg;
    }

    case Q_VIEWPORT:
        return (uint32_t)(uintptr_t)&viewport;

    case Q_POSITIONS:
        return (uint32_t)(uintptr_t)positions;

    case Q_GROUPS:
        return (uint32_t)(uintptr_t)groups;

    case Q_ATOMS:
        return (uint32_t)(uintptr_t)atom_motion;

    case Q_SPIN:
        return (uint32_t)(uintptr_t)&spin_state;

    case Q_THERMO:
        return (uint32_t)(uintptr_t)&thermo;

    case Q_PERPLEXITY:
        return (uint32_t)(uintptr_t)&pplx;

    case Q_UNTI:
        return (uint32_t)(uintptr_t)unti;

    case Q_UNTI_COUNT:
        return unti_count;

    case Q_EM_UNITY:
    {
        union
        {
            float f;
            uint32_t u;
        } v = {.f = em_unity};
        return v.u;
    }

    case Q_GATES:
        return (uint32_t)(uintptr_t)fnd_gates;

    case Q_BYTES:
        return a ? BC_BYTES : (uint32_t)(uintptr_t)chain_bytes;

    case Q_COMPILE:
        if (!bc_compile(&chain, chain_bytes, a))
            return 0;
        for (uint32_t i = 0; i < BC_LANES; i++)
            chain.strip[i] = field_shared.state[i];
        field_shared.model_epoch++;
        return 1;

    case Q_RUN:
        return chain_run();

    case Q_STRIP:
        return (uint32_t)(uintptr_t)chain.strip;

    case Q_MESH:
        return (uint32_t)(uintptr_t)strip_mesh;
    case Q_INDICES:
        return (uint32_t)(uintptr_t)strip_indices;
    case Q_INDEX_COUNT:
        return 128u * 16u * 6u;

    case Q_RIM:
        return (uint32_t)(uintptr_t)strip_rim;

    case Q_OBSERVE:
        return (uint32_t)(uintptr_t)&chain.observation;

    case Q_BIO_BYTES:
        return a ? sizeof(bio_recipe) : (uint32_t)(uintptr_t)&bio_staging;
    case Q_BIO_APPLY:
        if (a != sizeof(bio_recipe))
            return 0;
        return bio_apply(&bio, &bio_staging) ? bio.epoch : 0;
    case Q_BIO_RECIPE:
        bio_ready();
        return (uint32_t)(uintptr_t)&bio.recipe;
    case Q_BIO_OBSERVE:
        bio_ready();
        return (uint32_t)(uintptr_t)&bio;
    case Q_MESH_EPOCH:
        return mesh_epoch;
    case Q_POPULATION:
        bio_ready();
        return (uint32_t)bio.recipe.density;
    case Q_BIO_MOTION:
        bio_ready();
        return (uint32_t)(uintptr_t)bio.displacement;
    default:
        return 0;
    }
}
