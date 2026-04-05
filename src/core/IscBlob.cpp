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
// Phase 14.5: Migrated from raw Firebird::IBlob* to fbcpp::Blob (RAII).
//
//////////////////////////////////////////////////////////////////////

#include "stdio.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <span>

#include "IscDbc.h"
#include "Connection.h"
#include "IscBlob.h"
#include "IscConnection.h"
#include "IscStatement.h"
#include "SQLError.h"
#include <fb-cpp/Blob.h>

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

	// Ensure transaction is active (creates one if needed)
	statement->startTransaction();

	fbcpp::BlobId fbBlobId;
	fbBlobId.id = blobId;

	try
	{
		fbcpp::Blob blob(*connection->attachment_, *connection->transaction_, fbBlobId);

		char buffer [DEFAULT_BLOB_BUFFER_LENGTH];

		for (;;)
		{
			unsigned bytesRead = blob.readSegment(std::span<char>(buffer, sizeof(buffer)));
			if (bytesRead == 0) break;
			putSegment (bytesRead, buffer, true);
		}

		blob.close();
		fetched = true;
	}
	catch( const fbcpp::DatabaseException& error )
	{
		throw SQLError::fromDatabaseException(error);
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
	statement->startTransaction();

	try
	{
		fbcpp::Blob blob(*connection->attachment_, *connection->transaction_);

		for ( int len, offset = 0; len = getSegmentLength (offset); offset += len )
		{
			blob.writeSegment(std::span<const char>((const char*)getSegment(offset), len));
		}

		*(ISC_QUAD*)sqldata = blob.getId().id;
		blob.close();
	}
	catch( const fbcpp::DatabaseException& error )
	{
		throw SQLError::fromDatabaseException(error);
	}
}

void IscBlob::writeStreamHexToBlob(char * sqldata)
{
	IscConnection * connection = statement->connection;
	statement->startTransaction();

	try
	{
		fbcpp::Blob blob(*connection->attachment_, *connection->transaction_);

		for ( int len, offset = 0; len = getSegmentLength (offset); offset += len )
		{
			blob.writeSegment(std::span<const char>(convStrHexToBinary((char*)getSegment(offset), len), len / 2));
		}

		*(ISC_QUAD*)sqldata = blob.getId().id;
		blob.close();
	}
	catch( const fbcpp::DatabaseException& error )
	{
		throw SQLError::fromDatabaseException(error);
	}
}

void IscBlob::writeBlob(char * sqldata, char *data, int length)
{
	IscConnection * connection = statement->connection;
	statement->startTransaction();

	try
	{
		fbcpp::Blob blob(*connection->attachment_, *connection->transaction_);

		for ( int len, offset = 0; len = getSegmentLength (offset); offset += len )
		{
			blob.writeSegment(std::span<const char>(convStrHexToBinary((char*)getSegment(offset), len), len / 2));
		}

		if ( length )
		{
			int post = DEFAULT_BLOB_BUFFER_LENGTH;

			while ( length > post )
			{
				blob.writeSegment(std::span<const char>(data, post));
				data += post;
				length -= post;
			}

			if ( length > 0 )
			{
				blob.writeSegment(std::span<const char>(data, length));
			}
		}

		*(ISC_QUAD*)sqldata = blob.getId().id;
		blob.close();
	}
	catch( const fbcpp::DatabaseException& error )
	{
		throw SQLError::fromDatabaseException(error);
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

void IscBlob::directOpenBlob( char * sqldata )
{
	IscConnection * connection = statement->connection;
	fetched = false;
	statement->startTransaction();

	// Close any previously open direct blob
	directBlobHandle_.reset();

	fbcpp::BlobId fbBlobId;
	fbBlobId.id = *(ISC_QUAD*)sqldata;

	try
	{
		directBlobHandle_ = std::make_unique<fbcpp::Blob>(
			*connection->attachment_, *connection->transaction_, fbBlobId);

		directLength = directBlobHandle_->getLength();
		directBlob = true;
		offset = 0;
	}
	catch( const fbcpp::DatabaseException& error )
	{
		throw SQLError::fromDatabaseException(error);
	}
}

bool IscBlob::directFetchBlob( char * bufData, int lenData, int &lenRead )
{
	bool bEndData = false;

	if ( lenData )
	{
		char *data = bufData;

		try
		{
			while ( lenData > 0 )
			{
				int chunkSize = lenData > DEFAULT_BLOB_BUFFER_LENGTH ? DEFAULT_BLOB_BUFFER_LENGTH : lenData;
				unsigned bytesRead = directBlobHandle_->read(std::span<char>(data, chunkSize));
				if (bytesRead == 0) break;

				data += bytesRead;
				lenData -= bytesRead;
			}

			lenRead = static_cast<int>(data - bufData);
			offset += lenRead;
		}
		catch( const fbcpp::DatabaseException& error )
		{
			throw SQLError::fromDatabaseException(error);
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
		int post = lenData > DEFAULT_BLOB_BUFFER_LENGTH ? DEFAULT_BLOB_BUFFER_LENGTH : lenData;
		char *data = bufData;

		try
		{
			while ( lenData )
			{
				length = directBlobHandle_->readSegment(std::span<char>(data, post));
				if (length == 0) break;

				short *address = (short*)data + length - 1;
				unsigned char *end = (unsigned char *)data + length - 1;

				data += length*2;
				lenData -= length;
				if ( lenData < post )
					post = lenData;

				while( length-- )
					*address-- = conwBinToHexStr[*end--];
			}

			lenRead = static_cast<int>(data - bufData);
			offset += lenRead;
		}
		catch( const fbcpp::DatabaseException& error )
		{
			throw SQLError::fromDatabaseException(error);
		}
	}
	return bEndData;
}

void IscBlob::directCloseBlob()
{
	directBlobHandle_.reset();
	fetched = true;
	directBlob = false;
}

//
// Block direct operations at record SQLPutData 
//
void IscBlob::directCreateBlob( char * sqldata )
{
	IscConnection * connection = statement->connection;
	statement->startTransaction();

	// Close any previously open direct blob
	directBlobHandle_.reset();

	try
	{
		directBlobHandle_ = std::make_unique<fbcpp::Blob>(
			*connection->attachment_, *connection->transaction_);

		*(ISC_QUAD*)sqldata = directBlobHandle_->getId().id;
	}
	catch( const fbcpp::DatabaseException& error )
	{
		throw SQLError::fromDatabaseException(error);
	}
}

void IscBlob::directWriteBlob( char *data, int length )
{
	int post = DEFAULT_BLOB_BUFFER_LENGTH;

	try
	{
		while ( length > post )
		{
			directBlobHandle_->writeSegment(std::span<const char>(data, post));
			data += post;
			length -= post;
		}

		if ( length > 0 )
		{
			directBlobHandle_->writeSegment(std::span<const char>(data, length));
		}
	}
	catch( const fbcpp::DatabaseException& error )
	{
		throw SQLError::fromDatabaseException(error);
	}
}

}; // end namespace IscDbcLibrary
