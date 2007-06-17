#ifndef __DEVICE_SPECIFIC__
#define __DEVICE_SPECIFIC__

#include <cibyl.h>

typedef int NOPH_DeviceSpecific_t;

void NOPH_DeviceSpecific_init(void);
void NOPH_DeviceSpecific_getPlatform(int addr, int size);

#endif /* !__DEVICE_SPECIFIC__ */
