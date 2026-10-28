static inline ignition_t
ignit_kernel(void)
{
    return deploy_nit(
        atomic_load_explicit(
            &field_ignition,
            memory_order_acquire));
}
