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
 *  The Original Code was created by Vladimir Tsvigun for IBPhoenix.
 *
 *  Copyright (c) 2003 Vladimir Tsvigun
 *  All Rights Reserved.
 */

//  
// IscArray.cpp: IscArray class.
//
//////////////////////////////////////////////////////////////////////

#include "stdio.h"
#include <stdlib.h>
#include <ctype.h>

#include "IscDbc.h"
#include "IscArray.h"
#include "Value.h"
#include "IscConnection.h"
#include "IscStatement.h"
#include "SQLError.h"

#define SKIP_WHITE(p)	while (charTable [*p] == WHITE) ++p
#define SET_INFO_FROM_SUBTYPE( a, b, c ) arrSubTypeElement == 1 ? (a) :  arrSubTypeElement == 2 ? (b) : (c)

#define PUNCT			1
#define WHITE			2
#define DIGIT			4
#define LETTER			8
#define QUOTE			16
#define IDENT			32

namespace IscDbcLibrary {

extern char charTable [];

void CAttrArray::loadAttributes ( IscConnection *connection, char * nameRelation, char * nameFields, int sqlsubtype )
{
	ISC_STATUS statusVector [20];
	void *transactionHandle = connection->startTransaction();

	if ( !connection->GDS->_array_lookup_bounds(statusVector,&connection->databaseHandle, &transactionHandle,
						nameRelation, nameFields, &arrDesc) )
	{
		arrCountElement = 1;
		for(int i = 0; i < arrDesc.array_desc_dimensions; i++)
			arrCountElement = arrCountElement *	(arrDesc.array_desc_bounds[i].array_bound_upper -
					arrDesc.array_desc_bounds[i].array_bound_lower + 1);

// Examples array long: 1,2,3,4 to string '{1,2,3,4}' octetLength = 45
//          array varchar(15): aa,sa,dsd,ww to string '{'aa','sa','dsd','ww'}' octetLength = 73
		arrOctetLength = (arrCountElement - 1)  // + (',' * (count elements - 1) )
						 + 2; // + ( '{','}' ) - size 2

		arrSizeElement = arrDesc.array_desc_length;
		arrTypeElement = arrDesc.array_desc_dtype;

		switch ( arrTypeElement )
		{
		case blr_varying:
		case blr_varying2:
			arrSizeElement += 2;
			arrOctetLength += 2 * arrCountElement; // + ''
			break;
		case blr_cstring:
		case blr_cstring2:
			arrSizeElement += 1;
			arrOctetLength += 2 * arrCountElement; // + ''
			break;
		case blr_text:
		case blr_text2:
			arrOctetLength += 2 * arrCountElement; // + ''
			break;
		}

		arrOctetLength += getPrecisionInternal() * arrCountElement;
		arrBufDataSize = arrSizeElement * arrCountElement;
		arrBufData = NULL;
		arrSubTypeElement = sqlsubtype;
	}
	else
	{
		memset( this, 0, sizeof ( *this) );
		THROW_ISC_EXCEPTION (connection, statusVector);
	}
}

int CAttrArray::getPrecisionInternal()
{
	switch ( arrTypeElement )
	{
	case blr_short:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_SHORT_LENGTH,
										MAX_DECIMAL_SHORT_LENGTH,
										MAX_SMALLINT_LENGTH);

	case blr_long:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_LONG_LENGTH,
										MAX_DECIMAL_LONG_LENGTH,
										MAX_INT_LENGTH);

	case blr_float:
		return MAX_FLOAT_LENGTH;

	case blr_d_float:
	case blr_double:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_DOUBLE_LENGTH,
										MAX_DECIMAL_DOUBLE_LENGTH,
										MAX_DOUBLE_LENGTH);

	case blr_quad:
	case blr_int64:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_LENGTH,
										MAX_DECIMAL_LENGTH,
										MAX_QUAD_LENGTH);

	case blr_sql_time:
		return MAX_TIME_LENGTH;

	case blr_sql_date:
		return MAX_DATE_LENGTH;

	case blr_timestamp:
		return MAX_TIMESTAMP_LENGTH;
	}

	return arrDesc.array_desc_length;
}

int	CAttrArray::getBufferLength()
{
	return arrOctetLength;
//	return arrDesc.array_desc_length * arrCountElement;
}

JString CAttrArray::getFbSqlType()
{
	char temp [30];
	char name [80];
	char * ch = temp;
	char sqlscale = arrDesc.array_desc_scale;
	unsigned short sqllen = arrDesc.array_desc_length;

	switch ( arrTypeElement )
	{
	case blr_short:
		if ( arrSubTypeElement == 1 )
			sprintf (temp, "NUMERIC(%d,%d)", MAX_NUMERIC_SHORT_LENGTH, -sqlscale);
		else if ( arrSubTypeElement == 2 )
			sprintf (temp, "DECIMAL(%d,%d)", MAX_DECIMAL_SHORT_LENGTH, -sqlscale);
		else
			ch = "SMALLINT";
		break;

	case blr_long:
		if ( arrSubTypeElement == 1 )
			sprintf (temp, "NUMERIC(%d,%d)", MAX_NUMERIC_LONG_LENGTH, -sqlscale);
		else if ( arrSubTypeElement == 2 )
			sprintf (temp, "DECIMAL(%d,%d)", MAX_DECIMAL_LONG_LENGTH, -sqlscale);
		else
			ch = "INTEGER";
		break;

	case blr_int64:
		if ( arrSubTypeElement == 1 )
			sprintf (temp, "NUMERIC(%d,%d)", MAX_NUMERIC_LENGTH, -sqlscale);
		else if ( arrSubTypeElement == 2 )
			sprintf (temp, "DECIMAL(%d,%d)", MAX_DECIMAL_LENGTH, -sqlscale);
		else
			ch = "BIGINT";
		break;

	case blr_quad:
		ch = "QUAD";
		break;

	case blr_timestamp:
		ch = "TIMESTAMP";
		break;

	case blr_sql_time:
		ch = "TIME";
		break;

	case blr_sql_date:
		ch = "DATE";
		break;

	case blr_float:
		ch = "FLOAT";
		break;

	case blr_d_float:
	case blr_double:
		if ( arrSubTypeElement == 1 )
			sprintf (temp, "NUMERIC(%d,%d)", MAX_NUMERIC_DOUBLE_LENGTH, -sqlscale);
		else if ( arrSubTypeElement == 2 )
			sprintf (temp, "DECIMAL(%d,%d)", MAX_DECIMAL_DOUBLE_LENGTH, -sqlscale);
		else
			ch = "DOUBLE PRECISION";
		break;

	case blr_text:
	case blr_text2:
		if ( sqllen == 1 )
			ch = "CHAR";
		else
			sprintf (temp, "CHAR(%d)", sqllen);
		break;

	case blr_varying:
	case blr_varying2:
		sprintf (temp, "VARCHAR(%d)", sqllen);
		break;

	case blr_cstring:
	case blr_cstring2:
		sprintf (temp, "CSTRING(%d)", sqllen);
		break;

	default:
		ch = "*unknown type*";
	}

	int len = sprintf (name, "%s[", ch);
	ch = name + len;
	for(int i = 0; i < arrDesc.array_desc_dimensions; i++)
	{
		len = sprintf ( ch , "%d:%d,", arrDesc.array_desc_bounds[i].array_bound_lower
									, arrDesc.array_desc_bounds[i].array_bound_upper);
		ch += len;
	}
	*(ch-1) = ']';
	*ch = '\0';

	return name;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscArray::IscArray()
{
	init();
}

IscArray::IscArray ( CAttrArray * ptArr )
{
	init();
	attach(ptArr);
}

IscArray::IscArray ( IscConnection *connect, XSQLVAR *var )
{
	init();
	connection = connect;
	bind(connect, var);
}

IscArray::~IscArray()
{
	removeBufData();
}

void IscArray::init()
{
	enType = enTypeArray;

	connection = NULL;
	memset ( &arrayId, ~0, sizeof(ISC_QUAD) );
	clearData = false;
	fetched = false;
	fetchedBinary = false;
	memset ( (CAttrArray *)this , 0, sizeof(*(CAttrArray *)this) );
}

void IscArray::attach(CAttrArray * arr, bool fetchBinary, bool bClear)
{
	memcpy ( (CAttrArray *)this, arr, sizeof(*(CAttrArray *)this) );
	fetchedBinary = fetchBinary;
	clearData = bClear;
}

void IscArray::attach(char * pointBlob, bool fetchBinary, bool bClear)
{
	clear();
	CAttrArray * arr = (CAttrArray *)*(long*)pointBlob;
	attach(arr, fetchBinary, bClear);
	fetched = false;
}

void IscArray::detach(CAttrArray * arr)
{
	memcpy ( arr, (CAttrArray *)this, sizeof(*(CAttrArray *)this) );
	clearData = false;
}

void IscArray::bind(IscConnection *connect,XSQLVAR *var)
{
	if ( !memcmp(&arrayId,&*(ISC_QUAD*) var->sqldata,sizeof(arrayId)) )
		return;

	clear();
	removeBufData();

	clearData = true;
	connection = connect;
	arrayId = *(ISC_QUAD*)var->sqldata;
	fetched = false;
	fetchedBinary = false;
	enType = enTypeArray;

	loadAttributes ( connection, var->relname, var->sqlname, var->sqlsubtype );
	arrBufData = (void*)malloc(arrBufDataSize);
}

void IscArray::bind(Connection *connect, char * sqldata)
{
	clear();
	clearData = true;

	arrayId = *(ISC_QUAD*)sqldata;
	fetched = false;
	fetchedBinary = false;
}

void IscArray::removeBufData()
{
	if( arrBufData && clearData)
	{
		free(arrBufData);
		arrBufData = NULL;
	}
}

int IscArray::length()
{
	return arrBufDataSize;
}

void IscArray::getBytesFromArray()
{
	ISC_STATUS statusVector [20];
	void *transactionHandle = connection->startTransaction();
	long lenbuf = arrBufDataSize;

	int ret = connection->GDS->_array_get_slice(statusVector, &connection->databaseHandle, &transactionHandle,
		&arrayId, &arrDesc, arrBufData, &lenbuf);

	if ( ret )
		THROW_ISC_EXCEPTION (connection, statusVector);

	fetchedBinary = true;
}

void IscArray::getBinary(long pos, long length, void * address)
{
	if(!fetchedBinary)
		getBytesFromArray();

	memcpy(address, (char*)arrBufData+pos, length);
}

int IscArray::getLength()
{
	if (!fetched)
		fetchArrayToString();

	return BinaryBlob::length();
}

int IscArray::getSegment(int offset, int length, void * address)
{
	if (!fetched)
		fetchArrayToString();

	return Stream::getSegment (offset, length, address);
}

void IscArray::fetchArrayToString()
{
	if(!fetchedBinary)
		getBytesFromArray();

	fetched = true;

	int i, len;
	char * buf = (char*)malloc(65535u);

	char * ptSrc = (char*)arrBufData;
	char * ptDst = buf;

	switch (arrTypeElement)
	{
	case blr_text :
	case blr_cstring :
		for(i = 0; i < arrCountElement; i++)
		{
			if(!i)memcpy(ptDst,"{\'",2),ptDst+=2;
			else memcpy(ptDst,"\',\'",3),ptDst+=3;
			memcpy(ptDst, ptSrc, arrSizeElement);
			ptDst+=arrSizeElement;
			ptSrc += arrSizeElement;
		}
		if(i)memcpy(ptDst,"\'}",2),ptDst+=2;
		*ptDst='\0';
		break;

	case blr_varying :
		for(i = 0; i < arrCountElement; i++)
		{
			if(!i)memcpy(ptDst,"{\'",2),ptDst+=2;
			else memcpy(ptDst,"\',\'",3),ptDst+=3;
			len = strlen(ptSrc);
			if(len > arrSizeElement-2)
				len = arrSizeElement - 2;
			memcpy(ptDst, ptSrc, len);
			ptDst+=len;
			ptSrc += arrSizeElement;
		}
		if(i)memcpy(ptDst,"\'}",2),ptDst+=2;
		*ptDst='\0';
		break;

	case blr_short :
	case blr_float :
	case blr_long :
	case blr_int64 :
	case blr_double :
		for(i = 0; i < arrCountElement; i++)
		{
			if(!i)*ptDst++='{';
			else *ptDst++=',';
			switch(arrTypeElement)
			{
			case blr_short :
				len=sprintf(ptDst,"%i",*(short*)ptSrc);
				break;
			case blr_long :
				len=sprintf(ptDst,"%i",*(long*)ptSrc);
				break;
			case blr_int64 :
				len=sprintf(ptDst,"%ld",*(__int64*)ptSrc);
				break;
			case blr_float :
				len=sprintf(ptDst,"%.4g",*(float*)ptSrc);
				break;
			case blr_double :
				len=sprintf(ptDst,"%.8g",*(double*)ptSrc);
				break;
			default:
				len=0;
			}
			ptDst+=len;
			ptSrc += arrSizeElement;
		}
		if(i)*ptDst++='}';
		*ptDst='\0';
	}

	len = ptDst - buf;
	putSegment(len, buf, true);
	free(buf);
}

void IscArray::writeBlob(char * sqldata)
{
	ISC_STATUS statusVector [20];
	CFbDll * GDS = connection->GDS;
	void *transactionHandle = connection->startTransaction();

	arrayId = *(ISC_QUAD*)sqldata;
	memset( &arrayId, 0, sizeof(arrayId) );

	long len = getSegmentLength(0);
	GDS->_array_put_slice ( statusVector, &connection->databaseHandle, &transactionHandle,
			&arrayId, &arrDesc, (char*) Stream::getSegment(0), &len );
	if ( statusVector [1] )
		THROW_ISC_EXCEPTION (connection, statusVector);
}

void IscArray::writeBlob(char * sqldata, char *data, long length)
{
	ISC_STATUS statusVector [20];
	CFbDll * GDS = connection->GDS;
	void *transactionHandle = connection->startTransaction();

	arrayId = *(ISC_QUAD*)sqldata;
	memset(&arrayId,0,sizeof(arrayId));

	GDS->_array_put_slice ( statusVector, &connection->databaseHandle, &transactionHandle,
		&arrayId, &arrDesc, data, &length );
	if ( statusVector [1] )
		THROW_ISC_EXCEPTION (connection, statusVector);
}

void IscArray::convStringToArray( char *data, long length )
{
	char *ptCh, *ptSrc = data, *ptEnd = data + length;
	char * ptDst = (char*)arrBufData;
	int i=0;

	memset ( arrBufData, 0, arrBufDataSize);

	while( ptSrc < ptEnd && i<arrCountElement)
	{
		if(*ptSrc=='{' || *ptSrc=='}' || *ptSrc==',')
		{
			ptSrc++;
			continue;
		}

		ptCh=ptSrc;
		SKIP_WHITE(ptCh);

		if(*ptCh=='\'')
		{
			char * pt;
			int len,lenSrc;
			++ptCh;
			while(*ptCh && *ptCh!='\'')
				ptCh++;
			if(*ptCh!='\'')
				break;
			else
				++ptCh;

			lenSrc=ptCh-ptSrc-2; // 2 is this ''
			switch(arrTypeElement)
			{
			case blr_varying: 
				len = arrSizeElement-2;
				if(lenSrc > len)
					lenSrc=len;
				if(lenSrc > 0)
				{
					pt=ptDst;
					++ptSrc; // >> first '
					do
						*pt++=*ptSrc++;
					while(--lenSrc);
					*pt='\0';
				}
				else 
					*ptDst='\0';
				break;
			case blr_text:
				len = arrSizeElement;
				if(lenSrc > len)
					lenSrc=len;
				pt=ptDst;
				if(lenSrc > 0)
				{
					++ptSrc; // >> first '
					do
						*pt++=*ptSrc++;
					while(--lenSrc);
				}

				lenSrc=len-(pt-ptDst);

				for(;lenSrc;--lenSrc)
					*pt++=' ';

				ptDst += arrSizeElement;
				ptSrc=ptCh+1;
				i++;
				continue;
			}
		}

		while(*ptCh && (isdigit(*ptCh) || *ptCh=='.'))
			ptCh++;

		if( ptCh != ptSrc )
		{
			*ptCh='\0';

			switch (arrTypeElement)
			{
			case blr_short :
				*(short*)ptDst = (short)atoi(ptSrc);
				break;
			case blr_long :
				*(long*)ptDst = atol(ptSrc);
				break;
			case blr_quad :
			case blr_int64 :
				*(__int64*)ptDst = (__int64)atol(ptSrc);
				break;
			case blr_float :
				*(float*)ptDst = (float)atof(ptSrc);
				break;
			case blr_double :
				*(double*)ptDst = (double)atof(ptSrc);
				break;
			}

			ptDst += arrSizeElement;
			ptSrc=ptCh+1;
			i++;
		}
		else
			ptSrc++;

	}
}

void IscArray::writeStringHexToBlob(char * sqldata, char *data, long length)
{
	ISC_STATUS statusVector [20];
	CFbDll * GDS = connection->GDS;
	void *transactionHandle = connection->startTransaction();
	ISC_QUAD &arrayId = *(ISC_QUAD*)sqldata;

	memset(&arrayId,0,sizeof(arrayId));

	convStringToArray ( data, length );

	long lenbuf = arrBufDataSize;

	GDS->_array_put_slice ( statusVector, &connection->databaseHandle, &transactionHandle,
		&arrayId, &arrDesc, arrBufData, &lenbuf );
	if ( statusVector [1] )
		THROW_ISC_EXCEPTION (connection, statusVector);
}

void IscArray::writeArray(Value * value)
{

	switch (value->type)
	{
	case BlobPtr:
		{
			char * ptDst = (char*)arrBufData;
			Blob *blob = value->data.blob;
			for (int offset = 0, len; (len = blob->getSegmentLength(offset)); offset += len)
				memcpy(&ptDst[offset],(char*) blob->getSegment (offset),len);
		}
		break;

	case Varchar:
		convStringToArray ( value->data.string.string , value->data.string.length );
		break;

	case String:
		convStringToArray ( value->data.string.string , strlen( value->data.string.string ) );
		break;
	} // End switch (value->type)

	ISC_STATUS statusVector [20];
	void *transactionHandle = connection->startTransaction();
	long lenbuf = arrBufDataSize;

	memset(&arrayId,0,sizeof(arrayId));

	int ret = connection->GDS->_array_put_slice(statusVector, &connection->databaseHandle, &transactionHandle,
		&arrayId, &arrDesc, arrBufData, &lenbuf);
	if (ret || lenbuf != arrBufDataSize)
		THROW_ISC_EXCEPTION (connection, statusVector);
}

}; // end namespace IscDbcLibrary
