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
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 */

#ifndef __SETUP_ATTRIBUTES_H
#define __SETUP_ATTRIBUTES_H


#include "WriteBuildNo.h"


#define BUILDTYPE_VERSION 	"WI_T"
#define MAJOR_VERSION 		2
#define MINOR_VERSION 		1
#define REVNO_VERSION 		0

#ifdef _WIN64
#define SUFFIX_BUILD		"64"
#else
#define SUFFIX_BUILD		"32"
#endif

#define DRIVER_FULL_NAME	"Firebird ODBC driver"
#define DRIVER_NAME			"OdbcFb"
#define DEFAULT_DRIVER		"IscDbc"

#ifdef _WINDOWS
#define DRIVER_SETUP		"Setup"
#define DRIVER_EXT			".dll"
#else
#define DRIVER_SETUP		"S"
#define DRIVER_EXT			".so"
#endif

#define VALUE_FILE_EXT		"*.fdb,*.gdb"
#define VALUE_API_LEVEL		"1"
#define VALUE_CONNECT_FUN	"YYY"
#define VALUE_FILE_USAGE	"0"
#define VALUE_DRIVER_VER	"03.51"
#define VALUE_SQL_LEVEL		"1"

#define INSTALL_DRIVER		"Driver"
#define INSTALL_SETUP		"Setup"
#define INSTALL_FILE_EXT	"FileExtns"
#define INSTALL_API_LEVEL	"APILevel"
#define INSTALL_CONNECT_FUN	"ConnectFunctions"
#define INSTALL_FILE_USAGE	"FileUsage"
#define INSTALL_DRIVER_VER	"DriverODBCVer"
#define INSTALL_SQL_LEVEL	"SQLLevel"

#define SETUP_DSN			"DSN"
#define SETUP_DESCRIPTION	"Description"
#define SETUP_DBNAME		"Dbname"
#define SETUP_DBNAMEALWAYS	"DbnameAlways"
#define SETUP_CLIENT		"Client"
#define SETUP_DRIVER		"Driver"
#define SETUP_USER			"User"
#define SETUP_PASSWORD		"Password"
#define SETUP_ROLE			"Role"
#define SETUP_CHARSET		"CharacterSet"
#define SETUP_JDBC_DRIVER	"JdbcDriver"
#define SETUP_READONLY_TPB	"ReadOnly"
#define SETUP_NOWAIT_TPB	"NoWait"
#define SETUP_DIALECT		"Dialect"
#define SETUP_TIMEOUT		"Timeout"
#define SETUP_USESCHEMA		"UseSchemaIdentifier"
#define SETUP_QUOTED		"QuotedIdentifier"
#define SETUP_SENSITIVE		"SensitiveIdentifier"
#define SETUP_AUTOQUOTED	"AutoQuotedIdentifier"
#define SETUP_PAGE_SIZE		"PageSize"
#define SETUP_LOCKTIMEOUT	"LockTimeoutWaitTransactions"
#define SETUP_SAFETHREAD    "SafeThread"

#define FLAG_DATABASEACCESS	"DatabaseAccess"

#define KEY_DSN_JDBC_DRIVER	"JDBC_DRIVER"
#define KEY_FILEDSN			"FILEDSN"
#define KEY_SAVEDSN			"SAVEDSN"
#define KEY_DSN_DATABASE	"DATABASE"
#define KEY_DSN_BACKUPFILE	"BACKUPFILE"
#define KEY_DSN_LOGFILE		"LOGFILE"
#define KEY_DSN_CREATE_DB	"CREATE_DB"
#define KEY_DSN_BACKUP_DB	"BACKUP_DB"
#define KEY_DSN_RESTORE_DB	"RESTORE_DB"
#define KEY_DSN_REPAIR_DB	"REPAIR_DB"
#define KEY_DSN_DROP_DB	    "DROP_DB"
#define KEY_DSN_UID			"UID"
#define KEY_DSN_PWD			"PWD"
#define KEY_DSN_CHARSET		"CHARSET"
#define KEY_DSN_QUOTED		"QUOTED"
#define KEY_DSN_SENSITIVE	"SENSITIVE"
#define KEY_DSN_AUTOQUOTED	"AUTOQUOTED"
#define KEY_DSN_USESCHEMA	"USESCHEMA"
#define KEY_DSN_LOCKTIMEOUT	"LOCKTIMEOUT"
#define KEY_DSN_SAFETHREAD	"SAFETHREAD"

#define LEN_KEY(keydsn) sizeof(keydsn) - 1

#define BUILD_STR(x)	#x
#define BUILD_STR1(x)	x
#define BUILD_STR2(x)   BUILD_STR(x)

#if MAJOR_VERSION < 10
#define ZERO_MAJOR "0"
#else
#define ZERO_MAJOR
#endif

#if MINOR_VERSION < 10
#define ZERO_MINOR "0"
#else
#define ZERO_MINOR
#endif

#if BUILDNUM_VERSION   < 100
#define ZERO_BUILDNUM "00"
#elif BUILDNUM_VERSION < 1000
#define ZERO_BUILDNUM "0"
#else
#define ZERO_BUILDNUM
#endif

#define BUILD_DRIVER_VERSION(major,minor,buildnum) major "." minor "." buildnum
#ifdef __BORLANDC__
#define BUILD_VERSION_STR(major,minor,revno,buildnum) major "." minor "." "0" "." buildnum
#else
#define BUILD_VERSION_STR(major,minor,revno,buildnum) major "." minor "." revno "." buildnum
#endif

#define FILE_DESCRIPTION_STR	DRIVER_NAME "\0"
#define INTERNAL_NAME_STR		DRIVER_NAME "\0"
#define ORIGINAL_FILENAME_STR	DRIVER_NAME DRIVER_EXT "\0"

#define DRIVER_VERSION		BUILD_DRIVER_VERSION( ZERO_MAJOR BUILD_STR2( MAJOR_VERSION ), ZERO_MINOR BUILD_STR2( MINOR_VERSION ), ZERO_BUILDNUM BUILD_STR2( BUILDNUM_VERSION ) )
#define FILE_VERSION		MAJOR_VERSION,MINOR_VERSION,REVNO_VERSION,BUILDNUM_VERSION
#define FILE_VERSION_STR	BUILDTYPE_VERSION BUILD_VERSION_STR( BUILD_STR2( MAJOR_VERSION ), BUILD_STR2( MINOR_VERSION ), BUILD_STR2(REVNO_VERSION), BUILD_STR2(BUILDNUM_VERSION) ) "\0"
#define PRODUCT_VERSION		FILE_VERSION
#define PRODUCT_VERSION_STR	BUILD_VERSION_STR ( BUILD_STR2( MAJOR_VERSION ), BUILD_STR2( MINOR_VERSION ), BUILD_STR2( REVNO_VERSION ), BUILD_STR2( BUILDNUM_VERSION ) ) "\0"
#define DRIVER_BUILD_KEY	( MAJOR_VERSION * 1000000 + MINOR_VERSION * 10000 + BUILDNUM_VERSION )

#endif
