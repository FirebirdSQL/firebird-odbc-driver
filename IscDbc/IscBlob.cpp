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

#include "IscDbc.h"
#include "IscBlob.h"
#include "IscResultSet.h"
#include "IscConnection.h"
#include "IscStatement.h"
#include "SQLError.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscBlob::IscBlob(IscConnection *connect, ISC_QUAD *id)
{
	connection = connect;
	blobId = *id;
	fetched = false;
	bArray = false;
}

IscBlob::~IscBlob()
{

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

	int ret = GDS->_open_blob2 (statusVector, &connection->databaseHandle, &transactionHandle,
							  &blobHandle, &blobId, 0, NULL);

	if (ret)
		THROW_ISC_EXCEPTION (statusVector);

	char buffer [10000];
	unsigned short length;

	for (;;)
		{
		int ret = GDS->_get_segment (statusVector, &blobHandle, &length, sizeof (buffer), buffer);
		if (ret)
			if (ret == isc_segstr_eof)
				break;
			else if (ret != isc_segment)
				THROW_ISC_EXCEPTION (statusVector);
		putSegment (length, buffer, true);
		}

	GDS->_close_blob (statusVector, &blobHandle);
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

