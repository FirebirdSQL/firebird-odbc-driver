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
 *
 *	2002-10-11	OdbcDesc.cpp
 *              Contributed by C G Alvarez
 *              Added sqlGetDescField()
 *
 */

// OdbcDesc.cpp: implementation of the OdbcDesc class.
//
//////////////////////////////////////////////////////////////////////

#include <memory.h>
#include "OdbcJdbc.h"
#include "OdbcDesc.h"
#include "OdbcConnection.h"
#include "Connection.h"
#include "DescRecord.h"
#include "SQLException.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

OdbcDesc::OdbcDesc(OdbcDescType type, OdbcConnection *connect)
{
	connection = connect;
	metaData = NULL;
	recordSlots = 0;
	records = NULL;

	headType = type;
	headAllocType = SQL_DESC_ALLOC_AUTO;
	headArraySize = 1;
	headArrayStatusPtr = (SQLUSMALLINT*)NULL;
	headBindOffsetPtr = (SQLINTEGER*)NULL;
	headRowsProcessedPtr = (SQLUINTEGER*)NULL;
	headCount = 0;
	headBindType = SQL_BIND_BY_COLUMN;

	if( headType == odtImplementationRow )
		bDefined = false;
	else
		bDefined = true;
}

void OdbcDesc::setDefaultImplDesc (StatementMetaData * ptMetaData)
{
	metaData = ptMetaData;

	if( headType == odtImplementationParameter )
		return;

	bDefined = false;
	recordSlots = 0;
	records = NULL;

	headAllocType = SQL_DESC_ALLOC_AUTO;
	headArraySize = 1;
	headArrayStatusPtr = (SQLUSMALLINT*)NULL;
	headBindOffsetPtr = (SQLINTEGER*)NULL;
	headRowsProcessedPtr = (SQLUINTEGER*)NULL;
	headCount = 0;

	if(	metaData == NULL )
		return;

	bDefined = true;

	headCount = metaData->getParameterCount();
	getDescRecord (headCount);
}

void OdbcDesc::removeRecords()
{
	if (records)
	{
		for (int n = 0; n < recordSlots; ++n)
			if (records [n])
				delete records [n];
		delete [] records;
		records = NULL;
	}
	headCount = 0;
	recordSlots = 0;
}
 
/*
			case odtApplicationRow:
			case odtApplicationParameter:
			case odtImplementationRow:
			case odtImplementationParameter:
*/

RETCODE OdbcDesc::operator =(OdbcDesc &sour)
{
	if( headType == odtImplementationRow )
		return sqlReturn (SQL_ERROR, "HY016", "Cannot modify an implementation row descriptor");

	if( sour.headType == odtImplementationRow && sour.bDefined == false )
		return sqlReturn (SQL_ERROR, "HY007", "Associated statement is not prepared");

	removeRecords();
	getDescRecord(sour.headCount);

	headArraySize = sour.headArraySize;
	headArrayStatusPtr = sour.headArrayStatusPtr;
	headBindOffsetPtr = sour.headBindOffsetPtr;
	headRowsProcessedPtr = sour.headRowsProcessedPtr;
	headBindType = sour.headBindType;

	for ( int n = 0 ; n <= headCount ; n++ )
		*records [n] = sour.records [n];

	return sqlSuccess();
}

OdbcDesc::~OdbcDesc()
{
	if (connection)
		connection->descriptorDeleted (this);

	removeRecords();
}

OdbcObjectType OdbcDesc::getType()
{
	return odbcTypeDescriptor;
}

RETCODE OdbcDesc::sqlGetDescField(int recNumber, int fieldId, SQLPOINTER ptr, int bufferLength, SQLINTEGER *lengthPtr)
{
    clearErrors();
	long size = 0;
	SQLCHAR *string = NULL;
	DescRecord *record = NULL;

	if ( bDefined == false )
		return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");

	if ( recNumber > headCount )
		return sqlReturn (SQL_NO_DATA_FOUND, "HY021", "Inconsistent descriptor information");

	switch (fieldId)
	{
// Head
		case SQL_DESC_COUNT:
		case SQL_DESC_ALLOC_TYPE:
		case SQL_DESC_ARRAY_SIZE:
		case SQL_DESC_ARRAY_STATUS_PTR:
		case SQL_DESC_BIND_OFFSET_PTR:
		case SQL_DESC_BIND_TYPE:
		case SQL_DESC_ROWS_PROCESSED_PTR:
			break;
		default:
			if ( !recNumber && headType == odtImplementationParameter )
					return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			record = getDescRecord (recNumber);
	}

	try
	{
	switch (fieldId)
		{
// Head
		case SQL_DESC_COUNT:
			if (ptr)
				*(SQLSMALLINT*) ptr = headCount,
				size = sizeof (SQLSMALLINT);
			break;

		case SQL_DESC_ALLOC_TYPE:
			switch(headType)
			{
			case odtApplication:
			case odtApplicationRow:
			case odtApplicationParameter:
#pragma FB_COMPILER_MESSAGE("Modify after realized SQLAllocHandle ( SQL_HANDLE_DESC ) FIXME!")
//				if call SQLAllocHandle ( SQL_HANDLE_DESC )
//				value = SQL_DESC_ALLOC_USER;
				if (ptr)
					*(SQLSMALLINT*) ptr = SQL_DESC_ALLOC_AUTO,
					size = sizeof (SQLSMALLINT);
				break;
			case odtImplementationRow:
			case odtImplementationParameter:
				if (ptr)
					*(SQLSMALLINT*) ptr = SQL_DESC_ALLOC_AUTO,
					size = sizeof (SQLSMALLINT);
				break;
			}
			break;

		case SQL_DESC_ARRAY_SIZE:
			switch(headType)
			{
			case odtApplication:
			case odtApplicationParameter:
			case odtApplicationRow:
				if (ptr)
					*(SQLUINTEGER*) ptr = headArraySize,
					size = sizeof (SQLUINTEGER);
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		case SQL_DESC_ARRAY_STATUS_PTR:
			if (ptr)
				*(SQLUSMALLINT**) ptr = headArrayStatusPtr,
				size = sizeof (SQLUSMALLINT*);
			break;

		case SQL_DESC_BIND_OFFSET_PTR:
			switch(headType)
			{
			case odtApplication:
			case odtApplicationParameter:
			case odtApplicationRow:
				if (ptr)
					*(SQLINTEGER **)ptr = headBindOffsetPtr,
					size = sizeof (SQLINTEGER *);
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		case SQL_DESC_BIND_TYPE:
			switch(headType)
			{
			case odtApplication:
			case odtApplicationParameter:
			case odtApplicationRow:
				if (ptr)
					*(SQLINTEGER*)ptr = headBindType,
					size = sizeof (SQLINTEGER);
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		case SQL_DESC_ROWS_PROCESSED_PTR:
			switch(headType)
			{
			case odtImplementationRow:
			case odtImplementationParameter:
				if (ptr)
					*(SQLUINTEGER**)ptr = headRowsProcessedPtr,
					size = sizeof (SQLUINTEGER*);
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;
// Record
		case SQL_DESC_TYPE:
			if (record && ptr)
				*(SQLSMALLINT*) ptr = record->type,
				size = sizeof (SQLSMALLINT);
			break;

		case SQL_DESC_DATETIME_INTERVAL_CODE:
			if (record && ptr)
				*(SQLSMALLINT*) ptr = record->datetimeIntervalCode,
				size = sizeof (SQLSMALLINT);
			break;

		case SQL_DESC_CONCISE_TYPE:
			if (record && ptr)
				*(SQLSMALLINT*) ptr = record->conciseType,
				size = sizeof (SQLSMALLINT);
			break;

		case SQL_DESC_AUTO_UNIQUE_VALUE:
			if(headType ==  odtImplementationRow)
			{
				if (record && ptr)
					*(SQLINTEGER*) ptr = record->autoUniqueValue,
					size = sizeof (SQLINTEGER);
			}
			else
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			break;

		case SQL_DESC_BASE_COLUMN_NAME:
			if(headType ==  odtImplementationRow)
			{
				if (record)
					string = (SQLCHAR*)record->baseColumnName.getString();
			}
			else
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			break;

		case SQL_DESC_BASE_TABLE_NAME:
			if(headType ==  odtImplementationRow)
			{
				if (record)
					string = (SQLCHAR*)record->baseTableName.getString();
			}
			else
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			break;

		case SQL_DESC_CASE_SENSITIVE:
			switch(headType)
			{
			case odtImplementationRow:
			case odtImplementationParameter:
				if (record && ptr)
					*(SQLINTEGER*) ptr = record->caseSensitive,
					size = sizeof (SQLINTEGER);
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		case SQL_DESC_CATALOG_NAME:
			if(headType ==  odtImplementationRow)
			{
				if (record)
					string = (SQLCHAR*)record->catalogName.getString();
			}
			else
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			break;

		case SQL_DESC_DATETIME_INTERVAL_PRECISION:
			if (record && ptr)
				*(SQLINTEGER*) ptr = record->datetimeIntervalPrecision,
				size = sizeof (SQLINTEGER);
			break;

		case SQL_DESC_DISPLAY_SIZE:
			if(headType ==  odtImplementationRow)
			{
				if (record && ptr)
					*(SQLINTEGER*) ptr = record->displaySize,
					size = sizeof (SQLINTEGER);
			}
			else
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			break;

		case SQL_DESC_FIXED_PREC_SCALE:
			switch(headType)
			{
			case odtImplementationRow:
			case odtImplementationParameter:
				if (record && ptr)
					*(SQLSMALLINT*) ptr = record->fixedPrecScale,
					size = sizeof (SQLSMALLINT);
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		case SQL_DESC_LABEL:
			if(headType ==  odtImplementationRow)
			{
				if (record)
					string = (SQLCHAR*)record->label.getString();
			}
			else
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			break;

		case SQL_DESC_LITERAL_PREFIX:
			if(headType ==  odtImplementationRow)
			{
				if (record)
					string = (SQLCHAR*)record->literalPrefix.getString();
			}
			else
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			break;

		case SQL_DESC_LITERAL_SUFFIX:
			if(headType ==  odtImplementationRow)
			{
				if (record)
					string = (SQLCHAR*)record->literalSuffix.getString();
			}
			else
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			break;

		case SQL_DESC_LOCAL_TYPE_NAME:
			if(headType ==  odtImplementationRow)
			{
				if (record)
					string = (SQLCHAR*)record->localTypeName.getString();
			}
			else
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			break;

		case SQL_DESC_LENGTH:
			if (record && ptr)
				*(SQLUINTEGER*) ptr = record->length,
				size = sizeof (SQLUINTEGER);
			break;

		case SQL_DESC_NAME:
			switch(headType)
			{
			case odtImplementationRow:
			case odtImplementationParameter:
				if (record)
					string = (SQLCHAR*)record->name.getString();
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		case SQL_DESC_NULLABLE:
			switch(headType)
			{
			case odtImplementationRow:
			case odtImplementationParameter:
			if (record && ptr)
				*(SQLSMALLINT*) ptr = record->nullable,
				size = sizeof (SQLSMALLINT);
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		case SQL_DESC_OCTET_LENGTH:
			if (record && ptr)
				*(SQLINTEGER*) ptr = record->octetLength,
				size = sizeof (SQLINTEGER);	
			break;
			
		case SQL_DESC_OCTET_LENGTH_PTR:
			switch(headType)
			{
			case odtApplication:
			case odtApplicationParameter:
			case odtApplicationRow:
				if (record && ptr)
					*(SQLINTEGER **)ptr = record->octetLengthPtr,
					size = sizeof (SQLINTEGER*);
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		case SQL_DESC_PARAMETER_TYPE:
			if(headType == odtImplementationParameter)
			{
				if (record && ptr)
					*(SQLSMALLINT*) ptr = record->parameterType,
					size = sizeof (SQLSMALLINT);
			}
			else
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			break;

		case SQL_DESC_PRECISION:
			if (record && ptr)
				*(SQLSMALLINT*) ptr = record->precision,
				size = sizeof (SQLSMALLINT);
			break;

		case SQL_DESC_SCALE:
			if (record && ptr)
				*(SQLSMALLINT*) ptr = record->scale,
				size = sizeof (SQLSMALLINT);
			break;

		case SQL_DESC_SCHEMA_NAME:
			if(headType ==  odtImplementationRow)
			{
				if (record)
					string = (SQLCHAR*)record->schemaName.getString();
			}
			else
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			break;

		case SQL_DESC_SEARCHABLE:
			if(headType ==  odtImplementationRow)
			{
				if (record && ptr)
					*(SQLSMALLINT*) ptr = record->searchable,
					size = sizeof (SQLSMALLINT);
			}
			else
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			break;

		case SQL_DESC_TABLE_NAME:
			if(headType ==  odtImplementationRow)
			{
				if (record)
					string = (SQLCHAR*)record->tableName.getString();
			}
			else
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			break;

		case SQL_DESC_TYPE_NAME:
			switch(headType)
			{
			case odtImplementationRow:
			case odtImplementationParameter:
				if (record)
					string = (SQLCHAR*)record->typeName.getString();
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		case SQL_DESC_UNSIGNED:
			switch(headType)
			{
			case odtImplementationRow:
			case odtImplementationParameter:
				if (record && ptr)
					*(SQLSMALLINT*) ptr = record->unSigned,
					size = sizeof (SQLSMALLINT);
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		case SQL_DESC_UPDATABLE:
			if(headType ==  odtImplementationRow)
			{
				if (record && ptr)
					*(SQLSMALLINT*) ptr = record->updaTable,
					size = sizeof (SQLSMALLINT);
			}
			else
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			break;

		case SQL_DESC_INDICATOR_PTR:
			switch(headType)
			{
			case odtApplication:
			case odtApplicationParameter:
			case odtApplicationRow:
				if (record && ptr)
					*(SQLINTEGER **)ptr = record->indicatorPtr,
					size = sizeof (SQLINTEGER*);
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		case SQL_DESC_UNNAMED:
			switch(headType)
			{
			case odtImplementationRow:
			case odtImplementationParameter:
				if (record && ptr)
					*(SQLSMALLINT*) ptr = record->unNamed,
					size = sizeof (SQLSMALLINT);
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		case SQL_DESC_DATA_PTR:
			switch(headType)
			{
			case odtApplication:
			case odtApplicationParameter:
			case odtApplicationRow:
				if (record && ptr)
					*(SQLPOINTER*)ptr = record->dataPtr,
					size = sizeof (SQLPOINTER);
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		default:
			return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
		}

		if( size )
		{
			if (lengthPtr)
				*lengthPtr = size;
		}
		else if (string)
			return returnStringInfo (ptr, bufferLength, lengthPtr, (char*)string);

	}
	catch (SQLException& exception)
	{
		postError ("HY000", exception);
		return SQL_ERROR;
	}

	return sqlSuccess();
}

RETCODE OdbcDesc::sqlSetDescField(int recNumber, int fieldId, SQLPOINTER value, int length)
{
	clearErrors();
	DescRecord *record = NULL;

	if (recNumber)
		record = getDescRecord (recNumber);

	switch (fieldId)
		{
// Head
		case SQL_DESC_COUNT:
			switch(headType)
			{
			case odtApplication:
			case odtApplicationRow:
			case odtApplicationParameter:
			case odtImplementationParameter:
#pragma FB_COMPILER_MESSAGE("If modify value realized ReAlloc FIXME!")
				headCount = (SQLSMALLINT)value;
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		case SQL_DESC_ARRAY_SIZE:
			switch(headType)
			{
			case odtApplication:
			case odtApplicationParameter:
			case odtApplicationRow:
				headArraySize = (SQLUINTEGER)value;
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		case SQL_DESC_ARRAY_STATUS_PTR:
			headArrayStatusPtr = (SQLUSMALLINT*)value;
			break;

		case SQL_DESC_BIND_OFFSET_PTR:
			switch(headType)
			{
			case odtApplication:
			case odtApplicationParameter:
			case odtApplicationRow:
				headBindOffsetPtr = (SQLINTEGER*)value;
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		case SQL_DESC_BIND_TYPE:
			switch(headType)
			{
			case odtApplication:
			case odtApplicationParameter:
			case odtApplicationRow:
				headBindType = (SQLINTEGER)value;
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		case SQL_DESC_ROWS_PROCESSED_PTR:
			switch(headType)
			{
			case odtImplementationRow:
			case odtImplementationParameter:
				headRowsProcessedPtr = (SQLUINTEGER*)value;
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;
// Record
		case SQL_DESC_TYPE:
			switch(headType)
			{
			case odtApplication:
			case odtApplicationRow:
			case odtApplicationParameter:
			case odtImplementationParameter:
				if (record)
					record->type = (SQLSMALLINT)value;
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
//				return sqlReturn (SQL_NO_DATA_FOUND, "HY021", "Inconsistent descriptor information");
			}
			break;

		case SQL_DESC_DATETIME_INTERVAL_CODE:
			switch(headType)
			{ // DESC_MOST
			case odtApplication:
			case odtApplicationRow:
			case odtApplicationParameter:
			case odtImplementationParameter:
				if (record)
					record->datetimeIntervalCode = (SQLSMALLINT)value;
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		case SQL_DESC_CONCISE_TYPE:
			switch(headType)
			{ // DESC_MOST
			case odtApplication:
			case odtApplicationRow:
			case odtApplicationParameter:
			case odtImplementationParameter:
				if (record)
					record->conciseType = (SQLSMALLINT)value;
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		case SQL_DESC_DATETIME_INTERVAL_PRECISION:
			switch(headType)
			{ // DESC_MOST
			case odtApplication:
			case odtApplicationRow:
			case odtApplicationParameter:
			case odtImplementationParameter:
				if (record)
					record->datetimeIntervalPrecision = (SQLINTEGER)value;
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		case SQL_DESC_LENGTH:
			switch(headType)
			{ // DESC_MOST
			case odtApplication:
			case odtApplicationRow:
			case odtApplicationParameter:
			case odtImplementationParameter:
				if (record)
					record->length = (SQLUINTEGER)value;
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		case SQL_DESC_NAME:
			if(headType == odtImplementationParameter)
			{
				if (record)
					record->name = (const char*)value;
			}
			else
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			break;

		case SQL_DESC_OCTET_LENGTH:
			switch(headType)
			{ // DESC_MOST
			case odtApplication:
			case odtApplicationRow:
			case odtApplicationParameter:
			case odtImplementationParameter:
				if (record)
				{
					record->octetLength = (SQLINTEGER)value;
					if ( !record->length ) 
						record->length = (SQLINTEGER)value;
				}
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		case SQL_DESC_OCTET_LENGTH_PTR:
			switch(headType)
			{ 
			case odtApplication:
			case odtApplicationRow:
			case odtApplicationParameter:
				if (record)
					record->octetLengthPtr = (SQLINTEGER*)value;
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;
			
		case SQL_DESC_PARAMETER_TYPE:
			if(headType == odtImplementationParameter)
			{
				if (record)
					record->parameterType = (SQLSMALLINT)value;
			}
			else
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			break;

		case SQL_DESC_PRECISION:
			switch(headType)
			{ // DESC_MOST
			case odtApplication:
			case odtApplicationRow:
			case odtApplicationParameter:
			case odtImplementationParameter:
				if (record)
					record->precision = (SQLSMALLINT)value;
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		case SQL_DESC_SCALE:
			switch(headType)
			{ // DESC_MOST
			case odtApplication:
			case odtApplicationRow:
			case odtApplicationParameter:
			case odtImplementationParameter:
				if (record)
					record->scale = (SQLSMALLINT)value;
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		case SQL_DESC_INDICATOR_PTR:
			switch(headType)
			{ 
			case odtApplication:
			case odtApplicationRow:
			case odtApplicationParameter:
				if (record)
					record->indicatorPtr = (SQLINTEGER*)value;
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		case SQL_DESC_UNNAMED:
			if(headType == odtImplementationParameter)
			{
				if (record)
					record->unNamed = (SQLSMALLINT)value;
			}
			else
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			break;
		
		case SQL_DESC_DATA_PTR:
#pragma FB_COMPILER_MESSAGE("Consistency Checks ( help fn. SQLSetDescRec ) FIXME!")
			switch(headType)
			{ // DESC_MOST
			case odtApplication:
			case odtApplicationRow:
			case odtApplicationParameter:
				if (record)
					record->dataPtr = value;
			case odtImplementationParameter:
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		default:
			return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
		}

	return sqlSuccess();
}

DescRecord* OdbcDesc::getDescRecord(int number)
{
	if (number >= recordSlots)
	{
		int oldSlots = recordSlots;
		DescRecord **oldRecords = records;
		recordSlots = number + 20;
		records = new DescRecord* [recordSlots];
		memset (records, 0, sizeof (DescRecord*) * recordSlots);
		if (oldSlots)
		{
			memcpy (records, oldRecords, sizeof (DescRecord*) * oldSlots);
			delete [] oldRecords;
		}
	}

	if (number > headCount)
		headCount = number;

	DescRecord *record = records [number];

	if (record == NULL)
		records [number] = record = new DescRecord;

	return record;		
}

void OdbcDesc::allocBookmarkField()
{
	getDescRecord(0);
}

RETCODE OdbcDesc::sqlGetDescRec(	SQLSMALLINT recNumber, 
									SQLCHAR *name, 
									SQLSMALLINT bufferLength,
									SQLSMALLINT *stringLengthPtr, 
									SQLSMALLINT *typePtr, 
									SQLSMALLINT *subTypePtr, 
									SQLINTEGER  *lengthPtr, 
									SQLSMALLINT *precisionPtr, 
									SQLSMALLINT *scalePtr, 
									SQLSMALLINT *nullablePtr)
{
	RETCODE rc;
    clearErrors();
	DescRecord *record = NULL;

	if ( bDefined == false )
		return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");

	if ( recNumber > headCount )
		return sqlReturn (SQL_NO_DATA_FOUND, "HY021", "Inconsistent descriptor information");

	if ( !recNumber && headType == odtImplementationParameter )
			return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");

	record = getDescRecord (recNumber);

	try
	{
		rc = returnStringInfo (name, bufferLength,stringLengthPtr,record->name.getString());
		if( rc )
			return rc;

		*typePtr = record->type;
		*subTypePtr = record->datetimeIntervalCode;
		*lengthPtr = record->octetLength;
		*precisionPtr = record->precision;
		*scalePtr = record->scale;
		*nullablePtr = record->nullable;
	}
	catch (SQLException& exception)
	{
		postError ("HY000", exception);
		return SQL_ERROR;
	}

	return sqlSuccess();
}

RETCODE OdbcDesc::sqlSetDescRec(	SQLSMALLINT	recNumber,
									SQLSMALLINT	type,
									SQLSMALLINT	subType,
									SQLINTEGER	length,
									SQLSMALLINT	precision,
									SQLSMALLINT	scale,
									SQLPOINTER	dataPtr,
									SQLINTEGER *stringLengthPtr,
									SQLINTEGER *indicatorPtr)
{
    clearErrors();
	DescRecord *record = NULL;

	if(	headType == odtImplementationRow )
		return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");

	if ( bDefined == false )
		return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");

	if (recNumber)
	{
		if ( recNumber > headCount )
			return sqlReturn (SQL_NO_DATA_FOUND, "HY021", "Inconsistent descriptor information");

		record = getDescRecord (recNumber);
	}

	try
	{
		record->type = type;
		record->datetimeIntervalCode = subType;
		record->octetLength = length;
		record->precision = precision;
		record->scale = scale;
		record->dataPtr = dataPtr;
		record->octetLengthPtr = stringLengthPtr;
		record->indicatorPtr = indicatorPtr;
	}
	catch (SQLException& exception)
	{
		postError ("HY000", exception);
		return SQL_ERROR;
	}

	return sqlSuccess();
}
