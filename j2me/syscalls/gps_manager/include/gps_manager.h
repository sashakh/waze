#ifndef __GPS_MANAGER_H__
#define __GPS_MANAGER_H__

#include <cibyl.h>
#include <javax/microedition/midlet.h>

/* GpsManager class (this is not in J2ME) */
typedef int NOPH_GpsManager_t;

NOPH_GpsManager_t NOPH_GpsManager_getInstance(void);
void NOPH_GpsManager_searchGps(NOPH_GpsManager_t gm, NOPH_MIDlet_t m, const char* wait_msg, const char* not_found_msg);
int NOPH_GpsManager_connect(NOPH_GpsManager_t gm, const char* url);
void NOPH_GpsManager_disconnect(NOPH_GpsManager_t gm);
void NOPH_GpsManager_start(NOPH_GpsManager_t gm);
void NOPH_GpsManager_stop(NOPH_GpsManager_t gm);
int NOPH_GpsManager_read(NOPH_GpsManager_t gm, void* buffer, int size);
int NOPH_GpsManager_getURL(NOPH_GpsManager_t gm, void* buffer, int size);

#endif /* !__GPS_MANAGER_H__ */
