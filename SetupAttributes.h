/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
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

/* 
 * I'm reasonably sure this string should match the one in Setup.cpp 
 * for the declaration of driverInfo. It is only used in one place - 
 *   OdbcConnection::sqlDriverConnect()
 * however, if an attempt is made to create a FileDSN the system 
 * doesn't use 'OdbcJdbc' as the driver string.		PR 2002-06-04
 */
#define DRIVER_FULL_NAME	"Firebird/InterBase(r) driver"
#define DRIVER_NAME			"OdbcJdbc"

#define DRIVER_VERSION		"01.00.0000"
#define SETUP_DSN			"DSN"
#define SETUP_DBNAME		"Dbname"
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
#endif
