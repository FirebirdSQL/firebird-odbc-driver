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
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 */

#ifndef __ISCJAVATYPE_H_
#define __ISCJAVATYPE_H_

/*
 *		Sql types (actually from java.sql.types)
 */

#define JDBC_NULL		   0 

#define JDBC_BIT 		  -7
#define JDBC_TINYINT 	  -6
#define JDBC_SMALLINT	   5
#define JDBC_INTEGER 	   4
#define JDBC_BIGINT 	  -5

#define JDBC_FLOAT 		   6
#define JDBC_REAL 		   7
#define JDBC_DOUBLE 	   8

#define JDBC_NUMERIC 	   2
#define JDBC_DECIMAL	   3

#define JDBC_CHAR		   1
#define JDBC_VARCHAR 	  12
#define JDBC_LONGVARCHAR  -1

#define JDBC_SQL_DATE 	  9
#define JDBC_SQL_TIME 	  10
#define JDBC_SQL_TIMESTAMP	  11

#define JDBC_DATE 		  91
#define JDBC_TIME 		  92
#define JDBC_TIMESTAMP 	  93

#define JDBC_BINARY		  -2
#define JDBC_VARBINARY 	  -3
#define JDBC_LONGVARBINARY 	  -4

#endif
