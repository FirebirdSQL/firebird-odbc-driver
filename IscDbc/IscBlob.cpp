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

// IscBlob.cpp: implementation of the IscBlob class.
//
//////////////////////////////////////////////////////////////////////

#include "stdio.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "IscDbc.h"
#include "Connection.h"
#include "IscBlob.h"
#include "IscResultSet.h"
#include "IscConnection.h"
#include "IscStatement.h"
#include "SQLError.h"

extern short conwBinToHexStr[];

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscBlob::IscBlob()
{
	statement = NULL;
	memset(&blobId,0,sizeof(ISC_QUAD));
	directBlobHandle = NULL;
	fetched = false;
	directBlob = false;
	offset = 0;

	enType = enTypeBlob;
}

IscBlob::IscBlob(IscStatement *stmt, XSQLVAR *var)
{
	directBlob = false;
	bind (stmt, (char*)var->sqldata);
	setType(var->sqlsubtype);
}

IscBlob::~IscBlob()
{

}

void IscBlob::setType(short sqlsubtype)
{
	if ( sqlsubtype == 1 )
		enType = enTypeClob;
	else
		enType = enTypeBlob;
}

void IscBlob::bind(Statement *stmt, char * sqldata)
{
	clear();
	statement = (IscStatement *) stmt;
	blobId = *(ISC_QUAD*) sqldata;
	fetched = false;
	offset = 0;
}

void IscBlob::attach(char * pointBlob, bool bFetched, bool clear)
{
	IscBlob * ptBlob = (IscBlob *)*(intptr_t*)pointBlob;

	statement = ptBlob->statement;
	memcpy(&blobId,&ptBlob->blobId, sizeof(blobId));
	fetched = bFetched;
	Stream::attach(*((Stream *)((BinaryBlob*)ptBlob)),clear);
	offset = 0;
}

int IscBlob::length()
{
	if ( directBlob )
		return directLength;

	if (!fetched)
		fetchBlob();

	return BinaryBlob::getLength();
}

int IscBlob::getSegment(int offset, int length, void * address)
{
	if (!fetched)
		fetchBlob();

	return Stream::getSegment (offset, length, address);
}

void IscBlob::fetchBlob()
{
	ISC_STATUS statusVector [20];
	IscConnection * connection = statement->connection;
	isc_tr_handle transactionHandle = statement->startTransaction();
	isc_blob_handle blobHandle = NULL;

	int ret = connection->GDS->_open_blob2 (statusVector, &connection->databaseHandle, &transactionHandle,
							  &blobHandle, &blobId, 0, NULL);

	if (ret)
		THROW_ISC_EXCEPTION (connection, statusVector);

	char buffer [DEFAULT_BLOB_BUFFER_LENGTH];
	unsigned short length;

	for (;;)
		{
		int ret = connection->GDS->_get_segment (statusVector, &blobHandle, &length, sizeof (buffer), buffer);
		if (ret)
			if (ret == isc_segstr_eof)
				break;
			else if (ret != isc_segment)
				THROW_ISC_EXCEPTION (connection, statusVector);
		putSegment (length, buffer, true);
		}

	connection->GDS->_close_blob (statusVector, &blobHandle);
	blobHandle = NULL;
	fetched = true;
}

char* IscBlob::getString()
{
	if (!fetched)
		fetchBlob();

	return BinaryBlob::getString ();
}

int IscBlob::getSegmentLength(int pos)
{
	if (!fetched)
		fetchBlob();

	return BinaryBlob::getSegmentLength (pos);
}

void* IscBlob::getSegment(int pos)
{
	if (!fetched)
		fetchBlob();

	return BinaryBlob::getSegment (pos);
}

void IscBlob::writeBlob(char * sqldata)
{
	ISC_STATUS statusVector [20];
	IscConnection * connection = statement->connection;
	CFbDll * GDS = connection->GDS;
	isc_blob_handle blobHandle = NULL;
	isc_tr_handle transactionHandle = statement->startTransaction();
	GDS->_create_blob2 ( statusVector, 
					  &connection->databaseHandle,
					  &transactionHandle,
					  &blobHandle,
					  (ISC_QUAD*) sqldata,
					  0, NULL);

	if ( statusVector [1] )
		THROW_ISC_EXCEPTION (connection, statusVector);

	for ( int len, offset = 0; len = getSegmentLength (offset); offset += len )
	{
		GDS->_put_segment ( statusVector, &blobHandle, len, (char*) getSegment (offset));
		if ( statusVector [1] )
			THROW_ISC_EXCEPTION (connection, statusVector);
	}

	GDS->_close_blob ( statusVector, &blobHandle);
	if ( statusVector [1] )
		THROW_ISC_EXCEPTION (connection, statusVector);
}

void IscBlob::writeStreamHexToBlob(char * sqldata)
{
	ISC_STATUS statusVector [20];
	IscConnection * connection = statement->connection;
	CFbDll * GDS = connection->GDS;
	isc_blob_handle blobHandle = NULL;
	isc_tr_handle transactionHandle = statement->startTransaction();
	GDS->_create_blob2 ( statusVector, 
					  &connection->databaseHandle,
					  &transactionHandle,
					  &blobHandle,
					  (ISC_QUAD*) sqldata,
					  0, NULL);

	if ( statusVector [1] )
		THROW_ISC_EXCEPTION (connection, statusVector);

	for ( int len, offset = 0; len = getSegmentLength (offset); offset += len )
	{
		GDS->_put_segment ( statusVector, &blobHandle, len/2, convStrHexToBinary ( (char*)getSegment (offset), len ) );
		if ( statusVector [1] )
			THROW_ISC_EXCEPTION (connection, statusVector);
	}

	GDS->_close_blob ( statusVector, &blobHandle);
	if ( statusVector [1] )
		THROW_ISC_EXCEPTION (connection, statusVector);
}

void IscBlob::writeBlob(char * sqldata, char *data, int length)
{
	ISC_STATUS statusVector [20];
	IscConnection * connection = statement->connection;
	CFbDll * GDS = connection->GDS;
	isc_blob_handle blobHandle = NULL;
	isc_tr_handle transactionHandle = statement->startTransaction();
	GDS->_create_blob2 ( statusVector, 
					  &connection->databaseHandle,
					  &transactionHandle,
					  &blobHandle,
					  (ISC_QUAD*) sqldata,
					  0, NULL );

	if ( statusVector [1] )
		THROW_ISC_EXCEPTION (connection, statusVector);

	if ( length )
	{
		int post = DEFAULT_BLOB_BUFFER_LENGTH;

		while ( length > post )
		{
			GDS->_put_segment ( statusVector, &blobHandle, post, data);
			if ( statusVector [1] )
				THROW_ISC_EXCEPTION ( connection, statusVector );
			data += post;
			length -= post;
		}

		if ( length > 0 )
		{
			GDS->_put_segment ( statusVector, &blobHandle, (unsigned short)length, data);
			if ( statusVector [1] )
				THROW_ISC_EXCEPTION (connection, statusVector);
		}
	}

	GDS->_close_blob ( statusVector, &blobHandle);
	if ( statusVector [1] )
		THROW_ISC_EXCEPTION ( connection, statusVector);
}

void  IscBlob::writeStringHexToBlob(char * sqldata, char *data, int length)
{
	if ( isBlob() )
	{
		Stream::convStrHexToBinary (data, length);
		writeBlob(sqldata, data, length/2);
	}
	else
		writeBlob(sqldata, data, length);
}

//
// Block direct operations reading SQLGetData
//
extern signed int getVaxInteger(const unsigned char * ptr, signed short length);

void IscBlob::directOpenBlob( char * sqldata )
{
	ISC_STATUS statusVector [20];
	IscConnection * connection = statement->connection;
	CFbDll * GDS = connection->GDS;
	fetched = false;

	if ( directBlobHandle )
		GDS->_close_blob (statusVector, &directBlobHandle);

	isc_tr_handle transactionHandle = statement->startTransaction();
	int ret = GDS->_open_blob2 (statusVector, &connection->databaseHandle, &transactionHandle,
							  &directBlobHandle, (ISC_QUAD*) sqldata, 0, NULL);
	if (ret)
		THROW_ISC_EXCEPTION (connection, statusVector);
	
	const char blob_info[] = { isc_info_blob_total_length };
	unsigned char buffer[64];

	ret = GDS->_blob_info ( statusVector, &directBlobHandle, sizeof(blob_info), (char*)blob_info, sizeof(buffer), (char*)buffer);
	if (ret)
		THROW_ISC_EXCEPTION (connection, statusVector);

	unsigned char * p = buffer;

	if ( *p++ == isc_info_blob_total_length )
		directLength = getVaxInteger(p+2, (short)getVaxInteger(p, 2));
	else
		directLength = 0;
	directBlob = true;
	offset = 0;
}

bool IscBlob::directFetchBlob( char * bufData, int lenData, int &lenRead )
{
	ISC_STATUS statusVector [20];
	unsigned short length;
	bool bEndData = false;

	if ( lenData )
	{
		IscConnection * connection = statement->connection;
		CFbDll * GDS = connection->GDS;
		int post = lenData > DEFAULT_BLOB_BUFFER_LENGTH ? DEFAULT_BLOB_BUFFER_LENGTH : lenData;
		char *data = bufData;
		int ret;

		while ( lenData )
		{
			if ( (ret = GDS->_get_segment (statusVector, &directBlobHandle, &length, post, data)) )
			{
				if (ret == isc_segstr_eof)
				{
					directCloseBlob();
					bEndData = true;
					break;
				}
				else if (ret != isc_segment)
					THROW_ISC_EXCEPTION (connection, statusVector);
			}
			data += length;
			lenData -= length;
			if ( lenData < post )
				post = lenData;
		}

		lenRead = data - bufData;
		offset += lenRead;
	}
	return bEndData;
}

bool IscBlob::directGetSegmentToHexStr( char * bufData, int lenData, int &lenRead )
{
	ISC_STATUS statusVector [20];
	unsigned short length;
	bool bEndData = false;

	if ( lenData )
	{
		IscConnection * connection = statement->connection;
		CFbDll * GDS = connection->GDS;
		int post = lenData > DEFAULT_BLOB_BUFFER_LENGTH ? DEFAULT_BLOB_BUFFER_LENGTH : lenData;
		char *data = bufData;
		int ret;

		while ( lenData )
		{
			if ( (ret = GDS->_get_segment (statusVector, &directBlobHandle, &length, post, data)) )
			{
				if (ret == isc_segstr_eof)
				{
					directCloseBlob();
					bEndData = true;
					break;
				}
				else if (ret != isc_segment)
					THROW_ISC_EXCEPTION (connection, statusVector);
			}
			
			short *address = (short*)data + length - 1;
			unsigned char *end = (unsigned char *)data + length - 1;

			data += length*2;
			lenData -= length;
			if ( lenData < post )
				post = lenData;

			while( length-- )
				*address-- = conwBinToHexStr[*end--];
		}

		lenRead = data - bufData;
		offset += lenRead;
	}
	return bEndData;
}

void IscBlob::directCloseBlob()
{
	if ( directBlobHandle )
	{
		ISC_STATUS statusVector [20];
		statement->connection->GDS->_close_blob (statusVector, &directBlobHandle);
		directBlobHandle = NULL;
	}
	fetched = true;
	directBlob = false;
}

//
// Block direct operations at record SQLPutData 
//
void IscBlob::directCreateBlob( char * sqldata )
{
	ISC_STATUS statusVector [20];
	IscConnection * connection = statement->connection;
	CFbDll * GDS = connection->GDS;

	if ( directBlobHandle )
		GDS->_close_blob (statusVector, &directBlobHandle);

	isc_tr_handle transactionHandle = statement->startTransaction();
	GDS->_create_blob2 ( statusVector, 
					  &connection->databaseHandle,
					  &transactionHandle,
					  &directBlobHandle,
					  (ISC_QUAD*) sqldata,
					  0, NULL );

	if ( statusVector [1] )
		THROW_ISC_EXCEPTION (connection, statusVector);
}

void IscBlob::directWriteBlob( char *data, int length )
{
	ISC_STATUS statusVector [20];
	IscConnection * connection = statement->connection;
	CFbDll * GDS = connection->GDS;

	int post = DEFAULT_BLOB_BUFFER_LENGTH;

	while ( length > post )
	{
		GDS->_put_segment ( statusVector, &directBlobHandle, post, data);
		if ( statusVector [1] )
			THROW_ISC_EXCEPTION ( connection, statusVector );
		data += post;
		length -= post;
	}

	if ( length > 0 )
	{
		GDS->_put_segment ( statusVector, &directBlobHandle, (unsigned short)length, data);
		if ( statusVector [1] )
			THROW_ISC_EXCEPTION (connection, statusVector);
	}
}

}; // end namespace IscDbcLibrary
