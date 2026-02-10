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
 *  The Original Code was created by James A. Starkey
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 *
 *
 *	2003-03-24	IscResultSet.cpp
 *				Contributed by Andrew Gough
 *				In IscResultSet::reset() delete the 'conversions' 
 *				array itself as well as the array elements.
 *
 */
// IscResultSet.cpp: implementation of the IscResultSet class.
//
//////////////////////////////////////////////////////////////////////

#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "IscDbc.h"
#include "IscResultSet.h"
#include "IscStatement.h"
#include "IscArray.h"
#include "IscBlob.h"
#include "IscConnection.h"
#include "SQLError.h"
#include "Value.h"

using namespace Firebird;

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscResultSet::IscResultSet( IscStatement *iscStatement ) : IscStatementMetaData( NULL , NULL )
{
	initResultSet(iscStatement);
}

IscResultSet::~IscResultSet()
{
	try
	{
		close();
	}
	catch ( std::exception &ex )
	{
		fprintf (stdout, "Failed while deleting IscResultSet (exception: \"%s\")\n", ex.what()); 
	}
}

StatementMetaData* IscResultSet::getMetaData()
{
	return (StatementMetaData*) this;
}

void IscResultSet::initResultSet(IscStatement *iscStatement)
{
	useCount	= 1;
	statement	= iscStatement;
	conversions = NULL;
	sqlda		= NULL;
	numberColumns = 0;
	sqldataOffsetPtr = 0;

	if (statement)
	{
		statement->addRef();
		sqlda = &statement->outputSqlda;
		numberColumns = sqlda->getColumnCount();
		values.alloc (numberColumns);
		allocConversions();

		// 10.2.2: Pre-allocate IscBlob pool for blob columns.
		blobColumnPool.resize(numberColumns);
		for (int n = 0; n < numberColumns; ++n)
		{
			auto* var = sqlda->Var(n + 1);
			if (var->sqltype == SQL_BLOB)
				blobColumnPool[n] = std::make_unique<IscBlob>();
		}

		// 10.5.1: Initialize N-row prefetch buffer.
		prefetchRowSize = static_cast<int>(sqlda->buffer.size());
		if (prefetchRowSize > 0)
			prefetchBuffer.resize(static_cast<size_t>(kPrefetchRows) * prefetchRowSize);
		prefetchCount = 0;
		prefetchIndex = 0;
		prefetchCursorDone = false;
	}

	nextSimulateForProcedure = false;
	activePosRowInSet = 0;
	statysPositionRow = enBEFORE_FIRST;
}

// Is used only for cursors OdbcFb
// It is forbidden to use in IscDbc
bool IscResultSet::nextFetch()
{
	if (!statement || !statement->fbResultSet)
		throw SQLEXCEPTION (RUNTIME_ERROR, "resultset is not active");

	// 10.5.1: N-row prefetch — serve from the local buffer when possible,
	// otherwise fill a new batch from the Firebird cursor.
	if (prefetchRowSize > 0 && !prefetchBuffer.empty())
	{
		// Return next buffered row if available.
		if (prefetchIndex < prefetchCount)
		{
			memcpy(sqlda->buffer.data(),
				   prefetchBuffer.data() + static_cast<size_t>(prefetchIndex) * prefetchRowSize,
				   prefetchRowSize);
			++prefetchIndex;
			return true;
		}

		// Buffer exhausted. If the cursor already returned RESULT_NO_DATA
		// during the previous batch fill, we're done — no more rows.
		if (prefetchCursorDone)
		{
			close();
			return false;
		}

		// Fill a new batch from the Firebird cursor.
		prefetchCount = 0;
		prefetchIndex = 0;

		ThrowStatusWrapper status( statement->connection->GDS->_status );
		try
		{
			for (int i = 0; i < kPrefetchRows; ++i)
			{
				char* rowPtr = prefetchBuffer.data() + static_cast<size_t>(i) * prefetchRowSize;
				auto fetch_stat = statement->fbResultSet->fetchNext( &status, rowPtr );
				if ( fetch_stat == IStatus::RESULT_NO_DATA )
				{
					prefetchCursorDone = true;
					break;
				}
				++prefetchCount;
			}
		}
		catch( const FbException& error )
		{
			THROW_ISC_EXCEPTION( statement->connection, error.getStatus() );
		}

		if (prefetchCount == 0)
		{
			close();
			return false;
		}

		// Copy first row of the new batch into the sqlda buffer.
		memcpy(sqlda->buffer.data(), prefetchBuffer.data(), prefetchRowSize);
		prefetchIndex = 1;
		return true;
	}

	// Fallback: single-row fetch when prefetch buffer was not allocated.
	ThrowStatusWrapper status( statement->connection->GDS->_status );
	try
	{
		auto fetch_stat = statement->fbResultSet->fetchNext( &status, sqlda->buffer.data() );

		if( fetch_stat == IStatus::RESULT_NO_DATA ) {
			close();
			return false;
		}
	}
	catch( const FbException& error )
	{
		THROW_ISC_EXCEPTION( statement->connection, error.getStatus() );
	}

	return true;
}

bool IscResultSet::next()
{
	if (!statement || !statement->fbResultSet)
		throw SQLEXCEPTION (RUNTIME_ERROR, "resultset is not active");

	deleteBlobs();
	resetConversionContents();

	ThrowStatusWrapper status( statement->connection->GDS->_status );
	try
	{
		auto fetch_stat = statement->fbResultSet->fetchNext( &status, sqlda->buffer.data() );

		if( fetch_stat == IStatus::RESULT_NO_DATA ) {
			close();
			return false;
		}
	}
	catch( const FbException& error )
	{
		THROW_ISC_EXCEPTION( statement->connection, error.getStatus() );
	}

    Value *value = values.values;

	for (int n = 0; n < numberColumns; ++n, ++value)
	{
		// 10.2.2: For blob columns, reuse the pooled IscBlob instead of
		// allocating a new one in IscStatement::setValue.
		if (n < (int)blobColumnPool.size() && blobColumnPool[n])
		{
			auto* var = sqlda->Var(n + 1);
			if (sqlda->isNull(n + 1))
			{
				value->setNull();
			}
			else
			{
				IscBlob* pooledBlob = blobColumnPool[n].get();
				pooledBlob->bind(statement, var->sqldata);
				pooledBlob->setType(var->sqlsubtype);
				value->setValue(static_cast<Blob*>(pooledBlob));
			}
		}
		else
		{
			statement->setValue(value, n + 1, *sqlda);
		}
	}

	return true;
}

bool IscResultSet::setCurrentRowInBufferStaticCursor(int nRow)
{
	return sqlda->setCurrentRowInBufferStaticCursor(nRow);
}

bool IscResultSet::readFromSystemCatalog()
{
	if (!statement)
		throw SQLEXCEPTION(RUNTIME_ERROR, "resultset is not active");

	sqlda->initStaticCursor(statement);

	while (true)
	{
		sqlda->restoreOrgAdressFieldsStaticCursor(); //need to restore pointers to sqlda buffer
		if (nextFetch() == false) break;
		sqlda->addRowSqldaInBufferStaticCursor();
	}

	sqlda->restoreOrgAdressFieldsStaticCursor();
	sqlda->setCurrentRowInBufferStaticCursor(0);
	sqlda->copyNextSqldaFromBufferStaticCursor();

	return true;
}

bool IscResultSet::readStaticCursor()
{
	if (!statement || !statement->fbResultSet)
		throw SQLEXCEPTION (RUNTIME_ERROR, "resultset is not active");

	CFbDll * GDS = statement->connection->GDS;
	sqlda->initStaticCursor(statement);

	ThrowStatusWrapper status( GDS->_status );
	try
	{
		while( true )
		{
			sqlda->restoreOrgAdressFieldsStaticCursor(); //need to restore pointers to sqlda buffer
			auto fetch_stat = statement->fbResultSet->fetchNext( &status, sqlda->buffer.data() );
			if( fetch_stat == IStatus::RESULT_NO_DATA ) break;
			sqlda->addRowSqldaInBufferStaticCursor();
		}
	}
	catch( const FbException& error )
	{
		sqlda->restoreOrgAdressFieldsStaticCursor();
		THROW_ISC_EXCEPTION( statement->connection, error.getStatus() );
	}

	sqlda->restoreOrgAdressFieldsStaticCursor();
	sqlda->setCurrentRowInBufferStaticCursor(0);
	sqlda->copyNextSqldaFromBufferStaticCursor();

	return true;
}

void IscResultSet::copyNextSqldaInBufferStaticCursor()
{
	sqlda->copyNextSqldaInBufferStaticCursor();
}

void IscResultSet::copyNextSqldaFromBufferStaticCursor()
{
	sqlda->copyNextSqldaFromBufferStaticCursor();
}

int IscResultSet::getCountRowsStaticCursor()
{
	return sqlda->getCountRowsStaticCursor();
}

bool IscResultSet::getDataFromStaticCursor (int column/*, Blob * pointerBlobData*/)
{
	if ( !(activePosRowInSet >= 0 && activePosRowInSet < sqlda->getCountRowsStaticCursor()) )
		return false;

	sqlda->setCurrentRowInBufferStaticCursor(activePosRowInSet);
	sqlda->copyNextSqldaFromBufferStaticCursor();
	return true;
}

bool IscResultSet::nextFromProcedure()
{
	if ( nextSimulateForProcedure )
		return false;

	nextSimulateForProcedure = true;
	return true;
}

Value* IscResultSet::getValue(int index)
{
	if (index < 1 || index > values.count)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column index for result set");

	Value *value = values.values + index - 1;
	valueWasNull = value->type == Null;

	return value;
}

Value* IscResultSet::getValue(const char * columnName)
{
	return getValue (findColumn (columnName));
}

void IscResultSet::close()
{
	deleteBlobs();
	reset();

	// 10.2.2: Release pooled blob objects before releasing statement
	blobColumnPool.clear();

	if (statement)
	{
		statement->deleteResultSet (this);
		statement->release();
		statement = NULL;
	}
}

void IscResultSet::deleteBlobs()
{
	for (auto* blob : blobs)
		blob->release();

	blobs.clear();
}

const char* IscResultSet::genHTML(const char *series, const char *type, Properties *context)
{
	throw SQLEXCEPTION (FEATURE_NOT_YET_IMPLEMENTED, "function is not implemented");

	return NULL;
}

void IscResultSet::freeHTML(const char *html)
{
	delete [] (char*) html;
}

void IscResultSet::addRef()
{
	++useCount;
}

int IscResultSet::release()
{
	if (--useCount == 0)
	{
		delete this;
		return 0;
	}

	return useCount;
}

void IscResultSet::reset()
{
	if (conversions)
	{
		for (int n = 0; n < numberColumns; ++n)
			if (conversions [n])
			{
				delete [] conversions [n];
				conversions [n] = NULL;
			}
		delete[] conversions;
		conversions = NULL;
	}
}

void IscResultSet::resetConversionContents()
{
	if (conversions)
	{
		for (int n = 0; n < numberColumns; ++n)
			if (conversions [n])
			{
				delete [] conversions [n];
				conversions [n] = NULL;
			}
	}
	else
	{
		allocConversions();
	}
}

const char* IscResultSet::getCursorName()
{
	NOT_YET_IMPLEMENTED;
	return "";
}

int IscResultSet::findColumn(const char * columnName)
{
	int n = sqlda->findColumn (columnName);

	if (n >= 0)
		return n + 1;

	throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column name %s for result set",
							columnName);
	return -1;
}

bool IscResultSet::wasNull()
{
	return valueWasNull;
}

int IscResultSet::getColumnCount()
{
	return numberColumns;
}

// Used class Value
void IscResultSet::allocConversions()
{
	conversions = new char* [numberColumns];
	memset (conversions, 0, sizeof (char*) * numberColumns);
}

void IscResultSet::setValue(int index, const char * value)
{
	if (index < 1 || index > values.count)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column index for result set");
	values.values [index - 1].setString (value, true);
}

void IscResultSet::setValue(int index, int value)
{
	if (index < 1 || index > values.count)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column index for result set");
	values.values [index - 1].setValue (value, true);
}

void IscResultSet::setNull(int index)
{
	if (index < 1 || index > values.count)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column index for result set");
	values.values [index - 1].setNull();
}

const char* IscResultSet::getString(int id)
{
	return getValue (id)->getString(conversions + id - 1);
}


const char* IscResultSet::getString(const char * columnName)
{
	return getString (findColumn (columnName));
}

TimeStamp IscResultSet::getTimestamp(int id)
{
	return getValue (id)->getTimestamp();
}

TimeStamp IscResultSet::getTimestamp(const char * columnName)
{
	return getValue (columnName)->getTimestamp();
}

SqlTime IscResultSet::getTime(int id)
{
	return getValue (id)->getTime();
}

SqlTime IscResultSet::getTime(const char * columnName)
{
	return getValue (columnName)->getTime();
}

DateTime IscResultSet::getDate(int id)
{
	return getValue (id)->getDate();
}

DateTime IscResultSet::getDate(const char * columnName)
{
	return getValue (columnName)->getDate();
}

int IscResultSet::getInt(int id)
{
	return getValue (id)->getLong();
}

int IscResultSet::getInt(const char * columnName)
{
	return getValue (columnName)->getLong();
}

float IscResultSet::getFloat(int id)
{
	return getValue (id)->getFloat();
}

float IscResultSet::getFloat(const char * columnName)
{
	return getValue (columnName)->getFloat();
}

char IscResultSet::getByte(int id)
{
	return getValue (id)->getByte();
}

char IscResultSet::getByte(const char * columnName)
{
	return getValue (columnName)->getByte();
}

Blob* IscResultSet::getBlob(int index)
{
	Blob *blob = getValue (index)->getBlob();
	blobs.push_back (blob);

	return blob;
}

Blob* IscResultSet::getBlob(const char * columnName)
{
	Blob *blob = getValue (columnName)->getBlob();
	blobs.push_back (blob);

	return blob;
}

double IscResultSet::getDouble(int id)
{
	return getValue (id)->getDouble();
}

double IscResultSet::getDouble(const char * columnName)
{
	return getValue (columnName)->getDouble();
}

QUAD IscResultSet::getLong(int id)
{
	return getValue (id)->getQuad();
}

QUAD IscResultSet::getLong(const char * columnName)
{
	return getValue (columnName)->getQuad();
}

short IscResultSet::getShort(int id)
{
	return getValue (id)->getShort();
}

short IscResultSet::getShort(const char * columnName)
{
	return getValue (columnName)->getShort();
}

bool IscResultSet::getBoolean(int id)
{
	return getValue (id)->getBoolean();
}

bool IscResultSet::getBoolean(const char * columnName)
{
	return getValue (columnName)->getBoolean();
}

int IscResultSet::objectVersion()
{
	return RESULTSET_VERSION;
}

void IscResultSet::setPosRowInSet(int posRow)
{
	activePosRowInSet = posRow;
}	

int IscResultSet::getPosRowInSet()
{
	return activePosRowInSet;
}	

size_t* IscResultSet::getSqlDataOffsetPtr()
{
	return &sqldataOffsetPtr;
}	

bool IscResultSet::isBeforeFirst()
{
	return statysPositionRow == enBEFORE_FIRST;
}

bool IscResultSet::isAfterLast()
{
	return statysPositionRow == enAFTER_LAST;
}

bool IscResultSet::isCurrRowsetStart()
{
	return statysPositionRow == enSUCCESS;
}

bool IscResultSet::isFirst()
{
	NOT_YET_IMPLEMENTED;
	return 0;
}

bool IscResultSet::isLast()
{
	NOT_YET_IMPLEMENTED;
	return 0;
}

void IscResultSet::beforeFirst()
{
	statysPositionRow = enBEFORE_FIRST;
}

void IscResultSet::afterLast()
{
	statysPositionRow = enAFTER_LAST;
}

void IscResultSet::currRowsetStart()
{
	statysPositionRow = enSUCCESS;
}

bool IscResultSet::first()
{
	NOT_YET_IMPLEMENTED;
	return 0;
}

bool IscResultSet::last()
{
	NOT_YET_IMPLEMENTED;
	return 0;
}

int IscResultSet::getRow()
{
	NOT_YET_IMPLEMENTED;
	return 0;
}

bool IscResultSet::absolute (int row)
{
	NOT_YET_IMPLEMENTED;
	return 0;
}

bool IscResultSet::relative (int rows)
{
	NOT_YET_IMPLEMENTED;
	return 0;
}

bool IscResultSet::previous()
{
	NOT_YET_IMPLEMENTED;
	return 0;
}

void IscResultSet::setFetchDirection (int direction)
{
	NOT_YET_IMPLEMENTED;
}

int IscResultSet::getFetchDirection ()
{
	NOT_YET_IMPLEMENTED;
	return 0;
}

int IscResultSet::getFetchSize()
{
	NOT_YET_IMPLEMENTED;
	return 0;
}

int IscResultSet::getType()
{
	NOT_YET_IMPLEMENTED;
	return 0;
}

bool IscResultSet::rowUpdated()
{
	NOT_YET_IMPLEMENTED;
	return 0;
}

bool IscResultSet::rowInserted()
{
	NOT_YET_IMPLEMENTED;
	return 0;
}

bool IscResultSet::rowDeleted()
{
	NOT_YET_IMPLEMENTED;
	return 0;
}

void IscResultSet::updateNull (int columnIndex)
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateBoolean (int columnIndex, bool value)
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateByte (int columnIndex, char value)
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateShort (int columnIndex, short value)
{
	sqlda->updateShort (columnIndex, value);
}

void IscResultSet::updateInt (int columnIndex, int value)
{
	sqlda->updateInt (columnIndex, value);
}

void IscResultSet::updateLong (int columnIndex, QUAD value)
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateFloat (int columnIndex, float value)
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateDouble (int columnIndex, double value)
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateText (int columnIndex, const char* value)
{
	sqlda->updateText (columnIndex, value);
}

void IscResultSet::updateString (int columnIndex, const char* value)
{
	sqlda->updateVarying (columnIndex, value);
}

void IscResultSet::updateBytes (int columnIndex, int length, const void *bytes)
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateDate (int columnIndex, DateTime value)
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateTime (int columnIndex, SqlTime value)
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateTimeStamp (int columnIndex, TimeStamp value)
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateBlob (int columnIndex, Blob* value)
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateNull (const char *columnName)
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateBoolean (const char *columnName, bool value)
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateByte (const char *columnName, char value)
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateShort (const char *columnName, short value)
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateInt (const char *columnName, int value)
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateLong (const char *columnName, QUAD value)
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateFloat (const char *columnName, float value)
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateDouble (const char *columnName, double value)
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateString (const char *columnName, const char* value)
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateBytes (const char *columnName, int length, const void *bytes)
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateDate (const char *columnName, DateTime value)
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateTime (const char *columnName, SqlTime value)
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateTimeStamp (const char *columnName, TimeStamp value)
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateBlob (const char *columnName, Blob* value)
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::insertRow()
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateRow()
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::deleteRow()
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::refreshRow()
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::cancelRowUpdates()
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::moveToInsertRow()
{
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::moveToCurrentRow()
{
	NOT_YET_IMPLEMENTED;
}

Statement *IscResultSet::getStatement()
{
	NOT_YET_IMPLEMENTED;
	return 0;
}

}; // end namespace IscDbcLibrary
