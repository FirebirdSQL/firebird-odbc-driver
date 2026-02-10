/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *     Software distributed under the License is distributed on 
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *     express or implied.  See the License for the specific 
 *     language governing rights and limitations under the License.
 *
 *
 *  The Original Code was created by Vladimir Tsvigun for IBPhoenix.
 *
 *  Copyright (c) 2004 Vladimir Tsvigun
 *  All Rights Reserved.
 */

#if !defined(_MAIN_H_INCLUDED_)
#define _MAIN_H_INCLUDED_

void trace (const char *msg);

#ifdef _WINDOWS
#define OUTPUT_MONITOR_EXECUTING(msg)  OutputDebugString(msg"\n");
#else
#define OUTPUT_MONITOR_EXECUTING(msg)
#endif

#ifdef DEBUG
#define TRACE(msg)		trace (msg"\n")
#else
#ifdef __MONITOR_EXECUTING
#define TRACE(msg)		OUTPUT_MONITOR_EXECUTING(msg)
#else
#define TRACE(msg)		
#endif
#endif

// Null handle checks — must be placed BEFORE GUARD_* macros to prevent
// null pointer dereference in DRIVER_LOCKED_LEVEL_CONNECT mode.
// These return SQL_INVALID_HANDLE if the handle is NULL.
#define NULL_CHECK(arg)			do { if (!(arg)) return SQL_INVALID_HANDLE; } while(0)

#if(DRIVER_LOCKED_LEVEL == DRIVER_LOCKED_LEVEL_ENV)

// Even at ENV level, use per-connection locking for statement/descriptor
// operations to avoid false serialization. The global lock is only needed
// for environment-level operations (alloc/free env).
#define GUARD					SafeDllThread wt
#define GUARD_ENV(arg)			NULL_CHECK(arg); GUARD
#define GUARD_HSTMT(arg)		NULL_CHECK(arg); SafeConnectThread wt(((OdbcStatement*)arg)->connection)
#define GUARD_HDBC(arg)			NULL_CHECK(arg); SafeConnectThread wt((OdbcConnection*)arg)
#define GUARD_HDESC(arg)		NULL_CHECK(arg); SafeConnectThread wt(((OdbcDesc*)arg)->connection)
#define GUARD_HTYPE(arg,arg1)	NULL_CHECK(arg); SafeConnectThread wt(												\
									arg1==SQL_HANDLE_DBC ? (OdbcConnection*)arg:					\
									arg1==SQL_HANDLE_STMT ? ((OdbcStatement*)arg)->connection:		\
									arg1==SQL_HANDLE_DESC ? ((OdbcDesc*)arg)->connection : NULL )

#elif(DRIVER_LOCKED_LEVEL == DRIVER_LOCKED_LEVEL_CONNECT)

#define GUARD					SafeDllThread wt
#define GUARD_ENV(arg)			NULL_CHECK(arg); SafeEnvThread wt((OdbcEnv*)arg)
#define GUARD_HSTMT(arg)		NULL_CHECK(arg); SafeConnectThread wt(((OdbcStatement*)arg)->connection)
#define GUARD_HDBC(arg) 		NULL_CHECK(arg); SafeConnectThread wt((OdbcConnection*)arg)
#define GUARD_HDESC(arg)		NULL_CHECK(arg); SafeConnectThread wt(((OdbcDesc*)arg)->connection)
#define GUARD_HTYPE(arg,arg1)	NULL_CHECK(arg); SafeConnectThread wt(												\
									arg1==SQL_HANDLE_DBC ? (OdbcConnection*)arg:					\
									arg1==SQL_HANDLE_STMT ? ((OdbcStatement*)arg)->connection:		\
									arg1==SQL_HANDLE_DESC ? ((OdbcDesc*)arg)->connection : NULL )


#endif

#endif // !defined(_MAIN_H_INCLUDED_)
