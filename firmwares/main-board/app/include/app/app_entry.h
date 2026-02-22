#ifndef APP_APP_ENTRY_H
#define APP_APP_ENTRY_H

#ifdef __cplusplus
extern "C" {
#endif

void AppEntry_Init(void);
void AppEntry_Loop(void);
void AppEntry_CreateTasks(void);

#ifdef __cplusplus
}
#endif

#endif
