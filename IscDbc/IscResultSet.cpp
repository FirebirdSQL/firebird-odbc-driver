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
#include "IscResultSetMetaData.h"
#include "IscArray.h"
#include "IscBlob.h"
#include "IscConnection.h"
#include "SQLError.h"
#include "Value.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


IscResultSet::IscResultSet(IscStatement *iscStatement)
{
	useCount	= 1;
	statement	= iscStatement;
	metaData	= NULL;
	conversions = NULL;
	sqlda		= NULL;

	if (iscStatement)
		numberColumns = statement->numberColumns;

	if (statement)
		{
		statement->addRef();
		sqlda = &statement->outputSqlda;
		numberColumns = sqlda->getColumnCount();
		sqlda->allocBuffer();
		values.alloc (numberColumns);
		allocConversions();
		}
	activePosRowInSet = 0;
	statysPositionRow = enBEFORE_FIRST;
}

IscResultSet::~IscResultSet()
{
	close();

	if (metaData)
		delete metaData;
}

ResultSetMetaData* IscResultSet::getMetaData()
{
	if (metaData)
		return (ResultSetMetaData*) metaData;

	metaData = new IscResultSetMetaData (this, numberColumns);

	return (ResultSetMetaData*) metaData;
}

// Is used only for cursors OdbcJdbc
// It is forbidden to use in IscDbc
bool IscResultSet::readForwardCursor()
{
	if (!statement)
		throw SQLEXCEPTION (RUNTIME_ERROR, "resultset is not active");

	ISC_STATUS statusVector [20];

	int dialect = statement->connection->getDatabaseDialect ();
	int ret = GDS->_dsql_fetch (statusVector, &statement->statementHandle, dialect, *sqlda);

	if (ret)
	{
		if (ret == 100)
		{
			close();
			return false;
		}
		THROW_ISC_EXCEPTION (statusVector);
	}

	return true;
}

bool IscResultSet::next()
{
	if (!statement)
		throw SQLEXCEPTION (RUNTIME_ERROR, "resultset is not active");


	deleteBlobs();
	reset();
	allocConversions();
	ISC_STATUS statusVector [20];

	int dialect = statement->connection->getDatabaseDialect ();
	int ret = GDS->_dsql_fetch (statusVector, &statement->statementHandle, dialect, *sqlda);

	if (ret)
		{
		if (ret == 100)
			{
			close();
			return false;
			}
		THROW_ISC_EXCEPTION (statusVector);
		}

	XSQLVAR *var = sqlda->sqlda->sqlvar;
    Value *value = values.values;

	for (int n = 0; n < numberColumns; ++n, ++var, ++value)
		statement->setValue (value, var);

	return true;
}

bool IscResultSet::setCurrentRowInBufferStaticCursor(int nRow)
{
	return sqlda->setCurrentRowInBufferStaticCursor(nRow);
}

bool IscResultSet::readStaticCursor()
{
	if (!statement)
		throw SQLEXCEPTION (RUNTIME_ERROR, "resultset is not active");

	ISC_STATUS statusVector [20];

	int dialect = statement->connection->getDatabaseDialect ();
	int ret;

	sqlda->initStaticCursor(statement->connection);
	sqlda->setCurrentRowInBufferStaticCursor(0);
	while(!(ret = GDS->_dsql_fetch (statusVector, &statement->statementHandle, dialect, *sqlda)))
		sqlda->copyNextSqldaInBufferStaticCursor();

	if ( ret != 100 )
		THROW_ISC_EXCEPTION (statusVector);

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

bool IscResultSet::getDataFromStaticCursor (int column, int cType, void * pointer, int bufferLength, long * indicatorPointer)
{
	if ( !(activePosRowInSet >= 0 && activePosRowInSet < sqlda->getCountRowsStaticCursor()) )
		return false;

	char * sqldata;
	short * sqlind;
	XSQLVAR *var = sqlda->sqlda->sqlvar + column - 1;
    Value *value = values.values + column - 1;

	sqlda->setCurrentRowInBufferStaticCursor(activePosRowInSet);
	sqlda->getAdressFieldFromCurrentRowInBufferStaticCursor(column,sqldata,sqlind);

	if ( *sqlind == -1 )
		value->type = Null;
	else if ( (var->sqltype & ~1) == SQL_ARRAY )
	{
		SIscArrayData * ptArr = (SIscArrayData *)*(long*)sqldata;
		IscArray iscArr(ptArr);
		iscArr.fetchArrayToString();
		value->setString(iscArr.getString(),false);
	}
	else if ( (var->sqltype & ~1) == SQL_BLOB )
	{
		IscBlob * ptBlob = (IscBlob *)*(long*)sqldata;
		value->setString(ptBlob->getString(),false);
	}
	else
	{
		XSQLVAR Var = *var;
		Var.sqlind = sqlind;
		Var.sqldata = sqldata;
		statement->setValue (value, &Var);
	}

	return true;
}

const char* IscResultSet::getString(int id)
{
	if (id < 1 || id > numberColumns)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column index for result set");

	/*if (conversions [id - 1])
		return conversions [id - 1];*/
	return getValue (id)->getString(conversions + id - 1);
}


const char* IscResultSet::getString(const char * columnName)
{
	return getString (findColumn (columnName));
}

long IscResultSet::getInt(int id)
{
	return getValue (id)->getLong();
}

long IscResultSet::getInt(const char * columnName)
{
	return getValue (columnName)->getLong();
}

Value* IscResultSet::getValue(int index)
{
	if (index < 1 || index > numberColumns)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column index for result set");

	Value *value = values.values + index - 1;
	valueWasNull = value->type == Null;

	return value;
}

bool IscResultSet::isNull(int index)
{
	if (index < 1 || index > numberColumns)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column index for result set");

	Value *value = values.values + index - 1;
	return value->type == Null;
}

Value* IscResultSet::getValue(const char * columnName)
{
	return getValue (findColumn (columnName));
}

void IscResultSet::close()
{
	reset();

	if (statement)
		{
		statement->deleteResultSet (this);
		statement->release();
		statement = NULL;
		}
}

Blob* IscResultSet::getBlob(int index)
{
	Blob *blob = getValue (index)->getBlob();
	blobs.append (blob);

	return blob;
}

Blob* IscResultSet::getBlob(const char * columnName)
{
	Blob *blob = getValue (columnName)->getBlob();
	blobs.append (blob);

	return blob;
}

void IscResultSet::deleteBlobs()
{
	FOR_OBJECTS (Blob*, blob, &blobs)
		blob->release();
	END_FOR;

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

int IscResultSet::findColumn(const char * columnName)
{
	int n = sqlda->findColumn (columnName);

	if (n >= 0)
		return n + 1;

	/***
	for (int n = 0; n < numberColumns; ++n)
		if (!strcasecmp (columnNames [n], columnName))
			return n + 1;
	***/

	throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column name %s for result set",
							columnName);

	return -1;
}



bool IscResultSet::wasNull()
{
	return valueWasNull;
}

void IscResultSet::allocConversions()
{
	conversions = new char* [numberColumns];
	memset (conversions, 0, sizeof (char*) * numberColumns);
}

void IscResultSet::setNull(int index)
{
	if (index < 1 || index > numberColumns)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column index for result set");
	values.values [index - 1].setNull();
}

void IscResultSet::setValue(int index, const char * value)
{
	if (index < 1 || index > numberColumns)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column index for result set");
	values.values [index - 1].setString (value, true);
}

void IscResultSet::setValue(int index, long value)
{
	if (index < 1 || index > numberColumns)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column index for result set");
	values.values [index - 1].setValue (value, true);
}

short IscResultSet::getShort(int id)
{
	return getValue (id)->getShort();
}

short IscResultSet::getShort(const char * columnName)
{
	return getValue (columnName)->getShort();
}

QUAD IscResultSet::getQuad(int id)
{
	return getValue (id)->getQuad();
}

QUAD IscResultSet::getQuad(const char * columnName)
{
	return getValue (columnName)->getQuad();
}

double IscResultSet::getDouble(int id)
{
	return getValue (id)->getDouble();
}

double IscResultSet::getDouble(const char * columnName)
{
	return getValue (columnName)->getDouble();
}

char IscResultSet::getByte(int id)
{
	return getValue (id)->getByte();
}

char IscResultSet::getByte(const char * columnName)
{
	return getValue (columnName)->getByte();
}

float IscResultSet::getFloat(int id)
{
	return getValue (id)->getFloat();
}

float IscResultSet::getFloat(const char * columnName)
{
	return getValue (columnName)->getFloat();
}

int IscResultSet::getColumnType(int index, int &realSqlType)
{
	if (index < 1 || index > numberColumns)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column index for result set");
	return sqlda->getColumnType (index, realSqlType);
}

int IscResultSet::getColumnDisplaySize(int index)
{
	if (index < 1 || index > numberColumns)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column index for result set");
	return sqlda->getDisplaySize (index);
}

const char* IscResultSet::getColumnName(int index)
{
	if (index < 1 || index > numberColumns)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column index for result set");
	return sqlda->getColumnName (index);
}

const char* IscResultSet::getTableName(int index)
{
	return sqlda->getTableName (index);
}

const char* IscResultSet::getColumnTypeName(int index)
{
	return sqlda->getColumnTypeName (index);
}

int IscResultSet::getPrecision(int index)
{
	if (index < 1 || index > numberColumns)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column index for result set");
	return sqlda->getPrecision (index);
}

int IscResultSet::getScale(int index)
{
	if (index < 1 || index > numberColumns)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column index for result set");
	return -sqlda->getScale (index);
}

bool IscResultSet::isNullable(int index)
{
	if (index < 1 || index > numberColumns)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column index for result set");
	return sqlda->isNullable (index);
}

DateTime IscResultSet::getDate(int id)
{
	return getValue (id)->getDate();
}

DateTime IscResultSet::getDate(const char * columnName)
{
	return getValue (columnName)->getDate();
}

SqlTime IscResultSet::getTime(int id)
{
	return getValue (id)->getTime();
}

SqlTime IscResultSet::getTime(const char * columnName)
{
	return getValue (columnName)->getTime();
}

TimeStamp IscResultSet::getTimestamp(int id)
{
	return getValue (id)->getTimestamp();
}

TimeStamp IscResultSet::getTimestamp(const char * columnName)
{
	return getValue (columnName)->getTimestamp();
}

int IscResultSet::objectVersion()
{
	return RESULTSET_VERSION;
}

const char* IscResultSet::getSchemaName(int index)
{
	if (index < 1 || index > numberColumns)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column index for result set");
	return sqlda->getOwnerName (index);
}

void IscResultSet::setPosRowInSet(int posRow)
{
	activePosRowInSet = posRow;
}	

int IscResultSet::getPosRowInSet()
{
	return activePosRowInSet;
}	

bool IscResultSet::isBeforeFirst()
{
	return statysPositionRow == enBEFORE_FIRST;
}

bool IscResultSet::isAfterLast()
{
	return statysPositionRow == enAFTER_LAST;
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
	NOT_YET_IMPLEMENTED;
}

void IscResultSet::updateInt (int columnIndex, int value)
{
	NOT_YET_IMPLEMENTED;
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

void IscResultSet::updateString (int columnIndex, const char* value)
{
	NOT_YET_IMPLEMENTED;
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

