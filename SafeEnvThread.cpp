#ifdef _WIN32
#include <windows.h>
#endif

#include "OdbcJdbc.h"
#include "SafeEnvThread.h"

#if(DRIVER_LOCKED_LEVEL == DRIVER_LOCKED_LEVEL_ENV)

void * MutexEnvThread::mutexLockedLevelEnv = NULL;

MutexEnvThread iniMutexEnv;

#endif

