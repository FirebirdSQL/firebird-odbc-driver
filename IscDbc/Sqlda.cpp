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
 *				Amended getDisplaySize() and getPrecision()
 *				to return char and varchar lengths more correctly.
 *
 */

// Sqlda.cpp: implementation of the Sqlda class.
//
//////////////////////////////////////////////////////////////////////

#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "IscDbc.h"
#include "Sqlda.h"
#include "SQLError.h"
#include "Value.h"
#include "IscConnection.h"
#include "IscStatement.h"
#include "IscBlob.h"
#include "IscArray.h"

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
	int		indicatorsOffset;
	int		minRow;
	int		maxRow;
	int		curRow;
	short	*numColumnBlob;
	short	countColumnBlob;
	IscConnection	*connection;

public:

	CDataStaticCursor(IscConnection *connect, XSQLDA * sqlda,int * ptOffsetSqldata,int lnRow)
	{
		connection = connect;
		bYesBlob = false;
		ptSqlda = sqlda;
		offsetSqldata = ptOffsetSqldata;
		lenRow = lnRow;
		indicatorsOffset = lenRow - ptSqlda->sqld*sizeof(short);
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
		numColumnBlob = (short *)calloc(1,numberColumns*sizeof(*numColumnBlob));
		countColumnBlob = 0;

		for (n = 0; n < numberColumns; ++n, ++var)
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
			int nRow = 0; 
			for ( i = 0; i < countColumnBlob; ++i )
			{
				XSQLVAR * var = sqlvar + numColumnBlob[i];
				if ( (var->sqltype & ~1) == SQL_ARRAY )
				{
					for (n = 0; n < countBlocks ; ++n)
						if ( listBlocks[n] )
						{
							int l;
							char * pt = listBlocks[n] + (var->sqldata - sqlvar[0].sqldata);
							for ( l = 0; nRow < countAllRows && l < countRowsInBlock[n]; ++l, pt += lenRow, ++nRow)
							{
								if ( pt )
								{
									free ( ((SIscArrayData *)*(long*)pt)->arrBufData );
									delete (SIscArrayData *)*(long*)pt;
								}
							}
						}
				}
				else if ( (var->sqltype & ~1) == SQL_BLOB )
				{
					for (n = 0; n < countBlocks ; ++n)
						if ( listBlocks[n] )
						{
							int l;
							char * pt = listBlocks[n] + (var->sqldata - sqlvar[0].sqldata);
							for ( l = 0; nRow < countAllRows && l < countRowsInBlock[n]; ++l, pt += lenRow, ++nRow)
								if ( pt )delete (IscBlob *)*(long*)pt;
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
		sqlind = (short*)(ptRow + indicatorsOffset + column * sizeof(short));
	}

	char * next()
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
		if ( bYesBlob )
		{
			int n;
			XSQLVAR * sqlvar = ptSqlda->sqlvar;
			for ( n = 0; n < countColumnBlob; ++n )
			{
				XSQLVAR * var = sqlvar + numColumnBlob[n];
				if ( *var->sqlind == -1 )
					*(long*)var->sqldata = (long)0;
				else if ( (var->sqltype & ~1) == SQL_ARRAY )
				{
					SIscArrayData * ptArr = new SIscArrayData;
					IscArray iscArr(connection,var);
					iscArr.getBytesFromArray();
					iscArr.detach(ptArr);
					*(long*)var->sqldata = (long)ptArr;
				}
				else if ( (var->sqltype & ~1) == SQL_BLOB )
				{
					IscBlob * ptBlob = new IscBlob (connection, var);
					ptBlob->fetchBlob();
					*(long*)var->sqldata = (long)ptBlob;
				}
			}
		}
		memcpy(next(),orgBuf,lenRow);
		++countAllRows;
	}

	void operator >> (char * orgBuf)
	{
		memcpy(orgBuf,next(),lenRow);
	}
};
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Sqlda::Sqlda()
{
	sqlda = (XSQLDA*) tempSqlda;
	sqlda->version = SQLDA_VERSION1;
	sqlda->sqln = DEFAULT_SQLDA_COUNT;
	buffer = NULL;
	temps = NULL;
	dataStaticCursor = NULL;
	offsetSqldata = NULL;
	indicatorsOffset = 0;
	saveOrgAdressSqlData = NULL;
	saveOrgAdressSqlInd = NULL;
	needsbuffer = true;
}

Sqlda::~Sqlda()
{
	deleteTemps();
	deleteSqlda();

	if (buffer)
		delete [] buffer;
	if ( dataStaticCursor )
		delete 	dataStaticCursor;
	if ( offsetSqldata )
		delete [] offsetSqldata;
	if ( saveOrgAdressSqlData )
		delete [] saveOrgAdressSqlData;
	if ( saveOrgAdressSqlInd )
		delete [] saveOrgAdressSqlInd;
}

Sqlda::operator XSQLDA* ()
{
	return sqlda;
}

void Sqlda::deleteSqlda()
{
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

void Sqlda::allocBuffer()
{
	//We've already done it,
	// doing it again lengthens SQL_TEXT areas and causes
	// trouble. Contributed by Roger Gammans
	if (!needsbuffer) return;

	needsbuffer = false;

	if (buffer)
	{
		delete [] buffer;
		buffer = NULL;
	}

	if ( offsetSqldata )
		delete [] offsetSqldata;

	int offset = 0;
	int n = 0;
	int numberColumns = sqlda->sqld;
	XSQLVAR *var = sqlda->sqlvar;
	offsetSqldata = new int [numberColumns];

	for (n = 0; n < numberColumns; ++n, ++var)
	{
		int length = var->sqllen;
		int boundary = length;
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

		case SQL_SHORT:
			length = sizeof (short);
			break;

		case SQL_LONG:
		case SQL_TYPE_TIME:
		case SQL_TYPE_DATE:
			length = sizeof (long);
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

		case SQL_ARRAY:
		case SQL_BLOB:
			length = sizeof (ISC_QUAD);
			boundary = 4;
			break;
		}
		if (length == 0)
			throw SQLEXCEPTION (COMPILE_ERROR, "Sqlda variable has zero length");
		offset = ROUNDUP (offset, boundary);
		var->sqldata = (char*)(offsetSqldata[n] = offset);
		var->sqllen = length;
		offset += length;
	}

	offset = ROUNDUP (offset, sizeof (short));
	indicatorsOffset = offset;
	offset += sizeof (short) * numberColumns;
	buffer = new char [offset];
	lengthBufferRows = offset;
	short *indicators = (short*) (buffer + indicatorsOffset);
	var = sqlda->sqlvar;

	for (n = 0; n < numberColumns; ++n, ++var)
		{
		var->sqldata = buffer + (long) var->sqldata;
		var->sqlind = indicators + n;
		}
}

void Sqlda::initStaticCursor(IscConnection *connect)
{
	if ( dataStaticCursor )
		delete 	dataStaticCursor;

	dataStaticCursor = new CDataStaticCursor(connect,sqlda,offsetSqldata,lengthBufferRows);
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

int Sqlda::getCountRowsStaticCursor()
{
	return dataStaticCursor->getCountRowsStaticCursor();
}

int Sqlda::getColumnCount()
{
	return sqlda->sqld;
}

void Sqlda::getSqlData(int index, char *& ptData, short *& ptIndData)
{
	ptData = sqlda->sqlvar[index - 1].sqldata;
	ptIndData = sqlda->sqlvar[index - 1].sqlind;
}

void Sqlda::setSqlData(int index, long ptData, long ptIndData)
{
	saveSqlData(index, ptData, ptIndData);
	sqlda->sqlvar[index - 1].sqldata = (char*)ptData;
	sqlda->sqlvar[index - 1].sqlind = (short*)ptIndData;
}

void Sqlda::saveSqlData(int index, long ptData, long ptIndData)
{
	if ( !saveOrgAdressSqlData )
	{
		int numberColumns = sqlda->sqld;
		saveOrgAdressSqlData = new long [numberColumns];
		memset(saveOrgAdressSqlData,0,sizeof(*saveOrgAdressSqlData)*numberColumns);
		saveOrgAdressSqlInd = new long [numberColumns];
		memset(saveOrgAdressSqlInd,0,sizeof(*saveOrgAdressSqlInd)*numberColumns);
	}
	if ( !saveOrgAdressSqlData[index-1] )
	{
		saveOrgAdressSqlData[index-1] = ptData;
		saveOrgAdressSqlInd[index-1] = ptIndData;
	}
}

void Sqlda::restoreSqlData(int index)
{
	if ( !saveOrgAdressSqlData )
		return;

	int ind = index-1;
	if ( saveOrgAdressSqlData[ind] )
	{
		sqlda->sqlvar[ind].sqldata = (char*)saveOrgAdressSqlData[ind];
		saveOrgAdressSqlData[ind] = 0;
		sqlda->sqlvar[ind].sqlind = (short*)saveOrgAdressSqlInd[ind];
		saveOrgAdressSqlInd[ind] = 0;
	}
}

void Sqlda::print()
{
	XSQLVAR *var = sqlda->sqlvar;

	for (int n = 0; n < sqlda->sqld; ++n, ++var)
		{
		char *p = var->sqldata;
		printf ("%d. type %d, len %d, addr %x (%x) ",
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

				case SQL_SHORT:
					printf ("%d", *(short*) p);
					break;

				case SQL_LONG:
					printf ("%d", *(long*) p);
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
				case SQL_TYPE_TIME:
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

int Sqlda::getColumnDisplaySize(int index)
{
	XSQLVAR *var = sqlda->sqlvar + index - 1;

	switch (var->sqltype & ~1)
		{
		case SQL_SHORT:
			if ( var->sqlscale < 0 )
				return MAX_NUMERIC_LENGTH + 2;
			return MAX_SMALLINT_LENGTH + 1;
			
		case SQL_LONG:
			if ( var->sqlscale < 0 )
				return MAX_NUMERIC_LENGTH + 2;
			return MAX_INT_LENGTH + 1;

		case SQL_FLOAT:
			return MAX_FLOAT_LENGTH + 4;			

		case SQL_D_FLOAT:
		case SQL_DOUBLE:
			if ( var->sqlscale < 0 )
				return MAX_NUMERIC_LENGTH + 2;
			return MAX_DOUBLE_LENGTH + 4;			

		case SQL_QUAD:
		case SQL_INT64:
			if ( var->sqlscale < 0 )
				return MAX_NUMERIC_LENGTH + 2;
			return MAX_QUAD_LENGTH + 1;
			
		case SQL_ARRAY:
			return MAX_ARRAY_LENGTH;

		case SQL_BLOB:
			return MAX_BLOB_LENGTH;

		case SQL_TYPE_TIME:
			return MAX_TIME_LENGTH;

		case SQL_TYPE_DATE:
			return MAX_DATE_LENGTH;

		case SQL_TIMESTAMP:
			return MAX_TIMESTAMP_LENGTH;
        
        case SQL_TEXT: 
            return var->sqllen-1; 

        case SQL_VARYING: 
            return var->sqllen-2;
		}

	return var->sqllen;
}

const char* Sqlda::getColumnName(int index)
{
	XSQLVAR *var = sqlda->sqlvar + index - 1;

	if (var->aliasname [0])
		return var->aliasname;

	return var->sqlname;
}

int Sqlda::getPrecision(int index)
{
	XSQLVAR *var = sqlda->sqlvar + index - 1;

	switch (var->sqltype & ~1)
		{
		case SQL_SHORT:
			if ( var->sqlscale < 0 )
				return MAX_NUMERIC_LENGTH;
			return MAX_SMALLINT_LENGTH;

		case SQL_LONG:
			if ( var->sqlscale < 0 )
				return MAX_NUMERIC_LENGTH;
			return MAX_INT_LENGTH;

		case SQL_FLOAT:
			return MAX_FLOAT_LENGTH;

		case SQL_D_FLOAT:
		case SQL_DOUBLE:
			if ( var->sqlscale < 0 )
				return MAX_NUMERIC_LENGTH;
			return MAX_DOUBLE_LENGTH;

		case SQL_QUAD:
		case SQL_INT64:
			if ( var->sqlscale < 0 )
				return MAX_NUMERIC_LENGTH;
			return MAX_QUAD_LENGTH;

		case SQL_ARRAY:		
			return MAX_ARRAY_LENGTH;
		
		case SQL_BLOB:		
			return MAX_BLOB_LENGTH;

		case SQL_TYPE_TIME:
			return MAX_TIME_LENGTH;

		case SQL_TYPE_DATE:
			return MAX_DATE_LENGTH;

		case SQL_TIMESTAMP:
			return MAX_TIMESTAMP_LENGTH;

        case SQL_TEXT: 
            return var->sqllen-1; 

        case SQL_VARYING: 
            return var->sqllen-2;

		}

	return var->sqllen;
}

int Sqlda::getScale(int index)
{
	XSQLVAR *var = sqlda->sqlvar + index - 1;

	return var->sqlscale;
}

bool Sqlda::isNullable(int index)
{
	XSQLVAR *var = sqlda->sqlvar + index - 1;

	return (var->sqltype & 1) ? true : false;
}

int Sqlda::getColumnType(int index, int &realSqlType)
{
	XSQLVAR *var = sqlda->sqlvar + index - 1;

	return getSqlType (var->sqltype, var->sqlsubtype, var->sqlscale, realSqlType);
}

const char* Sqlda::getColumnTypeName(int index)
{
	XSQLVAR *var = sqlda->sqlvar + index - 1;

	return getSqlTypeName (var->sqltype, var->sqlsubtype, var->sqlscale);
}

int Sqlda::getSubType(int index)
{
	return sqlda->sqlvar[index - 1].sqlsubtype;
}

int Sqlda::getSqlType(int iscType, int subType, int sqlScale, int &realSqlType)
{
	switch (iscType & ~1)
	{
	case SQL_TEXT:
		return (realSqlType = JDBC_CHAR);

	case SQL_VARYING:
		return (realSqlType = JDBC_VARCHAR);

	case SQL_SHORT:
		realSqlType = JDBC_SMALLINT;
		if ( sqlScale < 0 )
		{
			if(subType == 2)
				return JDBC_DECIMAL;
			else
				return JDBC_NUMERIC;
		}
		return realSqlType;

	case SQL_LONG:
		realSqlType = JDBC_INTEGER;
		if ( sqlScale < 0 )
		{
			if(subType == 2)
				return JDBC_DECIMAL;
			else
				return JDBC_NUMERIC;
		}
		return realSqlType;

	case SQL_FLOAT:
		return (realSqlType = JDBC_FLOAT);

	case SQL_DOUBLE:
		realSqlType = JDBC_DOUBLE;
		if ( sqlScale < 0 )
		{
			if(subType == 2)
				return JDBC_DECIMAL;
			else
				return JDBC_NUMERIC;
		}
		return realSqlType;

	case SQL_QUAD:
		return JDBC_BIGINT;

	case SQL_INT64:
		realSqlType = JDBC_BIGINT;
		if ( sqlScale < 0 )
		{
			if(subType == 2)
				return JDBC_DECIMAL;
			else
				return JDBC_NUMERIC;
		}
		return realSqlType;

	case SQL_BLOB:
		if (subType == 1)
			return (realSqlType = JDBC_LONGVARCHAR);
		return (realSqlType = JDBC_LONGVARBINARY);

	case SQL_TIMESTAMP:
		return (realSqlType = JDBC_TIMESTAMP);

	case SQL_TYPE_TIME:
		return (realSqlType = JDBC_TIME);

	case SQL_TYPE_DATE:
		return (realSqlType = JDBC_DATE);

	case SQL_ARRAY:
		return (realSqlType = JDBC_ARRAY);
	}

	return (realSqlType = 0);
}

const char* Sqlda::getSqlTypeName(int iscType, int subType, int sqlScale)
{
	switch (iscType & ~1)
	{
	case SQL_TEXT:
		return "CHAR";

	case SQL_VARYING:
		return "VARCHAR";

	case SQL_SHORT:
		if ( sqlScale < 0 )
		{
			if(subType == 2)
			{
				return "DECIMAL";
			}
			else
			{
				return "NUMERIC";
			}
		}
		return "SMALLINT";

	case SQL_LONG:
		if ( sqlScale < 0 )
		{
			if(subType == 2)
			{
				return "DECIMAL";
			}
			else
			{
				return "NUMERIC";
			}
		}
		return "INTEGER";

	case SQL_FLOAT:
		return "FLOAT";

	case SQL_D_FLOAT:
	case SQL_DOUBLE:
		if ( sqlScale < 0 )
		{
			if(subType == 2)
			{
				return "DECIMAL";
			}
			else
			{
				return "NUMERIC";
			}
		}
		return "DOUBLE PRECISION";

	case SQL_QUAD:
		return "BIGINT";

	case SQL_INT64:
		if ( sqlScale < 0 )
		{
			if(subType == 2)
			{
				return "DECIMAL";
			}
			else
			{
				return "NUMERIC";
			}
		}
		return "BIGINT";

	case SQL_BLOB:
		if (subType == 1)
			return "LONG VARCHAR";
		return "LONG VARBINARY";

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

void Sqlda::setValue(int slot, Value * value, IscConnection *connection)
{
	XSQLVAR *var = sqlda->sqlvar + slot;

	// Check to see if we need to do the conversion.  Otherwise, the
	// InterBase do it.

	switch (var->sqltype & ~1)
		{
		case SQL_BLOB:	
			setBlob (var, value, connection);
			return;
		case SQL_ARRAY:	
			setArray (var, value, connection);
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

		case Short:
			var->sqltype = SQL_SHORT;
			var->sqllen = sizeof (short);
			break;

		case Long:
			var->sqltype = SQL_LONG;
			var->sqllen = sizeof (long);
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
			allocTemp (slot, var->sqllen);
			*(ISC_DATE*) (var->sqldata) = IscStatement::getIscDate (value->data.date);
			break;
									
		case TimeType:
			var->sqltype = SQL_TYPE_TIME;
			var->sqllen = sizeof (ISC_TIME);
			allocTemp (slot, var->sqllen);
			*(ISC_TIME*) (var->sqldata) = IscStatement::getIscTime (value->data.time);
			break;
									
		case Timestamp:
			var->sqltype = SQL_TIMESTAMP;
			var->sqllen = sizeof (ISC_TIMESTAMP);
			allocTemp (slot, var->sqllen);
			*(ISC_TIMESTAMP*) (var->sqldata) = IscStatement::getIscTimeStamp (value->data.timestamp);
			break;
									
		default:
			NOT_YET_IMPLEMENTED;
		}			

}

void Sqlda::setBlob(XSQLVAR * var, Value * value, IscConnection *connection)
{
	if (value->type == Null)
	{
		var->sqltype |= 1;
		*var->sqlind = -1;
		return;
	}

	var->sqltype &= ~1;
	ISC_STATUS statusVector [20];
	isc_blob_handle blobHandle = NULL;
	isc_tr_handle transactionHandle = connection->startTransaction();
	GDS->_create_blob2 (statusVector, 
					  &connection->databaseHandle,
					  &transactionHandle,
					  &blobHandle,
					  (ISC_QUAD*) var->sqldata,
					  0, NULL);

	if (statusVector [1])
		THROW_ISC_EXCEPTION (statusVector);

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
			length = sizeof (long);
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
					THROW_ISC_EXCEPTION (statusVector);
				}
			}
			break;

		case Date:
									
		default:
			NOT_YET_IMPLEMENTED;
		}			

	if (length)
		{
		GDS->_put_segment (statusVector, &blobHandle, length, address);
		if (statusVector [1])
			THROW_ISC_EXCEPTION (statusVector);
		}

	GDS->_close_blob (statusVector, &blobHandle);
	if (statusVector [1])
		THROW_ISC_EXCEPTION (statusVector);
}

void WriteToArray(IscConnection *connect,XSQLVAR *var,Value * value);

void Sqlda::setArray(XSQLVAR * var, Value * value, IscConnection *connection)
{
	if (value->type == Null)
		{
		var->sqltype |= 1;
		*var->sqlind = -1;
		memset (var->sqldata, 0, var->sqllen);
		return;
		}

	var->sqltype &= ~1;
	
	IscArray arr(connection,var);
	arr.writeArray(value);
	*(ISC_QUAD*)var->sqldata=arr.arrayId;
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
	XSQLVAR *var = sqlda->sqlvar + index - 1;

	return var->ownname;
}

void* Sqlda::allocTemp(int index, int length)
{
	if (!temps)
		{
		temps = new TempVector [sqlda->sqld];
		memset (temps, 0, sizeof (struct TempVector) * sqlda->sqld);
		}

	TempVector *temp = temps + index;
		
	if (temp->temp)
		{
		if (temp->length >= length)
			return sqlda->sqlvar [index].sqldata = temp->temp;
		delete [] temp->temp;
		}
	
	temp->length = length;

	return sqlda->sqlvar [index].sqldata = temp->temp = new char [length];	
}

void Sqlda::deleteTemps()
{
	if (temps)
		{
		for (int n = 0; n < sqlda->sqld; ++n)
			if (temps [n].temp)
				delete [] temps [n].temp;
		delete [] temps;
		temps = NULL;
		}
}
