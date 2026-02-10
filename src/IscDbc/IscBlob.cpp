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

using namespace Firebird;

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

IscBlob::IscBlob(IscStatement *stmt, char* buf, short sqlsubtype)
{
	directBlob = false;
	bind (stmt, buf);
	setType(sqlsubtype);
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
	IscConnection * connection = statement->connection;
	ITransaction* transactionHandle = statement->startTransaction();
	IBlob* blobHandle = nullptr;

	ThrowStatusWrapper status( connection->GDS->_status );
	try
	{
		blobHandle = connection->databaseHandle->openBlob( &status, transactionHandle, &blobId, 0, NULL );

		char buffer [DEFAULT_BLOB_BUFFER_LENGTH];
		unsigned int length;

		for (;;)
		{
			auto res = blobHandle->getSegment( &status, sizeof (buffer), buffer, &length );

			const bool keep_reading = ( res == IStatus::RESULT_OK || res == IStatus::RESULT_SEGMENT );
			if( !keep_reading ) break;

			putSegment (length, buffer, true);
		}

		blobHandle->close( &status );
		blobHandle = nullptr;
		fetched = true;
	}
	catch( const FbException& error )
	{
		if( blobHandle ) blobHandle->release();
		THROW_ISC_EXCEPTION ( connection, error.getStatus() );
	}
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
	IscConnection * connection = statement->connection;
	CFbDll * GDS = connection->GDS;
	IBlob* blobHandle = nullptr;
	ITransaction* transactionHandle = statement->startTransaction();

	ThrowStatusWrapper status( GDS->_status );
	try
	{
		blobHandle = connection->databaseHandle->createBlob( &status, transactionHandle, (ISC_QUAD*)sqldata, 0, NULL );

		for ( int len, offset = 0; len = getSegmentLength (offset); offset += len )
		{
			blobHandle->putSegment( &status, len, (char*) getSegment (offset) );
		}

		blobHandle->close( &status );
		blobHandle = nullptr;
	}
	catch( const FbException& error )
	{
		if( blobHandle ) blobHandle->release();
		THROW_ISC_EXCEPTION ( connection, error.getStatus() );
	}
}

void IscBlob::writeStreamHexToBlob(char * sqldata)
{
	IscConnection * connection = statement->connection;
	CFbDll * GDS = connection->GDS;
	IBlob* blobHandle = NULL;
	ITransaction* transactionHandle = statement->startTransaction();

	ThrowStatusWrapper status( GDS->_status );
	try
	{
		blobHandle = connection->databaseHandle->createBlob( &status, transactionHandle, (ISC_QUAD*)sqldata, 0, NULL );

		for ( int len, offset = 0; len = getSegmentLength (offset); offset += len )
		{
			blobHandle->putSegment( &status, len/2, convStrHexToBinary ( (char*)getSegment (offset), len ) );
		}

		blobHandle->close( &status );
		blobHandle = nullptr;
	}
	catch( const FbException& error )
	{
		if( blobHandle ) blobHandle->release();
		THROW_ISC_EXCEPTION ( connection, error.getStatus() );
	}
}

void IscBlob::writeBlob(char * sqldata, char *data, int length)
{
	IscConnection * connection = statement->connection;
	CFbDll * GDS = connection->GDS;
	IBlob* blobHandle = nullptr;
	ITransaction* transactionHandle = statement->startTransaction();

	ThrowStatusWrapper status( GDS->_status );
	try
	{
		blobHandle = connection->databaseHandle->createBlob( &status, transactionHandle, (ISC_QUAD*)sqldata, 0, NULL );

		for ( int len, offset = 0; len = getSegmentLength (offset); offset += len )
		{
			blobHandle->putSegment( &status, len/2, convStrHexToBinary ( (char*)getSegment (offset), len ) );
		}

		if ( length )
		{
			int post = DEFAULT_BLOB_BUFFER_LENGTH;

			while ( length > post )
			{
				blobHandle->putSegment( &status, post, data );
				data += post;
				length -= post;
			}

			if ( length > 0 )
			{
				blobHandle->putSegment( &status, (unsigned short)length, data );
			}
		}

		blobHandle->close( &status );
		blobHandle = nullptr;
	}
	catch( const FbException& error ) {
		if( blobHandle ) blobHandle->release();
		THROW_ISC_EXCEPTION ( connection, error.getStatus() );
	}
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
	IscConnection * connection = statement->connection;
	CFbDll * GDS = connection->GDS;
	fetched = false;

	ThrowStatusWrapper status( GDS->_status );

	if ( directBlobHandle )
		try
		{
			directBlobHandle->close( &status );
			directBlobHandle = nullptr;
		}
		catch( ... ) {
			if( directBlobHandle ) directBlobHandle->release();
		}

	ITransaction* transactionHandle = statement->startTransaction();

	try
	{
		directBlobHandle = connection->databaseHandle->openBlob( &status, transactionHandle, (ISC_QUAD*) sqldata, 0, NULL );

		const char blob_info[] = { isc_info_blob_total_length };
		unsigned char buffer[64];

		directBlobHandle->getInfo( &status, sizeof(blob_info), (const unsigned char*)blob_info, sizeof(buffer), (unsigned char*)buffer );

		unsigned char * p = buffer;

		if ( *p++ == isc_info_blob_total_length )
			directLength = getVaxInteger(p+2, (short)getVaxInteger(p, 2));
		else
			directLength = 0;
		directBlob = true;
		offset = 0;
	}
	catch( const FbException& error )
	{
		THROW_ISC_EXCEPTION ( connection, error.getStatus() );
	}
}

bool IscBlob::directFetchBlob( char * bufData, int lenData, int &lenRead )
{
	unsigned length;
	bool bEndData = false;

	if ( lenData )
	{
		IscConnection * connection = statement->connection;
		CFbDll * GDS = connection->GDS;
		int post = lenData > DEFAULT_BLOB_BUFFER_LENGTH ? DEFAULT_BLOB_BUFFER_LENGTH : lenData;
		char *data = bufData;
		ThrowStatusWrapper status( GDS->_status );

		try
		{
			while ( lenData )
			{
				auto res = directBlobHandle->getSegment( &status, post, data, &length );
				const bool keep_reading = ( res == IStatus::RESULT_OK || res == IStatus::RESULT_SEGMENT );
				if( !keep_reading ) break;

				data += length;
				lenData -= length;
				if ( lenData < post )
					post = lenData;
			}

			lenRead = data - bufData;
			offset += lenRead;
		}
		catch( const FbException& error )
		{
			THROW_ISC_EXCEPTION ( connection, error.getStatus() );
		}
	}
	return bEndData;
}

bool IscBlob::directGetSegmentToHexStr( char * bufData, int lenData, int &lenRead )
{
	unsigned length;
	bool bEndData = false;

	if ( lenData )
	{
		IscConnection * connection = statement->connection;
		CFbDll * GDS = connection->GDS;
		int post = lenData > DEFAULT_BLOB_BUFFER_LENGTH ? DEFAULT_BLOB_BUFFER_LENGTH : lenData;
		char *data = bufData;
		ThrowStatusWrapper status( GDS->_status );

		try
		{
			while ( lenData )
			{
				auto res = directBlobHandle->getSegment( &status, post, data, &length );
				const bool keep_reading = ( res == IStatus::RESULT_OK || res == IStatus::RESULT_SEGMENT );
				if( !keep_reading ) break;

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
		catch( const FbException& error )
		{
			THROW_ISC_EXCEPTION ( connection, error.getStatus() );
		}
	}
	return bEndData;
}

void IscBlob::directCloseBlob()
{
	if ( directBlobHandle )
	{
		ThrowStatusWrapper status( statement->connection->GDS->_status );
		try
		{
			directBlobHandle->close( &status );
			directBlobHandle = nullptr;
		}
		catch( ... )
		{
			if( directBlobHandle ) {
				directBlobHandle->release();
				directBlobHandle = nullptr;
			}
		}
	}
	fetched = true;
	directBlob = false;
}

//
// Block direct operations at record SQLPutData 
//
void IscBlob::directCreateBlob( char * sqldata )
{
	IscConnection * connection = statement->connection;
	CFbDll * GDS = connection->GDS;
	ThrowStatusWrapper status( GDS->_status );

	if ( directBlobHandle )
		try
		{
			directBlobHandle->close( &status );
			directBlobHandle = nullptr;
		}
		catch( ... )
		{
			if( directBlobHandle ) {
				directBlobHandle->release();
				directBlobHandle = nullptr;
			}
		}

	ITransaction* transactionHandle = statement->startTransaction();

	try
	{
		directBlobHandle = connection->databaseHandle->createBlob( &status, transactionHandle, (ISC_QUAD*) sqldata, 0, NULL);
	}
	catch( const FbException& error )
	{
		THROW_ISC_EXCEPTION ( connection, error.getStatus() );
	}
}

void IscBlob::directWriteBlob( char *data, int length )
{
	IscConnection * connection = statement->connection;
	CFbDll * GDS = connection->GDS;
	ThrowStatusWrapper status( GDS->_status );

	int post = DEFAULT_BLOB_BUFFER_LENGTH;

	try
	{
		while ( length > post )
		{
			directBlobHandle->putSegment( &status, post, data );
			data += post;
			length -= post;
		}

		if ( length > 0 )
		{
			directBlobHandle->putSegment( &status, (unsigned short)length, data );
		}
	}
	catch( const FbException& error )
	{
		THROW_ISC_EXCEPTION ( connection, error.getStatus() );
	}
}

}; // end namespace IscDbcLibrary
