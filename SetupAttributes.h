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
#define MAJOR_VERSION 		1
#define MINOR_VERSION 		3
#define REVNO_VERSION 		0

#define DRIVER_FULL_NAME	"Firebird/InterBase(r) driver"
#define DRIVER_NAME			"OdbcJdbc"

#define SETUP_DSN			"DSN"
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
#define SETUP_QUOTED		"QuotedIdentifier"
#define SETUP_SENSITIVE		"SensitiveIdentifier"
#define SETUP_AUTOQUOTED	"AutoQuotedIdentifier"
#define SETUP_PAGE_SIZE		"PageSize"

#define KEY_DSN_JDBC_DRIVER	"JDBC_DRIVER"
#define KEY_FILEDSN			"FILEDSN"
#define KEY_SAVEDSN			"SAVEDSN"
#define KEY_DSN_DATABASE	"DATABASE"
#define KEY_DSN_UID			"UID"
#define KEY_DSN_PWD			"PWD"
#define KEY_DSN_CHARSET		"CHARSET"
#define KEY_DSN_QUOTED		"QUOTED"

#define LEN_KEY(keydsn) sizeof(keydsn) - 1

#define BUILD_STR(x)	#x
#define BUILD_STR1(x)	x
#define BUILD_STR2(x)   BUILD_STR(x)

#define BUILD_DRIVER_VERSION(major,minor,buildnum) major"."minor"."buildnum
#ifdef __BORLANDC__
#define BUILD_VERSION_STR(major,minor,revno,buildnum) major "." minor "." "0" "." buildnum
#else
#define BUILD_VERSION_STR(major,minor,revno,buildnum) major "." minor "." revno "." buildnum
#endif

#define DRIVER_VERSION		BUILD_DRIVER_VERSION( "0" BUILD_STR2( MAJOR_VERSION ), "0" BUILD_STR2( MINOR_VERSION ), "00" BUILD_STR2( BUILDNUM_VERSION ) )
#define FILE_VERSION		MAJOR_VERSION,MINOR_VERSION,REVNO_VERSION,BUILDNUM_VERSION
#define FILE_VERSION_STR	BUILDTYPE_VERSION BUILD_VERSION_STR( BUILD_STR2( MAJOR_VERSION ), BUILD_STR2( MINOR_VERSION ), BUILD_STR2(REVNO_VERSION), BUILD_STR2(BUILDNUM_VERSION) ) "\0"
#define PRODUCT_VERSION		FILE_VERSION
#define PRODUCT_VERSION_STR	BUILD_VERSION_STR ( BUILD_STR2( MAJOR_VERSION ), BUILD_STR2( MINOR_VERSION ), BUILD_STR2( REVNO_VERSION ), BUILD_STR2( BUILDNUM_VERSION ) ) "\0"

#endif
