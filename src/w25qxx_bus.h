#ifndef W25QXX_BUS_H_
#define W25QXX_BUS_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *user_ctx; /* forwarded untouched */

    /* CS level control – level = 0 => active (low), 1 => inactive (high) */
    void (*set_cs)(void *user_ctx, int level);

    /* Blocking transfers (return 0 on success) */
    int (*spi_tx)(void *user_ctx, const uint8_t *tx, size_t len);
    int (*spi_rx)(void *user_ctx, uint8_t *rx, size_t len);

    /* Millisecond counter (monotonic, 32-bit wrap-around fine) */
    uint32_t (*ticks_ms)(void);

    /* Millisecond delay */
    void (*delay_ms)(uint32_t ms);

    /* Constant representing “never timeout”. */
    uint32_t max_delay_ms;
} w25qxx_bus_t;

#ifdef __cplusplus
}
#endif

#endif /* W25QXX_BUS_H_ */
