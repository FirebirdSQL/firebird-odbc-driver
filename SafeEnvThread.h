#if !defined(_SafeEnvThread__INCLUDED_)
#define _SafeEnvThread__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#if(DRIVER_LOCKED_LEVEL == DRIVER_LOCKED_LEVEL_ENV)

class MutexEnvThread
{
public:
	static void * mutexLockedLevelEnv;

public:
	MutexEnvThread()
	{
#ifdef _WIN32
		mutexLockedLevelEnv = CreateMutex (NULL, false, NULL);
#endif
#ifdef _PTHREADS
		int ret = pthread_mutex_init (&mutexLockedLevelEnv, NULL);
#endif
	}

	~MutexEnvThread()
	{
		if(mutexLockedLevelEnv)
		{
#ifdef _WIN32
			CloseHandle (mutexLockedLevelEnv);
#endif
#ifdef _PTHREADS
			int ret = pthread_mutex_destroy (&mutexLockedLevelEnv);
#endif 
		}
	}
};

class SafeEnvThread
{
public:
	SafeEnvThread()
	{
#ifdef _WIN32
		WaitForSingleObject (MutexEnvThread::mutexLockedLevelEnv, INFINITE);
#endif
#ifdef _PTHREADS
		pthread_mutex_lock (&MutexEnvThread::mutexLockedLevelEnv);
#endif 
	}
	virtual ~SafeEnvThread()
	{
#ifdef _WIN32
		ReleaseMutex (MutexEnvThread::mutexLockedLevelEnv);
#endif
#ifdef _PTHREADS
		pthread_mutex_unlock (&MutexEnvThread::mutexLockedLevelEnv);
#endif 
	}
};

#endif // #if(DRIVER_LOCKED_LEVEL == DRIVER_LOCKED_LEVEL_ENV)

#endif // !defined(_SafeEnvThread__INCLUDED_)
