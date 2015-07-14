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
 *  2002-11-25	Sqlda.cpp
 *				Contributed by C. G. Alvarez
 *				Changes to support better handling of 
 *				NUMERIC and DECIMAL
 *
 *  2002-10-11	Sqlda.cpp
 *				Contributed by C. G. Alvarez
 *              Extensive modifications to getDisplaySixe()
 *              and getPrecision() to take advantage of MAX_****
 *              constants. Other mods. to getSqlType()
 *
 *  2002-08-12	Sqlda.cpp
 *				Contributed by C. G. Alvarez
 *				Added getColumnTypeName()
 *
 *  2002-08-02	Sqlda.cpp
 *				Contributed by C. G. Alvarez
 *				Change getColumnType to pass var->sqlscale to getSQLType.   
 *				Change getSQLTypeName to keep in sync with this. 
 *				The purpose is to allow return of DECIMAL as JDBC_DECIMAL 
 *				instead of JDBC_BIGINT.
 *
 *	2002-06-04	Sqlda.cpp
 *				Contributed by Robert Milharcic
 *				Amended getColumnDisplaySize() and getPrecision()
 *				to return char and varchar lengths more correctly.
 *
 */

// Sqlda.cpp: implementation of the Sqlda class.
//
//////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include "IscDbc.h"
#include "Sqlda.h"
#include "SQLError.h"
#include "Value.h"
#include "IscConnection.h"
#include "IscStatement.h"
#include "IscBlob.h"

namespace IscDbcLibrary {

#define SET_INFO_FROM_SUBTYPE( a, b, c ) \
		var->sqlsubtype == 1 || (!var->sqlsubtype && var->sqlscale) ? (a) : \
		var->sqlsubtype == 2 ? (b) : (c)

static short sqlNull = -1;

class CDataStaticCursor
{
public:
	int		*offsetSqldata;
	XSQLDA	*ptSqlda;
	bool	bYesBlob;
	int		nMAXROWBLOCK;
	int		lenRow;
	char	**listBlocks;
	int		*countRowsInBlock;
	int		countBlocks;
	int		countAllRows;
	int		curBlock;
	char	*ptRowBlock;
	char	*ptOrgRowBlock;
	int		indicatorsOffset;
	int		minRow;
	int		maxRow;
	int		curRow;
	short	*numColumnBlob;
	short	countColumnBlob;
	IscStatement	*statement;

public:

	CDataStaticCursor ( IscStatement *stmt, XSQLDA * sqlda, int * ptOffsetSqldata, int lnRow)
	{
		statement = stmt;
		bYesBlob = false;
		ptSqlda = sqlda;
		offsetSqldata = ptOffsetSqldata;
		lenRow = lnRow;
		indicatorsOffset = lenRow - ptSqlda->sqld * sizeof(SQLLEN);
		nMAXROWBLOCK = 65535l/lnRow;
		
		if ( nMAXROWBLOCK < 40 )
			nMAXROWBLOCK = 40;

		countBlocks = 10;
		countAllRows = 0;
		listBlocks = (char **)calloc(1,countBlocks*sizeof(*listBlocks));
		countRowsInBlock = (int *)calloc(1,countBlocks*sizeof(*countRowsInBlock));
		ptRowBlock = *listBlocks = (char *)malloc(lenRow*nMAXROWBLOCK);
		curBlock = 0;
		minRow = 0;
		maxRow = *countRowsInBlock = nMAXROWBLOCK;
		curRow = 0;

		int n, numberColumns = ptSqlda->sqld;
		XSQLVAR * var = ptSqlda->sqlvar;
		ptOrgRowBlock = var->sqldata;
		numColumnBlob = (short *)calloc(1,numberColumns*sizeof(*numColumnBlob));
		countColumnBlob = 0;
		char * ptRow = ptRowBlock;
		int	*offset = offsetSqldata;
		SQLLEN *indicators = (SQLLEN*)( ptRow + indicatorsOffset );

		for (n = 0; n < numberColumns; ++n)
		{
			switch (var->sqltype & ~1)
			{
			case SQL_ARRAY:
			case SQL_BLOB:
				if ( !bYesBlob )
					bYesBlob = true;
				numColumnBlob[countColumnBlob++] = n;
				break;
			}
			var->sqldata = ptRow + *offset++;
			*indicators = 0;
			(var++)->sqlind = (short*)indicators++;
		}
		if ( !bYesBlob )
			free( numColumnBlob ),
			numColumnBlob = NULL;
	}

	~CDataStaticCursor()
	{
		int i,n;

		if ( bYesBlob )
		{
			XSQLVAR * sqlvar = ptSqlda->sqlvar;

			for ( i = 0; i < countColumnBlob; ++i )
			{
				XSQLVAR * var = sqlvar + numColumnBlob[i];
				int nRow = 0; 

				if ( (var->sqltype & ~1) == SQL_ARRAY )
				{
					for (n = 0; n < countBlocks ; ++n)
						if ( listBlocks[n] )
						{
							char * pt = listBlocks[n] + (var->sqldata - sqlvar[0].sqldata);
							for (int l = 0; nRow < countAllRows && l < countRowsInBlock[n]; ++l, pt += lenRow, ++nRow)
							{
								if ( pt && *(intptr_t*)pt )
								{
									free ( ((CAttrArray *)*(intptr_t*)pt)->arrBufData );
									delete (CAttrArray *)*(intptr_t*)pt;
								}
							}
						}
				}
				else if ( (var->sqltype & ~1) == SQL_BLOB )
				{
					for (n = 0; n < countBlocks ; ++n)
						if ( listBlocks[n] )
						{
							char * pt = listBlocks[n] + (var->sqldata - sqlvar[0].sqldata);
							for (int l = 0; nRow < countAllRows && l < countRowsInBlock[n]; ++l, pt += lenRow, ++nRow)
								if ( pt && *(intptr_t*)pt )
									delete (IscBlob *)*(intptr_t*)pt;
						}
				}
			}
		}


		for (n = 0; n < countBlocks ; ++n)
			if ( listBlocks[n] )
				free( listBlocks[n] );
		free( listBlocks );
		free( countRowsInBlock );

		if ( numColumnBlob )
			free( numColumnBlob );
	}

	void addRow ()
	{
		if ( bYesBlob )
		{
			int n;
			XSQLVAR * sqlvar = ptSqlda->sqlvar;
			for ( n = 0; n < countColumnBlob; ++n )
			{
				XSQLVAR * var = sqlvar + numColumnBlob[n];
				if ( *var->sqlind == -1 )
					*(intptr_t*)var->sqldata = 0;
				else if ( (var->sqltype & ~1) == SQL_ARRAY )
				{
					CAttrArray * ptArr = new CAttrArray;
					IscArray iscArr(statement,var);
					iscArr.getBytesFromArray();
					iscArr.detach(ptArr);
					*(intptr_t*)var->sqldata = (intptr_t)ptArr;
				}
				else if ( (var->sqltype & ~1) == SQL_BLOB )
				{
					IscBlob * ptBlob = new IscBlob (statement, var);
					ptBlob->fetchBlob();
					*(intptr_t*)var->sqldata = (intptr_t)ptBlob;
				}
			}
		}

		nextPosition();

		XSQLVAR * var = ptSqlda->sqlvar;
		char * ptRow = ptRowBlock;
		int	*offset = offsetSqldata;
		SQLLEN *indicators = (SQLLEN*)( ptRow + indicatorsOffset );
		int n = ptSqlda->sqld;

		while ( n-- )
		{
			var->sqldata = ptRow + *offset++;
			*indicators = 0;
			(var++)->sqlind = (short*)indicators++;
		}

		++countAllRows;
	}

	void restoreOriginalAdressFieldsSqlDa()
	{
		XSQLVAR * var = ptSqlda->sqlvar;
		char * ptRow = ptOrgRowBlock;
		int	*offset = offsetSqldata;
		SQLLEN *indicators = (SQLLEN*)( ptRow + indicatorsOffset );
		int n = ptSqlda->sqld;

		while ( n-- )
		{
			var->sqldata = ptRow + *offset++;
			(var++)->sqlind = (short*)indicators++;
		}
	}

	bool current(int nRow)
	{
		int i, n;

		if( !(nRow >= minRow && nRow < maxRow) )
		{
			for ( i = 0, n = countRowsInBlock[i]; 
						nRow > n && i < countBlocks; 
						n += countRowsInBlock[++i]);
			curBlock = i;
			maxRow = n;
			minRow = maxRow - countRowsInBlock[curBlock];
		}

		curRow = nRow - 1; // We put previous for use next() !!!
		ptRowBlock = listBlocks[curBlock] + (curRow - minRow) * lenRow;

		return true;
	}

	void getAdressFieldFromCurrentRowInBufferStaticCursor(int column, char *& sqldata, short *& sqlind)
	{
		char * ptRow = ptRowBlock + lenRow;
		sqldata = ptRow + offsetSqldata[--column];
		sqlind = (short*)( ptRow + indicatorsOffset + column * sizeof(SQLLEN) );
	}

	char * nextPosition()
	{
		if ( ++curRow < maxRow )
			ptRowBlock += lenRow;
		else
		{
			if ( ++curBlock == countBlocks )
			{
				int newCount = countBlocks+10;
				listBlocks = (char **)realloc(listBlocks,newCount*sizeof(*listBlocks));
				memset(&listBlocks[countBlocks],0,10*sizeof(*listBlocks));
				countRowsInBlock = (int *)realloc(countRowsInBlock,newCount*sizeof(*countRowsInBlock));
				memset(&countRowsInBlock[countBlocks],0,10*sizeof(*countRowsInBlock));
				countBlocks = newCount;
			}
			
			if ( !listBlocks[curBlock] )
			{
				listBlocks[curBlock] = (char *)malloc(lenRow*nMAXROWBLOCK);
				countRowsInBlock[curBlock] = nMAXROWBLOCK;
			}

			ptRowBlock = listBlocks[curBlock];
			minRow = curRow;
			maxRow = minRow + countRowsInBlock[curBlock];
		}

		return ptRowBlock;
	}

	int getCountRowsStaticCursor()
	{
		return countAllRows;
	}

	void operator << (char * orgBuf)
	{
		memcpy(nextPosition(),orgBuf,lenRow);
	}

	void operator >> (char * orgBuf)
	{
		memcpy(orgBuf,nextPosition(),lenRow);
	}

	void copyToBuffer(char * orgBuf)
	{
		memcpy(orgBuf,ptRowBlock,lenRow);
	}

	void copyToCurrentSqlda(char * orgBuf)
	{
		memcpy(ptRowBlock,orgBuf,lenRow);
	}
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction Sqlda
//////////////////////////////////////////////////////////////////////

Sqlda::Sqlda()
{
	init();
}

Sqlda::~Sqlda()
{
	remove();
}

void Sqlda::init()
{
	memset(tempSqlda,0,sizeof(tempSqlda));
	sqlda = (XSQLDA*) tempSqlda;
	sqlda->version = SQLDA_VERSION1;
	sqlda->sqln = DEFAULT_SQLDA_COUNT;
	buffer = NULL;
	orgsqlvar = NULL;
	dataStaticCursor = NULL;
	offsetSqldata = NULL;
	indicatorsOffset = 0;
	needsbuffer = true;
}

void Sqlda::remove()
{
	delete dataStaticCursor;
	delete [] buffer;
	delete [] offsetSqldata;

	deleteSqlda(); // Should stand only here!!!
}

void Sqlda::clearSqlda()
{
	remove();
	init();
}

void Sqlda::deleteSqlda()
{
	delete [] orgsqlvar;

	if (sqlda != (XSQLDA*) tempSqlda)
		free (sqlda);
}

bool Sqlda::checkOverflow()
{
    // sqln: number of fields allocated
    // sqld: actual number of fields

	if (sqlda->sqld <= sqlda->sqln)
		return false;

	int count = sqlda->sqld;
	deleteSqlda();
	sqlda = (XSQLDA*) malloc (XSQLDA_LENGTH (count));
	sqlda->version = SQLDA_VERSION1;
	sqlda->sqln = count;

	needsbuffer = true;

	return true;
}

void Sqlda::allocBuffer ( IscStatement *stmt )
{
	//We've already done it,
	// doing it again lengthens SQL_TEXT areas and causes
	// trouble. Contributed by Roger Gammans
	if (!needsbuffer) return;

	needsbuffer = false;

	delete [] orgsqlvar;
	delete [] buffer;
	delete [] offsetSqldata;

	int offset = 0;
	int n = 0;
	int numberColumns = sqlda->sqld;
	XSQLVAR *var = sqlda->sqlvar;
	offsetSqldata = new int [numberColumns];
	orgsqlvar = new CAttrSqlVar [numberColumns];
	CAttrSqlVar * orgvar = orgsqlvar;

	for (n = 0; n < numberColumns; ++n, ++var, ++orgvar)
	{
		int length = var->sqllen;
		int boundary = length;

		*orgvar = var;

		switch (var->sqltype & ~1)
		{
		case SQL_TEXT:
			boundary = 1;
			++length;
			break;

		case SQL_VARYING:
			boundary = 2;
			length += 2;
			break;

		case SQL_BOOLEAN:
			length = sizeof (TYPE_BOOLEAN);
			break;

		case SQL_SHORT:
			length = sizeof (short);
			break;

		case SQL_LONG:
		case SQL_TYPE_TIME:
		case SQL_TYPE_DATE:
			length = sizeof (int);
			break;

		case SQL_FLOAT:
			length = sizeof (float);
			break;

		case SQL_D_FLOAT:
		case SQL_DOUBLE:
			length = sizeof (double);
			break;

		case SQL_QUAD:
		case SQL_INT64:
		case SQL_TIMESTAMP:
			length = sizeof (QUAD);
			break;

		case SQL_BLOB:
			length = sizeof (ISC_QUAD);
			boundary = sizeof(void*);
			break;

		case SQL_ARRAY:
			orgvar->array = new CAttrArray;
			orgvar->array->loadAttributes ( stmt, var->relname, var->sqlname, var->sqlsubtype );
			length = sizeof (ISC_QUAD);
			boundary = sizeof(void*);
			break;
		}
		if (length == 0)
			throw SQLEXCEPTION (COMPILE_ERROR, "Sqlda variable has zero length");
		offset = ROUNDUP (offset, boundary);
		var->sqldata = (char*)(offsetSqldata[n] = offset);
		offset += length;
	}

	offset = ROUNDUP (offset, sizeof (SQLLEN));
	indicatorsOffset = offset;
	offset += sizeof(SQLLEN) * numberColumns;
	buffer = new char [offset];
	lengthBufferRows = offset;
	SQLLEN *indicators = (SQLLEN*)( buffer + indicatorsOffset );
	var = sqlda->sqlvar;

	for ( n = 0; n < numberColumns; ++n )
	{
		var->sqldata = buffer + (intptr_t) var->sqldata;
		(var++)->sqlind = (short*)indicators;
		*indicators++ = 0;
	}
}

void Sqlda::initStaticCursor(IscStatement *stmt)
{
	if ( dataStaticCursor )
		delete 	dataStaticCursor;

	dataStaticCursor = new CDataStaticCursor( stmt, sqlda, offsetSqldata, lengthBufferRows );
}

void Sqlda::addRowSqldaInBufferStaticCursor()
{
	dataStaticCursor->addRow();
}

void Sqlda::restoreOrgAdressFieldsStaticCursor()
{
	dataStaticCursor->restoreOriginalAdressFieldsSqlDa();
}

bool Sqlda::setCurrentRowInBufferStaticCursor(int nRow)
{
	return dataStaticCursor->current(nRow);
}

void Sqlda::getAdressFieldFromCurrentRowInBufferStaticCursor(int column, char *& sqldata, short *& sqlind)
{
	dataStaticCursor->getAdressFieldFromCurrentRowInBufferStaticCursor(column, sqldata, sqlind);
}

void Sqlda::copyNextSqldaInBufferStaticCursor()
{
	*dataStaticCursor << buffer;
}

void Sqlda::copyNextSqldaFromBufferStaticCursor()
{
	*dataStaticCursor >> buffer;
}

void Sqlda::saveCurrentSqldaToBuffer()
{
	dataStaticCursor->copyToBuffer( buffer );
}

void Sqlda::restoreBufferToCurrentSqlda()
{
	dataStaticCursor->copyToCurrentSqlda( buffer );
}

int Sqlda::getCountRowsStaticCursor()
{
	return dataStaticCursor->getCountRowsStaticCursor();
}

int Sqlda::getColumnCount()
{
	return sqlda->sqld;
}

void Sqlda::print()
{
	XSQLVAR *var = sqlda->sqlvar;

	for (int n = 0; n < sqlda->sqld; ++n, ++var)
		{
		char *p = var->sqldata;
		printf ("%d. type %d, len %d, addr %p (%p) ",
				n, var->sqltype, var->sqllen, p, var->sqlind);
		if ((var->sqltype & 1) && *var->sqlind == -1)
			printf ("<null>");
		else
			switch (var->sqltype & ~1)
				{
				case SQL_TEXT:
					printf ("'%.*s'", var->sqllen, p);
					break;

				case SQL_VARYING:
					printf ("'%.*s'", *(short*) p, p + 2);
					break;

				case SQL_BOOLEAN:
					printf ("%d", *(TYPE_BOOLEAN*) p);
					break;

				case SQL_SHORT:
					printf ("%d", *(short*) p);
					break;

				case SQL_LONG:
					printf ("%ld", *(int*) p);
					break;

				case SQL_FLOAT:
					printf ("%g", *(float*) p);
					break;

				case SQL_D_FLOAT:
				case SQL_DOUBLE:
					printf ("%g", *(double*) p);
					break;

				case SQL_QUAD:
				case SQL_INT64:
					printf ("big");
					break;

				case SQL_BLOB:
					printf ("blob");
					break;

				case SQL_TIMESTAMP:
					printf ("timestamp");
					break;

				case SQL_TYPE_TIME:
					printf ("time");
					break;

				case SQL_TYPE_DATE:
					printf ("date");
					break;

				case SQL_ARRAY:
					printf ("array");
					break;
				}
		printf ("\n");
		}
}

// Warning!
// It's hack, for exclude system filed description
// Return SQLDA for all system field has 31 length 
// and charsetId 3 (UNICODE_FSS) it's error!
//
//	if ( !(var->sqllen % getCharsetSize( var->sqlsubtype )) )
//		return var->sqllen / getCharsetSize( var->sqlsubtype );
//
int Sqlda::getColumnDisplaySize(int index)
{
	CAttrSqlVar *var = orgVar(index);

	switch (var->sqltype & ~1)
	{
	case SQL_TEXT:
		if ( var->sqllen == 1 && var->sqlsubtype == 1 )
			return MAX_TINYINT_LENGTH + 1;
		if ( !(var->sqllen % getCharsetSize( var->sqlsubtype )) )
			return var->sqllen / getCharsetSize( var->sqlsubtype );
		return var->sqllen;

	case SQL_BOOLEAN:
		return MAX_BOOLEAN_LENGTH;

	case SQL_SHORT:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_SHORT_LENGTH + 2,
										MAX_DECIMAL_SHORT_LENGTH + 2,
										MAX_SMALLINT_LENGTH + 1);
		
	case SQL_LONG:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_LONG_LENGTH + 2,
										MAX_DECIMAL_LONG_LENGTH + 2,
										MAX_INT_LENGTH + 1);

	case SQL_FLOAT:
		return MAX_FLOAT_LENGTH + 4;			

	case SQL_D_FLOAT:
	case SQL_DOUBLE:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_DOUBLE_LENGTH + 2,
										MAX_DECIMAL_DOUBLE_LENGTH + 2,
										MAX_DOUBLE_LENGTH + 4);

	case SQL_QUAD:
	case SQL_INT64:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_LENGTH + 2,
										MAX_DECIMAL_LENGTH + 2,
										MAX_QUAD_LENGTH + 1);
		
	case SQL_ARRAY:
		return var->array->arrOctetLength;
//		return MAX_ARRAY_LENGTH;

	case SQL_BLOB:
		return MAX_BLOB_LENGTH;

	case SQL_TYPE_TIME:
		return MAX_TIME_LENGTH;

	case SQL_TYPE_DATE:
		return MAX_DATE_LENGTH;

	case SQL_TIMESTAMP:
		return MAX_TIMESTAMP_LENGTH;
	}

	if ( !(var->sqllen % getCharsetSize( var->sqlsubtype )) )
		return var->sqllen / getCharsetSize( var->sqlsubtype );

	return var->sqllen;
}

const char* Sqlda::getColumnLabel(int index)
{
	XSQLVAR *var = Var(index);

	if (var->aliasname [0])
		return var->aliasname;

	return var->sqlname;
}

const char* Sqlda::getColumnName(int index)
{
	XSQLVAR *var = Var(index);

	return var->sqlname;
}

int Sqlda::getPrecision(int index)
{
	CAttrSqlVar *var = orgVar(index);

	switch (var->sqltype & ~1)
	{
	case SQL_TEXT:
		if ( var->sqllen == 1 && var->sqlsubtype == 1 )
			return MAX_TINYINT_LENGTH;
		if ( !(var->sqllen % getCharsetSize( var->sqlsubtype )) )
			return var->sqllen / getCharsetSize( var->sqlsubtype );
		return var->sqllen;

	case SQL_BOOLEAN:
		return MAX_BOOLEAN_LENGTH;

	case SQL_SHORT:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_SHORT_LENGTH,
										MAX_DECIMAL_SHORT_LENGTH,
										MAX_SMALLINT_LENGTH);

	case SQL_LONG:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_LONG_LENGTH,
										MAX_DECIMAL_LONG_LENGTH,
										MAX_INT_LENGTH);

	case SQL_FLOAT:
		return MAX_FLOAT_LENGTH;

	case SQL_D_FLOAT:
	case SQL_DOUBLE:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_DOUBLE_LENGTH,
										MAX_DECIMAL_DOUBLE_LENGTH,
										MAX_DOUBLE_LENGTH);

	case SQL_QUAD:
	case SQL_INT64:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_LENGTH,
										MAX_DECIMAL_LENGTH,
										MAX_QUAD_LENGTH);

	case SQL_ARRAY:	
		return var->array->arrOctetLength;
//		return MAX_ARRAY_LENGTH;
	
	case SQL_BLOB:		
		return MAX_BLOB_LENGTH;

	case SQL_TYPE_TIME:
		return MAX_TIME_LENGTH;

	case SQL_TYPE_DATE:
		return MAX_DATE_LENGTH;

	case SQL_TIMESTAMP:
		return MAX_TIMESTAMP_LENGTH;
	}

	if ( !(var->sqllen % getCharsetSize( var->sqlsubtype )) )
		return var->sqllen / getCharsetSize( var->sqlsubtype );

	return var->sqllen;
}

int Sqlda::getNumPrecRadix(int index)
{
	CAttrSqlVar *var = orgVar(index);

	switch (var->sqltype & ~1)
	{
	case SQL_SHORT:
	case SQL_LONG:
	case SQL_QUAD:
	case SQL_INT64:
		return 10;
	case SQL_FLOAT:
	case SQL_DOUBLE:
	case SQL_D_FLOAT:
		return 2;
	}

	return 0;
}

int Sqlda::getScale(int index)
{
	CAttrSqlVar *var = orgVar(index);

	switch (var->sqltype & ~1)
	{
	case SQL_TIMESTAMP:
	case SQL_TYPE_TIME:
		return ISC_TIME_SECONDS_PRECISION_SCALE;
	}

	return var->sqlscale;
}

bool Sqlda::isNullable(int index)
{
	CAttrSqlVar *var = orgVar(index);

	return (var->sqltype & 1) ? true : false;
}

int Sqlda::getColumnType(int index, int &realSqlType)
{
	return getSqlType ( orgVar(index), realSqlType );
}

const char* Sqlda::getColumnTypeName(int index)
{
	return getSqlTypeName ( orgVar(index) );
}

short Sqlda::getSubType(int index)
{
	return orgsqlvar[index - 1].sqlsubtype;
}

int Sqlda::getSqlType(CAttrSqlVar *var, int &realSqlType)
{
	switch (var->sqltype & ~1)
	{
	case SQL_TEXT:
		if ( var->sqllen == 1 && var->sqlsubtype == 1 )
			return (realSqlType = JDBC_TINYINT);
		else if ( ( var->sqlsubtype == 3 // UNICODE_FSS
				    || var->sqlsubtype == 4 ) // UTF8
			&& !(var->sqllen % getCharsetSize( var->sqlsubtype )) )
			return (realSqlType = JDBC_WCHAR);
		return (realSqlType = JDBC_CHAR);

	case SQL_VARYING:
		if ( ( var->sqlsubtype == 3 // UNICODE_FSS
				    || var->sqlsubtype == 4 ) // UTF8
			&& !(var->sqllen % getCharsetSize( var->sqlsubtype )) )
			return (realSqlType = JDBC_WVARCHAR);
		return (realSqlType = JDBC_VARCHAR);

	case SQL_BOOLEAN:
		return (realSqlType = JDBC_BOOLEAN);

	case SQL_SHORT:
		realSqlType = JDBC_SMALLINT;
		return SET_INFO_FROM_SUBTYPE ( JDBC_NUMERIC, JDBC_DECIMAL, realSqlType);

	case SQL_LONG:
		realSqlType = JDBC_INTEGER;
		return SET_INFO_FROM_SUBTYPE ( JDBC_NUMERIC, JDBC_DECIMAL, realSqlType);

	case SQL_FLOAT:
		return (realSqlType = JDBC_REAL);

	case SQL_DOUBLE:
		realSqlType = JDBC_DOUBLE;
		return SET_INFO_FROM_SUBTYPE ( JDBC_NUMERIC, JDBC_DECIMAL, realSqlType);

	case SQL_QUAD:
		return JDBC_BIGINT;

	case SQL_INT64:
		realSqlType = JDBC_BIGINT;
		return SET_INFO_FROM_SUBTYPE ( JDBC_NUMERIC, JDBC_DECIMAL, realSqlType);

	case SQL_BLOB:
		if (var->sqlsubtype == 1)
			return (realSqlType = JDBC_LONGVARCHAR);
		return (realSqlType = JDBC_LONGVARBINARY);

	case SQL_TIMESTAMP:
		return (realSqlType = JDBC_TIMESTAMP);

	case SQL_TYPE_TIME:
		return (realSqlType = JDBC_TIME);

	case SQL_TYPE_DATE:
		return (realSqlType = JDBC_DATE);

	case SQL_ARRAY:
		if ( var->array->arrOctetLength < MAX_VARCHAR_LENGTH )
			return (realSqlType = JDBC_VARCHAR);
		return (realSqlType = JDBC_LONGVARCHAR);
	}

	return (realSqlType = 0);
}

const char* Sqlda::getSqlTypeName ( CAttrSqlVar *var )
{
	switch (var->sqltype & ~1)
	{
	case SQL_TEXT:
		if ( var->sqllen == 1 && var->sqlsubtype == 1 )
			return "TINYINT";
		return "CHAR";

	case SQL_VARYING:
		return "VARCHAR";

	case SQL_BOOLEAN:
		return "BOOLEAN";

	case SQL_SHORT:
		return SET_INFO_FROM_SUBTYPE ( "NUMERIC", "DECIMAL", "SMALLINT");

	case SQL_LONG:
		return SET_INFO_FROM_SUBTYPE ( "NUMERIC", "DECIMAL", "INTEGER");

	case SQL_FLOAT:
		return "FLOAT";

	case SQL_D_FLOAT:
	case SQL_DOUBLE:
		return SET_INFO_FROM_SUBTYPE ( "NUMERIC", "DECIMAL", "DOUBLE PRECISION");

	case SQL_QUAD:
		return "BIGINT";

	case SQL_INT64:
		return SET_INFO_FROM_SUBTYPE ( "NUMERIC", "DECIMAL", "BIGINT");

	case SQL_BLOB:
		if ( var->sqlsubtype == 1 )
			return "BLOB SUB_TYPE TEXT";
		return "BLOB SUB_TYPE 0";

	case SQL_TIMESTAMP:
		return "TIMESTAMP";

	case SQL_TYPE_TIME:
		return "TIME";

	case SQL_TYPE_DATE:
		return "DATE";

	case SQL_ARRAY:
		return "ARRAY";

	default:
		NOT_YET_IMPLEMENTED;
	}

	return "*unknown type*";
}

const char* Sqlda::getTableName(int index)
{
	XSQLVAR *var = sqlda->sqlvar + index - 1;

	return var->relname;
}

void Sqlda::setValue(int slot, Value * value, IscStatement	*stmt)
{
	XSQLVAR *var = sqlda->sqlvar + slot;

	// Check to see if we need to do the conversion.  Otherwise, the
	// InterBase do it.

	switch (var->sqltype & ~1)
		{
		case SQL_BLOB:	
			setBlob (var, value, stmt);
			return;
		case SQL_ARRAY:	
			setArray (var, value, stmt);
			return;
		}

	var->sqlscale = 0;
	var->sqldata = (char*) &value->data;
	*var->sqlind = 0;

	switch (value->type)
		{
		case Null:
			var->sqltype |= 1;
			*var->sqlind = sqlNull;
			break;

		case String:
			var->sqltype = SQL_TEXT;
			var->sqldata = value->data.string.string;
			var->sqllen = value->data.string.length;
			break;

		case Boolean:
			var->sqltype = SQL_BOOLEAN;
			var->sqllen = sizeof (TYPE_BOOLEAN);
			break;

		case Short:
			var->sqltype = SQL_SHORT;
			var->sqllen = sizeof (short);
			break;

		case Long:
			var->sqltype = SQL_LONG;
			var->sqllen = sizeof (int);
			break;

		case Quad:
			var->sqltype = SQL_INT64;
			var->sqllen = sizeof (QUAD);
			break;

		case Float:
			var->sqltype = SQL_FLOAT;
			var->sqllen = sizeof (float);
			break;

		case Double:
			var->sqltype = SQL_DOUBLE;
			var->sqllen = sizeof (double);
			break;

		case Date:
			var->sqltype = SQL_TYPE_DATE;
			var->sqllen = sizeof (ISC_DATE);
			*(ISC_DATE*)var->sqldata = IscStatement::getIscDate (value->data.date);
			break;
									
		case TimeType:
			var->sqltype = SQL_TYPE_TIME;
			var->sqllen = sizeof (ISC_TIME);
			*(ISC_TIME*)var->sqldata = IscStatement::getIscTime (value->data.time);
			break;
									
		case Timestamp:
			var->sqltype = SQL_TIMESTAMP;
			var->sqllen = sizeof (ISC_TIMESTAMP);
			*(ISC_TIMESTAMP*)var->sqldata = IscStatement::getIscTimeStamp (value->data.timestamp);
			break;
									
		default:
			NOT_YET_IMPLEMENTED;
		}			

}

void Sqlda::setBlob(XSQLVAR * var, Value * value, IscStatement *stmt)
{
	if (value->type == Null)
	{
		var->sqltype |= 1;
		*var->sqlind = -1;
		return;
	}

	var->sqltype &= ~1;
	ISC_STATUS statusVector [20];
	IscConnection * connection = stmt->connection;
	CFbDll * GDS = connection->GDS;
	isc_blob_handle blobHandle = NULL;
	isc_tr_handle transactionHandle = stmt->startTransaction();
	GDS->_create_blob2 (statusVector, 
					  &connection->databaseHandle,
					  &transactionHandle,
					  &blobHandle,
					  (ISC_QUAD*) var->sqldata,
					  0, NULL);

	if (statusVector [1])
		THROW_ISC_EXCEPTION (connection, statusVector);

	char *address = (char*) &value->data;
	int length = 0;

	switch (value->type)
	{
	case String:
		address = value->data.string.string;
		length = value->data.string.length;
		break;

	case Short:
		length = sizeof (short);
		break;

	case Long:
		length = sizeof (int);
		break;

	case Quad:
		length = sizeof (QUAD);
		break;

	case Float:
		length = sizeof (float);
		break;

	case Double:
		length = sizeof (double);
		break;

	case BlobPtr:
		{
		length = 0;
		Blob *blob = value->data.blob;
		for (int len, offset = 0; len = blob->getSegmentLength (offset); offset += len)
			{
			GDS->_put_segment (statusVector, &blobHandle, len, (char*) blob->getSegment (offset));
			if (statusVector [1])
				THROW_ISC_EXCEPTION (connection, statusVector);
			}
		}
		break;

	case Date:
								
	default:
		NOT_YET_IMPLEMENTED;
	}			

	if ( length )
	{
		int post = DEFAULT_BLOB_BUFFER_LENGTH;

		while ( length > post )
		{
			GDS->_put_segment (statusVector, &blobHandle, post, address);
			if ( statusVector [1] )
				THROW_ISC_EXCEPTION (connection, statusVector);
			address+=post;
			length-=post;
		}

		if( length > 0 )
		{
			GDS->_put_segment (statusVector, &blobHandle, length, address);
			if ( statusVector [1] )
				THROW_ISC_EXCEPTION (connection, statusVector);
		}
	}

	GDS->_close_blob (statusVector, &blobHandle);
	if (statusVector [1])
		THROW_ISC_EXCEPTION (connection, statusVector);
}

void WriteToArray(IscConnection *connect,XSQLVAR *var,Value * value);

void Sqlda::setArray(XSQLVAR * var, Value * value, IscStatement *stmt)
{
	if (value->type == Null)
	{
		var->sqltype |= 1;
		*var->sqlind = -1;
		return;
	}

	var->sqltype &= ~1;
	
	IscArray arr(stmt,var);
	arr.writeArray(value);
	*(ISC_QUAD*)var->sqldata = *arr.arrayId;
}

int Sqlda::findColumn(const char * columnName)
{
	for (int n = 0; n < sqlda->sqld; ++n)
		if (strcasecmp (sqlda->sqlvar [n].sqlname, columnName) == 0)
			return n;

	return -1;
}

const char* Sqlda::getOwnerName(int index)
{
//	XSQLVAR *var = Var(index);
//	return var->ownname;
//	ATTENTION! Fb can not at present execute a design
//	INSERT INTO "OWNER"."EVCT" ("CNT") VALUES (?)
	return "";
}

int Sqlda::isBlobOrArray(int index)
{
	int type = orgVar(index)->sqltype & ~1;

	switch (type)
	{
	case SQL_BLOB:		// 520
	case SQL_ARRAY:		// 540
		return type;
	}

	return 0;
}

bool Sqlda::isNull(int index)
{
	return *(short*)Var(index)->sqlind == -1;
}

void Sqlda::setNull(int index)
{
	*(short*)Var(index)->sqlind = -1;
}

bool Sqlda::getBoolean (int index)
{
	XSQLVAR *var = Var(index);
	CONVERSION_CHECK_DEBUG((orgVar(index)->sqltype & ~1) == SQL_BOOLEAN);
	if ( isNull ( index) )
		return 0;
	return !!*(TYPE_BOOLEAN*)var->sqldata;
}

short Sqlda::getShort (int index)
{
	XSQLVAR *var = Var(index);
	CONVERSION_CHECK_DEBUG((orgVar(index)->sqltype & ~1) == SQL_SHORT);
	if ( isNull ( index) )
		return 0;
	return *(short*)var->sqldata;
}

int Sqlda::getInt (int index)
{
	XSQLVAR *var = Var(index);
	CONVERSION_CHECK_DEBUG((orgVar(index)->sqltype & ~1) == SQL_LONG);
	if ( isNull ( index) )
		return 0;
	return *(int*)var->sqldata;
}

char * Sqlda::getText (int index, int &len)
{
	XSQLVAR *var = Var(index);
	CONVERSION_CHECK_DEBUG((orgVar(index)->sqltype & ~1) == SQL_TEXT);
	if( isNull ( index) )
	{
		len = 0;
		return "";
	}
	len = var->sqllen;
	return var->sqldata;
}

char * Sqlda::getVarying (int index, int &len)
{
	XSQLVAR *var = Var(index);
	CONVERSION_CHECK_DEBUG((orgVar(index)->sqltype & ~1) == SQL_VARYING);
	if( isNull ( index) )
	{
		len = 0;
		return "";
	}
	len = *(short*)var->sqldata;
	return var->sqldata+2;
}

////////////////////////////////////////////////////////

void Sqlda::updateBoolean (int index, bool value)
{
	XSQLVAR *var = Var(index);
	CONVERSION_CHECK_DEBUG((orgVar(index)->sqltype & ~1) == SQL_BOOLEAN);
	*(TYPE_BOOLEAN*)var->sqldata = value;
	*var->sqlind = 0;
}

void Sqlda::updateShort (int index, short value)
{
	XSQLVAR *var = Var(index);
	CONVERSION_CHECK_DEBUG((orgVar(index)->sqltype & ~1) == SQL_SHORT);
	*(short*)var->sqldata = value;
	*var->sqlind = 0;
}

void Sqlda::updateInt (int index, int value)
{
	XSQLVAR *var = Var(index);
	CONVERSION_CHECK_DEBUG((orgVar(index)->sqltype & ~1) == SQL_LONG);
	*(int*)var->sqldata = value;
	*var->sqlind = 0;
}

void Sqlda::updateText (int index, const char* dst)
{
	XSQLVAR *var = Var(index);
	CONVERSION_CHECK_DEBUG((orgVar(index)->sqltype & ~1) == SQL_TEXT);
    char * src = var->sqldata;
	int n = var->sqllen;
	*var->sqlind = 0;

    if ( n < 1)
       return;

    while ( n-- && *dst)
        *src++ = *dst++;

	*src = '\0';
}

void Sqlda::updateVarying (int index, const char* dst)
{
	XSQLVAR *var = Var(index);
	CONVERSION_CHECK_DEBUG((orgVar(index)->sqltype & ~1) == SQL_VARYING);
    char * src = var->sqldata + sizeof(short);
	int n = var->sqllen;
	*var->sqlind = 0;

    if ( n < 1)
       return;

    while ( n-- && *dst)
        *src++ = *dst++;

	*(unsigned short*)var->sqldata = (unsigned short)(var->sqllen - n - 1);
}

}; // end namespace IscDbcLibrary
