#include "diag_log.h"

#include <stdint.h>

int _write(int file, char *ptr, int len)
{
    (void)file;
    if ((ptr == 0) || (len <= 0))
    {
        return 0;
    }

    DiagLog_Write((const uint8_t *)ptr, (uint16_t)len);
    return len;
}
