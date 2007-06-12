#ifndef __SOUND_MGR_H__
#define __SOUND_MGR_H__

#include <cibyl.h>

/* SoundMgr class (this is not in J2ME) */
typedef int NOPH_SoundMgr_t;

NOPH_SoundMgr_t NOPH_SoundMgr_getInstance(void);
int NOPH_SoundMgr_listCreate(NOPH_SoundMgr_t sm, int flags);
int NOPH_SoundMgr_listAdd(NOPH_SoundMgr_t sm, int list, const char* name);
int NOPH_SoundMgr_listCount(NOPH_SoundMgr_t sm, int list);
void NOPH_SoundMgr_listFree(NOPH_SoundMgr_t sm, int list);
void NOPH_SoundMgr_playList(NOPH_SoundMgr_t sm, int list);

#endif /* !__SOUND_MGR_H__ */
