#ifndef __COMMAND_MGR_H__
#define __COMMAND_MGR_H__

#include <cibyl.h>

/* CommandMgr class (this is not in J2ME) */
typedef int NOPH_CommandMgr_t;

NOPH_CommandMgr_t NOPH_CommandMgr_getInstance(void);
void NOPH_CommandMgr_addCommand(NOPH_CommandMgr_t cm, char* name, void* callback);
void NOPH_CommandMgr_setResultMem(NOPH_CommandMgr_t cm, int* addr);

#endif /* !__COMMAND_MGR_H__ */
