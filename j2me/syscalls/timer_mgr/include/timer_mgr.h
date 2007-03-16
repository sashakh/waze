#ifndef __TIMER_MGR_H__
#define __TIMER_MGR_H__

#include <cibyl.h>

/* TimerMgr class (this is not in J2ME) */
typedef int NOPH_TimerMgr_t;

NOPH_TimerMgr_t NOPH_TimerMgr_getInstance(void);
int NOPH_TimerMgr_set(NOPH_TimerMgr_t tm, int interval);
void NOPH_TimerMgr_remove(NOPH_TimerMgr_t tm, int index);
int NOPH_TimerMgr_getExpired(NOPH_TimerMgr_t tm);

#endif /* !__TIMER_MGR_H__ */
