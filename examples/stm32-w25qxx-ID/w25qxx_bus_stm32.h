#ifndef W25QXX_BUS_STM32_H_
#define W25QXX_BUS_STM32_H_

/* Adjust include paths to your HAL setup -------------------------------- */
#include "stm32_def.h"
#include "w25qxx_bus.h"

/* ----------------------------------------------------------------------- */
/* Backend-specific context structure                                      */
/* This structure holds the hardware-specific context for STM32:
 * - hspi: pointer to the SPI peripheral handle
 * - cs_pin: Pin number for chip select CS
 */
typedef struct
{
    SPIClass*          hspi;     /* SPI peripheral pointer                */
    uint32_t           cs_pin;   /* Pin for CS                            */
} W25qxxStm32Ctx;

/* ----------------------------------------------------------------------- */
/* Header-only callback implementations                                    */
/* `static inline` → each C file that includes this header gets its own    */
/* copy; therefore no multiple-definition linker errors.                   */

/* SPI transmit callback: sends data over SPI using HAL. Returns 0 on success. */
static inline int w25qxx_stm32_spi_tx(void *ctx, const uint8_t *tx, size_t len)
{
    W25qxxStm32Ctx *c = (W25qxxStm32Ctx *)ctx;
    c->hspi->transfer((uint8_t *)tx, (uint16_t)len, SPI_TRANSMITONLY);
    return 0;
}

/* SPI receive callback: receives data over SPI using HAL. Returns 0 on success. */
static inline int w25qxx_stm32_spi_rx(void *ctx, uint8_t *rx, size_t len)
{
    W25qxxStm32Ctx *c = (W25qxxStm32Ctx *)ctx;
    c->hspi->transfer(rx, len);
    return 0;
}

/* Set CS callback: sets the chip select pin high or low. */
static inline void w25qxx_stm32_set_cs(void *ctx, int level)
{
    W25qxxStm32Ctx *c = (W25qxxStm32Ctx *)ctx;
    digitalWrite(c->cs_pin, level ? HIGH : LOW);
}

/* Millisecond tick callback: returns current tick count. */
static inline uint32_t w25qxx_stm32_ticks(void)
{
    return HAL_GetTick();
}

/* Delay callback: delays for the specified number of milliseconds. */
static inline void w25qxx_stm32_delay(uint32_t ms)
{
    delay(ms);
}

/* ----------------------------------------------------------------------- */
/* Ready-made *template* bus object.                                       */
/* Marked `static const` so every translation unit has its own identical   */
/* instance (safe for the linker, tiny footprint).                         */
/* This object provides the function pointers for the bus interface.
 * The user_ctx must be set to point to a W25qxxStm32Ctx instance before use.
 */
static const w25qxx_bus_t w25qxx_bus_stm32 = {
    /* user_ctx     */ NULL,
    /* set_cs       */ w25qxx_stm32_set_cs,
    /* spi_tx       */ w25qxx_stm32_spi_tx,
    /* spi_rx       */ w25qxx_stm32_spi_rx,
    /* ticks_ms     */ w25qxx_stm32_ticks,
    /* delay_ms     */ w25qxx_stm32_delay,
    /* max_delay_ms */ HAL_MAX_DELAY
};

#endif /* W25QXX_BUS_STM32_H_ */
