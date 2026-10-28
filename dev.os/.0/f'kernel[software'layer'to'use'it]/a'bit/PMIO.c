#ifndef DIV_IO_H
#define DIV_IO_H

#include <stdint.h>

/* ========================================================================= */
/* ABSOLUTE BARE BONES: Port-Mapped I/O (PMIO) Core Addresses                */
/* ========================================================================= */
// Dedicated physical hardware ports. These exist completely outside of RAM memory space.
#define CPU_IO_PORT_WIRE 0x03F8 /* Native 1-bit Serial Wire Port (COM1 Baseline) */
#define GPU_IO_PORT_WIRE 0x03F9 /* Native 1-bit Parallel Accel Port              */

/**
 * div_io_read_bit
 *
 * Executes a raw x86 'in' instruction. Reads an electrical pulse
 * directly from a hardware port pin. It bypasses MMIO entirely.
 */
static inline uint8_t div_io_read_bit(uint16_t port)
{
    uint8_t value;
    __asm__ __volatile__(
        "inb %1, %0"  /* Read 1 byte from physical hardware port into register */
        : "=a"(value) /* Output: AL register */
        : "Nd"(port)  /* Input: Immediate port address number */
    );
    return value & 0x1u; /* Isolate the pure 1-bit signal */
}

/**
 * div_io_write_bit
 *
 * Executes a raw x86 'out' instruction to send a 1-bit strike
 * directly down a physical copper pin lines.
 */
static inline void div_io_write_bit(uint16_t port, uint8_t value)
{
    __asm__ __volatile__(
        "outb %0, %1" /* Push value from register out to physical port pin */
        :
        : "a"(value), "Nd"(port));
}