#ifdef _WIN32
#include <windows.h>
#endif

#include "OdbcJdbc.h"
#include "SafeEnvThread.h"

#if(DRIVER_LOCKED_LEVEL == DRIVER_LOCKED_LEVEL_ENV || DRIVER_LOCKED_LEVEL == DRIVER_LOCKED_LEVEL_CONNECT)

namespace OdbcJdbcLibrary {

#ifdef _WIN32
void * MutexEnvThread::mutexLockedLevelDll = NULL;
#endif
#ifdef _PTHREADS
pthread_mutex_t	MutexEnvThread::mutexLockedLevelDll;
#endif

MutexEnvThread iniMutexEnv;

}; // end namespace OdbcJdbcLibrary

#endif
