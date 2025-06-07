// Blink LED_BUILTIN slower if the FLASH chip is detected, and faster if there is no FLASH chip detected
#include <SPI.h>
#include "w25qxx.h"
#include "w25qxx_bus_stm32.h"

#define SERIAL_BAUD      115200
#define BLINK_FAST_DELAY 50
#define BLINK_SLOW_DELAY 1000
#define PAGE_SIZE 4096
#define DBG(...)    Serial.printf(__VA_ARGS__)

#if defined(ARDUINO_BLACKPILL_F411CE)
//              MOSI  MISO  SCLK
SPIClass SPIbus(PA7,  PA6,  PA5);
#define CS_PIN PA4
#else
//              MOSI  MISO  SCLK
SPIClass SPIbus(PC12, PC11, PC10);
#define CS_PIN PD2
#endif

GPIO_TypeDef *LED_GPIO_Port = digitalPinToPort(LED_BUILTIN);
uint16_t LED_Pin = digitalPinToBitMask(LED_BUILTIN);

GPIO_TypeDef *SPI_CS_GPIO_Port;
uint16_t SPI_CS_Pin;

// the setup routine runs once when you press reset:
void setup()
{
    CRC_HandleTypeDef hcrc;

    SPI_HandleTypeDef *hspi;

    W25QXX_HandleTypeDef w25qxx; // Handler for all w25qxx operations!

    Serial.begin(SERIAL_BAUD);
    while (!Serial) delay(100); // wait until Serial/monitor is opened

    Serial.println("SPI Flash test...");

    pinMode(LED_BUILTIN, OUTPUT);

    LED_GPIO_Port = digitalPinToPort(LED_BUILTIN);
    LED_Pin = digitalPinToBitMask(LED_BUILTIN);

    // ensure the CS pin is pulled HIGH
    pinMode(CS_PIN, OUTPUT); digitalWrite(CS_PIN, HIGH);

    SPI_CS_GPIO_Port = digitalPinToPort(CS_PIN);
    SPI_CS_Pin = digitalPinToBitMask(CS_PIN);

    SPIbus.begin();
    hspi = SPIbus.getHandle();

    hcrc.Instance = CRC;
    if (HAL_CRC_Init(&hcrc) != HAL_OK) {
        Error_Handler();
    }

    HAL_Delay(10); // Wait a bit to make sure the w25qxx is ready

    /* 1. Hardware context --------------------------------------------------- */
    W25qxxStm32Ctx flash_spi_hw = {
        .hspi    = hspi,              // which SPI peripheral
        .cs_port = SPI_CS_GPIO_Port,  // GPIO port used for CS
        .cs_pin  = SPI_CS_Pin         // GPIO pin used for CS
    };

    /* 2. Writable copy of the bus template --------------------------------- */
    w25qxx_bus_t flash_spi_bus;

/* 3. Initialise the flash chip (call once at start-up) ------------------ */

    /* copy the pre-filled callback table then bind our HW context */
    flash_spi_bus = w25qxx_bus_stm32;
    flash_spi_bus.user_ctx = &flash_spi_hw;

    w25qxx.bus = &flash_spi_bus;

    W25QXX_result_t res;
    res = w25qxx_init(&w25qxx);

    if (res == W25QXX_Ok) {
        DBG("W25QXX successfully initialized\n");
        DBG("Manufacturer       = 0x%2x\n", w25qxx.manufacturer_id);
        DBG("Device             = 0x%4x\n", w25qxx.device_id);
        DBG("Block size         = 0x%04lx (%lu)\n", w25qxx.block_size, w25qxx.block_size);
        DBG("Block count        = 0x%04lx (%lu)\n", w25qxx.block_count, w25qxx.block_count);
        DBG("Sector size        = 0x%04lx (%lu)\n", w25qxx.sector_size, w25qxx.sector_size);
        DBG("Sectors per block  = 0x%04lx (%lu)\n", w25qxx.sectors_in_block, w25qxx.sectors_in_block);
        DBG("Page size          = 0x%04lx (%lu)\n", w25qxx.page_size, w25qxx.page_size);
        DBG("Pages per sector   = 0x%04lx (%lu)\n", w25qxx.pages_in_sector, w25qxx.pages_in_sector);
        DBG("Total size (in kB) = 0x%04lx (%lu)\n", (w25qxx.block_count * w25qxx.block_size) / 1024, (w25qxx.block_count * w25qxx.block_size) / 1024);
    } else {
        DBG("Unable to initialize w25qxx\n");
        Error_Handler();
    }

    HAL_Delay(2000);

    uint8_t buf[PAGE_SIZE]; // Buffer the size of a page

    for (uint8_t run = 0; run <= 2; ++run) {

        DBG("\n-------------\nRun %d\n", run);

        DBG("Reading first page");

        res = w25qxx_read(&w25qxx, 0, (uint8_t *) &buf, sizeof(buf));
        if (res == W25QXX_Ok) {
            dump_hex("First page at start", 0, (uint8_t *) &buf, sizeof(buf));
        } else {
            DBG("Unable to read w25qxx\n");
        }

        DBG("Erasing first page");
        if (w25qxx_erase(&w25qxx, 0, sizeof(buf)) == W25QXX_Ok) {
            DBG("Reading first page\n");
            if (w25qxx_read(&w25qxx, 0, (uint8_t *) &buf, sizeof(buf)) == W25QXX_Ok) {
                dump_hex("After erase", 0, (uint8_t *) &buf, sizeof(buf));
            }
        }

        // Create a well known pattern
        fill_buffer(run, buf, sizeof(buf));

        // Write it to device
        DBG("Writing first page\n");
        if (w25qxx_write(&w25qxx, 0, (uint8_t *) &buf, sizeof(buf)) == W25QXX_Ok) {
            // now read it back
            DBG("Reading first page\n");
            if (w25qxx_read(&w25qxx, 0, (uint8_t *) &buf, sizeof(buf)) == W25QXX_Ok) {
                //DBG("  - sum = %lu", get_sum(buf, 256));
                dump_hex("After write", 0, (uint8_t *) &buf, sizeof(buf));
            }
        }
    }

    // Let's do a stress test
    uint32_t start;
    uint32_t sectors = w25qxx.block_count * w25qxx.sectors_in_block; // Entire chip

    DBG("Stress testing w25qxx device: sectors = %lu\n", sectors);

    DBG("Doing chip erase\n");
    start = HAL_GetTick();
    w25qxx_chip_erase(&w25qxx);
    DBG("Done erasing - took %lu ms\n", HAL_GetTick() - start);

    fill_buffer(0, buf, sizeof(buf));

    DBG("Writing all zeroes %lu sectors\n", sectors);
    start = HAL_GetTick();
    for (uint32_t i = 0; i < sectors; ++i) {
        w25qxx_write(&w25qxx, i * w25qxx.sector_size, buf, sizeof(buf));
    }
    DBG("Done writing - took %lu ms\n", HAL_GetTick() - start);

    DBG("Reading %lu sectors\n", sectors);
    start = HAL_GetTick();
    for (uint32_t i = 0; i < sectors; ++i) {
        w25qxx_read(&w25qxx, i * w25qxx.sector_size, buf, sizeof(buf));
    }
    DBG("Done reading - took %lu ms\n", HAL_GetTick() - start);

    DBG("Validating buffer .... ");
    if (check_buffer(0, buf, sizeof(buf))) {
        DBG("OK\n");
    } else {
        DBG("Not OK\n");
    }

    DBG("Doing chip erase\n");
    start = HAL_GetTick();
    w25qxx_chip_erase(&w25qxx);
    DBG("Done erasing - took %lu ms\n", HAL_GetTick() - start);

    fill_buffer(1, buf, sizeof(buf));

    DBG("Writing 10101010 %lu sectors\n", sectors);
    start = HAL_GetTick();
    for (uint32_t i = 0; i < sectors; ++i) {
        w25qxx_write(&w25qxx, i * w25qxx.sector_size, buf, sizeof(buf));
    }
    DBG("Done writing - took %lu ms\n", HAL_GetTick() - start);

    DBG("Reading %lu sectors\n", sectors);
    start = HAL_GetTick();
    for (uint32_t i = 0; i < sectors; ++i) {
        w25qxx_read(&w25qxx, i * w25qxx.sector_size, buf, sizeof(buf));
    }
    DBG("Done reading - took %lu ms\n", HAL_GetTick() - start);

    DBG("Validating buffer ... ");
    if (check_buffer(1, buf, sizeof(buf))) {
        DBG("OK\n");
    } else {
        DBG("Not OK\n");
    }

    DBG("Erasing %lu sectors sequentially\n", sectors);
    start = HAL_GetTick();
    for (uint32_t i = 0; i < sectors; ++i) {
        w25qxx_erase(&w25qxx, i * w25qxx.sector_size, sizeof(buf));
        if ((i > 0) && (i % 100 == 0)) {
            DBG("Done %4lu sectors - total time = %3lu s\n", i, (HAL_GetTick() - start) / 1000);
        }
    }
    DBG("Done erasing - took %lu ms\n", HAL_GetTick() - start);

    uint32_t now = 0, last_blink = 0, last_test = 0, offset_address = 0;

    while (1) {

        now = HAL_GetTick();

        if (now - last_blink >= 500) {

            HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

            last_blink = now;
        }

        if (now - last_test >= 1000) {

            DBG("---------------\nReading page at address     : 0x%08lx\n", offset_address);

            res = w25qxx_read(&w25qxx, offset_address, (uint8_t *) &buf, sizeof(buf));
            if (res == W25QXX_Ok) {
                //dump_hex("First page at start", offset_address, (uint8_t*) &buf, sizeof(buf));
                DBG("Reading old value           : 0x%08lx\n", HAL_CRC_Calculate(&hcrc, (uint32_t *)&buf, sizeof(buf) / 4));
            } else {
                DBG("Unable to read w25qxx\n");
            }

            // DBG("Erasing page");
            if (w25qxx_erase(&w25qxx, offset_address, sizeof(buf)) == W25QXX_Ok) {
                if (w25qxx_read(&w25qxx, offset_address, (uint8_t *) &buf, sizeof(buf)) == W25QXX_Ok) {
                    DBG("After erase                 : 0x%08lx\n", HAL_CRC_Calculate(&hcrc, (uint32_t *)&buf, sizeof(buf) / 4));
                }
            }

            // Create a well known pattern
            fill_buffer(2, buf, sizeof(buf));

            // Write it to device
            DBG("Writing page value          : 0x%08lx\n", HAL_CRC_Calculate(&hcrc, (uint32_t *)&buf, sizeof(buf) / 4));
            if (w25qxx_write(&w25qxx, offset_address, (uint8_t *) &buf, sizeof(buf)) == W25QXX_Ok) {
                // now read it back
                //DBG("Reading page");
                if (w25qxx_read(&w25qxx, offset_address, (uint8_t *) &buf, sizeof(buf)) == W25QXX_Ok) {
                    DBG("Reading back                : 0x%08lx\n", HAL_CRC_Calculate(&hcrc, (uint32_t *)&buf, sizeof(buf) / 4));
                }
            }

            DBG("Test time                   : %lu ms\n", HAL_GetTick() - now);

            offset_address += PAGE_SIZE / 4;

            if (offset_address + PAGE_SIZE > w25qxx.block_count * w25qxx.block_size)
                offset_address = 0;

            last_test = now;
        }
    }
}

// the loop routine runs over and over again forever:
void loop()
{
    while (1);
}
