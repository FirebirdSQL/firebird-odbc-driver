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
 *
 *
 *	2002-08-12	IscMetaDataResultSet.cpp
 *				Added changes from C. G. Alvarez to so that
 *				SQLColAttributes() called with SQL_COLUMN_TYPE_NAME returns 
 *				the name of the type instead of the number of the type
 *
 */

// IscMetaDataResultSet.cpp: implementation of the IscMetaDataResultSet class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include "IscDbc.h"
#include "IscMetaDataResultSet.h"
#include "IscDatabaseMetaData.h"
#include "IscResultSet.h"
#include "IscStatement.h"
#include "SQLError.h"
#include "IscConnection.h"
#include "IscBlob.h"
#include "Value.h"

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscMetaDataResultSet::IscMetaDataResultSet(IscDatabaseMetaData *meta) : IscResultSet (NULL)
{
	metaData = meta;
}

void IscMetaDataResultSet::prepareStatement(const char * sql)
{
	close();
	statement = new IscStatement ( metaData->connection );
	statement->setReadOnlyTransaction();
	statement->prepareStatement (sql);
	statement->execute();
	initResultSet ( statement );

	IscStatement *saveStatement = statement;
	readFromSystemCatalog();
	statement = saveStatement;
}

bool IscMetaDataResultSet::next()
{
	deleteBlobs();
	reset();
	allocConversions();

	if ( !(activePosRowInSet >= 0 && activePosRowInSet < sqlda->getCountRowsStaticCursor()) )
		return false;

	if ( activePosRowInSet )
		copyNextSqldaFromBufferStaticCursor();

	++activePosRowInSet;

	XSQLVAR *var = sqlda->sqlda->sqlvar;
    Value *value = values.values;
	int count = sqlda->sqlda->sqld;

	for ( ; count--; ++var, ++value )
	{
		statement->setValue( value, var );

		if ( *var->sqlind != -1 && (var->sqltype & ~1) == SQL_VARYING )
		{
			int	&length = value->data.string.length;
			char *beg = value->data.string.string;
			char *end = beg + length;
			char *save = end;

			while ( end > beg && *(--end) == ' ');

			if ( save != end )
			{
				length = end - beg + 1;
				*(end+1) = '\0';
			}
		}
	}

	return true;
}

bool IscMetaDataResultSet::isWildcarded(const char * pattern)
{
	for (const char *p = pattern; *p; ++p)
		if (*p == '%' || *p == '\\' || *p == '*')
			return true;

	return false;
}

void IscMetaDataResultSet::expandPattern(char *& stringOut, const char *prefix, const char * string, const char * pattern)
{
	char nameObj [256], * ch;
	const char * ptObj;
	int dialect = metaData->connection->getDatabaseDialect();
	int len;

	if ( dialect == 1 || *metaData->getIdentifierQuoteString() == ' '
		|| metaData->storesUpperCaseIdentifiers() )
	{
		strcpy( nameObj, pattern );
		ch = nameObj;
		while ( (*ch = UPPER ( *ch )) )
			++ch;
		ptObj = nameObj;
	}
	else
		ptObj = pattern;

	if (isWildcarded (pattern))
		len = sprintf (stringOut, "%s (%s like '%s %%' ESCAPE '\\' or %s like '%s' ESCAPE '\\')\n",
							prefix, string, ptObj, string, ptObj);
	else
		len = sprintf (stringOut, "%s %s = \'%s\'\n",prefix, string, ptObj);

	stringOut += len;
}

void IscMetaDataResultSet::addString(char *& stringOut, const char * string, int length)
{
	int len = length ? length : (int)strlen(string);
	memcpy ( stringOut, string, len);
	stringOut += len;
}

void IscMetaDataResultSet::convertBlobToString( int indSrc, int indDst )
{
	XSQLVAR *varDst = sqlda->Var( indDst );
	IscBlob * blob = new IscBlob( statement, varDst );
	blob->fetchBlob();

	*varDst->sqlind = -1;

	int length = blob->length();
	XSQLVAR *varSrc = sqlda->Var( indSrc );
	char * src = varSrc->sqldata + sizeof ( short );
	int lenSrc = varSrc->sqllen;
	*varSrc->sqlind = 0;

	if ( length > lenSrc )
		length = 255;

	blob->getBytes( 0, length, src );
	delete blob;

	*(unsigned short*)varSrc->sqldata = (unsigned short)length;
}

}; // end namespace IscDbcLibrary
