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

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscBlob::IscBlob()
{
	connection = NULL;
	memset(&blobId,0,sizeof(ISC_QUAD));
	fetched = false;

	enType = enTypeBlob;
}

IscBlob::IscBlob(IscConnection *connect, XSQLVAR *var)
{
	bind(connect, (char*)var->sqldata);
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

void IscBlob::bind(Connection *connect, char * sqldata)
{
	clear();
	connection = (IscConnection *) connect;
	blobId = *(ISC_QUAD*) sqldata;
	fetched = false;
}

void IscBlob::attach(char * pointBlob, bool bFetched, bool clear)
{
	IscBlob * ptBlob = (IscBlob *)*(long*)pointBlob;

	connection = ptBlob->connection;
	memcpy(&blobId,&ptBlob->blobId, sizeof(blobId));
	fetched = bFetched;
	Stream::attach(*((Stream *)((BinaryBlob*)ptBlob)),clear);
}

int IscBlob::length()
{
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
	void *transactionHandle = connection->startTransaction();
	isc_blob_handle blobHandle = NULL;

	int ret = connection->GDS->_open_blob2 (statusVector, &connection->databaseHandle, &transactionHandle,
							  &blobHandle, &blobId, 0, NULL);

	if (ret)
		THROW_ISC_EXCEPTION (connection, statusVector);

	char buffer [10000];
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
	CFbDll * GDS = connection->GDS;
	isc_blob_handle blobHandle = NULL;
	isc_tr_handle transactionHandle = connection->startTransaction();
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
	CFbDll * GDS = connection->GDS;
	isc_blob_handle blobHandle = NULL;
	isc_tr_handle transactionHandle = connection->startTransaction();
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

void IscBlob::writeBlob(char * sqldata, char *data, long length)
{
	ISC_STATUS statusVector [20];
	CFbDll * GDS = connection->GDS;
	isc_blob_handle blobHandle = NULL;
	isc_tr_handle transactionHandle = connection->startTransaction();
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

void  IscBlob::writeStringHexToBlob(char * sqldata, char *data, long length)
{
	if ( isBlob() )
	{
		Stream::convStrHexToBinary (data, length);
		writeBlob(sqldata, data, length/2);
	}
	else
		writeBlob(sqldata, data, length);
}

}; // end namespace IscDbcLibrary
