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
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "IscDbc.h"
#include "IscArray.h"
#include "Value.h"
#include "IscConnection.h"
#include "IscStatement.h"
#include "SQLError.h"

#define SKIP_WHITE(p)	while (charTable [*p] == WHITE) ++p

#define PUNCT			1
#define WHITE			2
#define DIGIT			4
#define LETTER			8
#define QUOTE			16
#define IDENT			32

namespace IscDbcLibrary {

extern char charTable [];
//
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
IscArray::IscArray(SIscArrayData * ptArr)
{
	attach(ptArr);
	connection = NULL;
	fetched = false;
	fetchedBinary = true;
	enType = enTypeArray;
}

IscArray::IscArray(IscConnection *connect,XSQLVAR *var)
{
	clear = true;
	connection = connect;
	arrayId = *(ISC_QUAD*)var->sqldata;
	fetched = false;
	fetchedBinary = false;
	enType = enTypeArray;

	int i;
	ISC_STATUS statusVector [20];
	void *transactionHandle = connection->startTransaction();

	int ret = connection->GDS->_array_lookup_bounds(statusVector,&connection->databaseHandle, &transactionHandle,
		var->relname,var->sqlname, &arrDesc);
	if (ret)
		THROW_ISC_EXCEPTION (connection, statusVector);

	arrBufData = NULL;

	// Computes total number of elements in the array or slice
	arrCountElement = 1;
	for(i = 0; i < arrDesc.array_desc_dimensions; i++)
		arrCountElement = arrCountElement *	(arrDesc.array_desc_bounds[i].array_bound_upper -
				arrDesc.array_desc_bounds[i].array_bound_lower + 1);

	arrSizeElement = arrDesc.array_desc_length;
	arrTypeElement = arrDesc.array_desc_dtype;

	if (arrTypeElement == blr_varying)
		arrSizeElement += 2;
	else if (arrTypeElement == blr_cstring)
		arrSizeElement += 1;

	arrBufDataSize = arrSizeElement * arrCountElement;
	arrBufData = (void*)malloc(arrBufDataSize);
}

IscArray::~IscArray()
{
	removeBufData();
}

void IscArray::attach(SIscArrayData * arr, bool bClear)
{
	arrBufData = arr->arrBufData;
	arrBufDataSize = arr->arrBufDataSize;
	arrCountElement = arr->arrCountElement;
	arrSizeElement = arr->arrSizeElement;
	arrTypeElement = arr->arrTypeElement;
	fetchedBinary = true;
	clear = bClear;
}

void IscArray::detach(SIscArrayData * arr)
{
	arr->arrBufData = arrBufData;
	arr->arrBufDataSize = arrBufDataSize;
	arr->arrCountElement = arrCountElement;
	arr->arrSizeElement = arrSizeElement;
	arr->arrTypeElement = arrTypeElement;
	clear = false;
}

void IscArray::removeBufData()
{
	if( arrBufData && clear)
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

void IscArray::getBytes(long pos, long length, void * address)
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

void IscArray::writeArray(Value * value)
{
	int i,offset;
//	bool bString;
	char * ptCh, * ptSrc;
	char * ptDst = (char*)arrBufData;

	switch (value->type)
	{
	case BlobPtr:
		{
			int len;
			Blob *blob = value->data.blob;
			for (offset = 0; len = blob->getSegmentLength(offset), len; offset += len)
				memcpy(&ptDst[offset],(char*) blob->getSegment (offset),len);
		}
		break;
	case Varchar:
		value->data.string.string[value->data.string.length]='\0';
	case String:
		ptSrc = value->data.string.string;
		i=0;
		while(*ptSrc && i<arrCountElement)
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
//				bString = true;
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
//			else 
//				bString = false;
//
			while(*ptCh && (isdigit(*ptCh) || *ptCh=='.'))
				ptCh++;
			if(ptCh!=ptSrc)
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
