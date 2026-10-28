/* Native 64-bit spatial matrix rotation macro (Your Core Math) */
#define div_rotate_step(v, n) \
    ((uint64_t)(((uint64_t)(v) << ((n) & 63u)) | ((uint64_t)(v) >> ((64u - (n)) & 63u))))
