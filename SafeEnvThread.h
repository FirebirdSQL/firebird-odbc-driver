#if !defined(_SafeEnvThread__INCLUDED_)
#define _SafeEnvThread__INCLUDED_

#if(DRIVER_LOCKED_LEVEL == DRIVER_LOCKED_LEVEL_ENV || DRIVER_LOCKED_LEVEL == DRIVER_LOCKED_LEVEL_CONNECT)

#ifdef _PTHREADS
#include <pthread.h>
#endif

namespace OdbcJdbcLibrary {

class MutexEnvThread
{
public:
#ifdef _WINDOWS
	static void * mutexLockedLevelDll;
#endif
#ifdef _PTHREADS
	static pthread_mutex_t	mutexLockedLevelDll;
#endif

public:
	MutexEnvThread()
	{
#ifdef _WINDOWS
		mutexLockedLevelDll = CreateMutex (NULL, false, NULL);
#endif
#ifdef _PTHREADS
		int ret = pthread_mutex_init (&mutexLockedLevelDll, NULL);
#endif
	}

	~MutexEnvThread()
	{
#ifdef _WINDOWS
		if(mutexLockedLevelDll)
			CloseHandle (mutexLockedLevelDll);
#endif
#ifdef _PTHREADS
		int ret = pthread_mutex_destroy (&mutexLockedLevelDll);
#endif 
	}
};

class SafeDllThread
{
public:
	SafeDllThread()
	{
#ifdef _WINDOWS
		WaitForSingleObject (MutexEnvThread::mutexLockedLevelDll, INFINITE);
#endif
#ifdef _PTHREADS
		pthread_mutex_lock (&MutexEnvThread::mutexLockedLevelDll);
#endif 
	}
	~SafeDllThread()
	{
#ifdef _WINDOWS
		ReleaseMutex (MutexEnvThread::mutexLockedLevelDll);
#endif
#ifdef _PTHREADS
		pthread_mutex_unlock (&MutexEnvThread::mutexLockedLevelDll);
#endif 
	}
};

}; // end namespace OdbcJdbcLibrary

#endif // #if(DRIVER_LOCKED_LEVEL == DRIVER_LOCKED_LEVEL_ENV || DRIVER_LOCKED_LEVEL == DRIVER_LOCKED_LEVEL_CONNECT)

#endif // !defined(_SafeEnvThread__INCLUDED_)
