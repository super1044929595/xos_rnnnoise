#include "diag_log.h"

#include "main.h"
#include "stm32f4xx_hal_uart.h"

#include <string.h>

extern UART_HandleTypeDef huart1;

static uint8_t s_diag_log_busy;
static uint8_t s_diag_log_buf[256];

void DiagLog_Init(void)
{
    s_diag_log_busy = 0U;
}

void DiagLog_Write(const uint8_t *data, uint16_t len)
{
    if ((data == 0) || (len == 0U))
    {
        return;
    }

    if (len > sizeof(s_diag_log_buf))
    {
        len = sizeof(s_diag_log_buf);
    }

    if (s_diag_log_busy != 0U)
    {
        return;
    }

    memcpy(s_diag_log_buf, data, len);
    s_diag_log_busy = 1U;
    if (HAL_UART_Transmit_DMA(&huart1, s_diag_log_buf, len) != HAL_OK)
    {
        s_diag_log_busy = 0U;
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        s_diag_log_busy = 0U;
    }
}
