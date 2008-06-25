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

#if(DRIVER_LOCKED_LEVEL == DRIVER_LOCKED_LEVEL_ENV)

#define GUARD					SafeDllThread wt
#define GUARD_ENV(arg)			GUARD
#define GUARD_HSTMT(arg)		GUARD
#define GUARD_HDBC(arg)			GUARD
#define GUARD_HDESC(arg)		GUARD
#define GUARD_HTYPE(arg1,arg2)	GUARD

#elif(DRIVER_LOCKED_LEVEL == DRIVER_LOCKED_LEVEL_CONNECT)

#define GUARD					SafeDllThread wt
#define GUARD_ENV(arg)			SafeEnvThread wt((OdbcEnv*)arg)
#define GUARD_HSTMT(arg)		SafeConnectThread wt(((OdbcStatement*)arg)->connection)
#define GUARD_HDBC(arg) 		SafeConnectThread wt((OdbcConnection*)arg)
#define GUARD_HDESC(arg)		SafeConnectThread wt(((OdbcDesc*)arg)->connection)
#define GUARD_HTYPE(arg,arg1)	SafeConnectThread wt(												\
									arg1==SQL_HANDLE_DBC ? (OdbcConnection*)arg:					\
									arg1==SQL_HANDLE_STMT ? ((OdbcStatement*)arg)->connection:		\
									arg1==SQL_HANDLE_DESC ? ((OdbcDesc*)arg)->connection : NULL )

#else

#define GUARD
#define GUARD_ENV(arg)
#define GUARD_HSTMT(arg)
#define GUARD_HDBC(arg)
#define GUARD_HDESC(arg)	
#define GUARD_HTYPE(arg1,arg2)

#endif

#endif // !defined(_MAIN_H_INCLUDED_)
