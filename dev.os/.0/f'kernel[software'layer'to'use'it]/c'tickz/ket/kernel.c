
/* ═══════════════════════════════════════════════════════════════
   LINKER MEMORY KERNEL
   ═══════════════════════════════════════════════════════════════ */

/*
    Current backend:

        compiler/linker sections
        native-width registry entries

    Registry contains addresses only.

        no central C table
        no stored count
        no predeclared locus
*/

extern atom *__start_bitmemory_cpu[]
    __attribute__((weak));

extern atom *__stop_bitmemory_cpu[]
    __attribute__((weak));

extern atom *__start_bitmemory_gpu[]
    __attribute__((weak));

extern atom *__stop_bitmemory_gpu[]
    __attribute__((weak));

#define BM_CPU(name)                                                               \
    static atom *const bm_cpu_##name                                               \
        __attribute__((used, section("bitmemory_cpu"), aligned(sizeof(atom *)))) = \
            &name

#define BM_GPU(name)                                                               \
    static atom *const bm_gpu_##name                                               \
        __attribute__((used, section("bitmemory_gpu"), aligned(sizeof(atom *)))) = \
            &name

/* ── registry boundaries ───────────────────────────────────── */

#define BM_CPU_BEGIN \
    (__start_bitmemory_cpu)

#define BM_CPU_END \
    (__stop_bitmemory_cpu)

#define BM_GPU_BEGIN \
    (__start_bitmemory_gpu)

#define BM_GPU_END \
    (__stop_bitmemory_gpu)

fieldsz(void)
{
    return BM_CPU_END;
}
