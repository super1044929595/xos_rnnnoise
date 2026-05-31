#ifndef DIAG_LOG_H
#define DIAG_LOG_H

#include <stdint.h>

void DiagLog_Init(void);
void DiagLog_Write(const uint8_t *data, uint16_t len);

#endif /* DIAG_LOG_H */
