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

    SPIbus.begin();
    // hspi = SPIbus.getHandle();

    hcrc.Instance = CRC;
    if (HAL_CRC_Init(&hcrc) != HAL_OK) {
        Error_Handler();
    }

    HAL_Delay(10); // Wait a bit to make sure the w25qxx is ready

    /* 1. Hardware context --------------------------------------------------- */
    W25qxxStm32Ctx flash_spi_hw = {
        .hspi    = &SPIbus,     // SPI peripheral pointer
        .cs_pin  = CS_PIN       // Pin used for CS
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
}

// the loop routine runs over and over again forever:
void loop()
{
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    HAL_Delay(250);
}
