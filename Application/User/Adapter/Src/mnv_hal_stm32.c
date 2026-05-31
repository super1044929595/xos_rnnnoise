#include "main.h"

#include "mnv_hal.h"

#include <string.h>

uint8_t mnv_hal_flash_read_byte(const uint8_t *flash_ptr)
{
    return *flash_ptr;
}

void mnv_hal_flash_read_block(const uint8_t *flash_src, uint8_t *sram_dst, uint16_t len)
{
    memcpy(sram_dst, flash_src, len);
}

void mnv_hal_fatal(void)
{
    Error_Handler();
}

void mnv_hal_critical_enter(void)
{
    __disable_irq();
}

void mnv_hal_critical_exit(void)
{
    __enable_irq();
}
