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
#include <stdio.h>
#include <stdlib.h>
#include "IscDbc/Connection.h"
#include "IscDbc/SQLException.h"
#include "OdbcJdbc.h"
#include "OdbcEnv.h"
#include "OdbcConnection.h"
#include "OdbcStatement.h"

namespace OdbcJdbcLibrary {

using namespace IscDbcLibrary;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

OdbcDesc::OdbcDesc(OdbcDescType type, OdbcConnection *connect)
{
	connection = connect;
	metaDataIn = NULL;
	metaDataOut = NULL;
	recordSlots = 0;
	records = NULL;

	headType = type;
	headAllocType = SQL_DESC_ALLOC_AUTO;
	headArraySize = 1;
	headArrayStatusPtr = (SQLUSMALLINT*)NULL;
	headBindOffsetPtr = (SQLLEN*)NULL;
	headRowsProcessedPtr = (SQLULEN*)NULL;
	headCount = 0;
	headBindType = SQL_BIND_BY_COLUMN;

	if( headType == odtImplementationRow )
		bDefined = false;
	else
		bDefined = true;
}

void OdbcDesc::setDefaultImplDesc (StatementMetaData * ptMetaDataOut, StatementMetaData * ptMetaDataIn)
{
	metaDataIn = ptMetaDataIn;
	metaDataOut = ptMetaDataOut;

	if( headType == odtImplementationParameter )
	{
		headCount = metaDataIn->getColumnCount();
		getDescRecord (headCount);
	}
	else
	{
		bDefined = false;
		removeRecords();

		headAllocType = SQL_DESC_ALLOC_AUTO;
		headArraySize = 1;
		headArrayStatusPtr = (SQLUSMALLINT*)NULL;
		headBindOffsetPtr = (SQLLEN*)NULL;
		headRowsProcessedPtr = (SQLULEN*)NULL;
		headCount = 0;

		if(	metaDataOut == NULL )
			return;

		headCount = metaDataOut->getColumnCount();
		getDescRecord (headCount);
		bDefined = headCount > 0;
	}
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
 
void OdbcDesc::releasePrepared()
{
	if (records)
	{
		for (int n = 0; n < recordSlots; ++n)
			if ( records [n] )
			{
				records [n]->isPrepared = false;
				records [n]->releaseAllocMemory();
			}
	}
}
 
void OdbcDesc::clearPrepared()
{
	if (records)
	{
		for (int n = 0; n < recordSlots; ++n)
			if ( records [n] )
			{
				records [n]->isPrepared = false;
				records [n]->freeLocalDataPtr();
				if ( records [n]->headSqlVarPtr )
					records [n]->headSqlVarPtr->restoreOrgPtrSqlData();
			}
	}
}
 
void OdbcDesc::updateDefinedIn()
{
	if (records)
	{
		for (int n = 1; n <= metaDataIn->getColumnCount(); n++)
		{
			DescRecord * record = records[n];
			if ( record )
			{
				record->freeLocalDataPtr();
				defFromMetaDataIn(n, record);
			}
		}
	}
}
 
void OdbcDesc::updateDefinedOut()
{
	if (records)
	{
		for (int n = 1; n <= metaDataOut->getColumnCount(); ++n)
		{
			DescRecord * record = records[n];
			if ( record && record->isDefined == false )
			{
				record->freeLocalDataPtr();
				defFromMetaDataOut(n, record );
			}
		}
	}
	bDefined = true;
}

void OdbcDesc::clearDefined()
{
	if (records)
	{
		for (int n = 0; n < recordSlots; ++n)
		{
			DescRecord * rec = records[n];
			if ( rec )
			{
				rec->isDefined = false;
				rec->currentFetched = 0;
			}
		}
	}
	bDefined = false;
}

SQLRETURN OdbcDesc::operator =(OdbcDesc &sour)
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
	{
		DescRecord *srcrec = sour.records[n];
		DescRecord &rec = *getDescRecord ( n );

		if ( srcrec )
		{
			rec = srcrec;
			rec.sizeColumnExtendedFetch = srcrec->sizeColumnExtendedFetch;
		}

		rec.isDefined = true;
	}

	return sqlSuccess();
}

OdbcDesc::~OdbcDesc()
{
	if (connection)
		connection->descriptorDeleted (this);

	removeRecords();
}

void OdbcDesc::defFromMetaDataIn(int recNumber, DescRecord * record)
{
	int realSqlType;

	record->autoUniqueValue = SQL_FALSE;
	record->caseSensitive = SQL_FALSE;
	record->catalogName = "";
	record->datetimeIntervalCode = 0;
	record->displaySize = metaDataIn->getColumnDisplaySize(recNumber);
	record->fixedPrecScale = SQL_FALSE;
	record->label = metaDataIn->getColumnLabel(recNumber);
	record->length = metaDataIn->getPrecision(recNumber);
	record->literalPrefix = "\"";
	record->literalSuffix = "\"";
	record->localTypeName = metaDataIn->getSqlTypeName(recNumber);
	record->name = metaDataIn->getColumnLabel(recNumber);
	record->baseColumnName = metaDataIn->getColumnName(recNumber);
	record->nullable = metaDataIn->isNullable(recNumber);
	record->octetLength = metaDataIn->getColumnDisplaySize(recNumber);
	record->precision = metaDataIn->getPrecision(recNumber);
	record->numPrecRadix = metaDataIn->getNumPrecRadix(recNumber);
	record->scale = metaDataIn->getScale(recNumber);
	record->schemaName = "";
	record->searchable = SQL_PRED_NONE;
	record->tableName = metaDataIn->getTableName(recNumber);
	record->baseTableName = metaDataIn->getTableName(recNumber);
	record->type = metaDataIn->getColumnType(recNumber, realSqlType);
	record->conciseType = getConciseType(realSqlType);
	record->typeName = metaDataIn->getColumnTypeName(recNumber);
	record->unNamed = !record->name.IsEmpty() ? SQL_NAMED : SQL_UNNAMED;
	record->unSigned = SQL_FALSE;
	record->updaTable = SQL_ATTR_WRITE;
	record->MbsToWcs = metaDataIn->getAdressMbsToWcs( recNumber );
	record->WcsToMbs = metaDataIn->getAdressWcsToMbs( recNumber );
	record->isDefined = true;

	record->isBlobOrArray = metaDataIn->isBlobOrArray (recNumber);

	metaDataIn->getSqlData(recNumber, record->dataBlobPtr, record->headSqlVarPtr);
	record->dataPtr = (SQLPOINTER)record->headSqlVarPtr->getSqlData();
	record->indicatorPtr = (SQLLEN*)record->headSqlVarPtr->getSqlInd();
}

void OdbcDesc::defFromMetaDataOut(int recNumber, DescRecord * record)
{
	int realSqlType;

	record->autoUniqueValue = SQL_FALSE;
	record->caseSensitive = SQL_FALSE;
	record->catalogName = "";
	record->datetimeIntervalCode = 0;
	record->displaySize = metaDataOut->getColumnDisplaySize(recNumber);
	record->fixedPrecScale = SQL_FALSE;
	record->label = metaDataOut->getColumnLabel(recNumber);
	record->length = metaDataOut->getPrecision(recNumber);
	record->literalPrefix = "\"";
	record->literalSuffix = "\"";
	record->localTypeName = metaDataOut->getSqlTypeName(recNumber);
	record->name = metaDataOut->getColumnLabel(recNumber);
	record->baseColumnName = metaDataOut->getColumnName(recNumber);
	record->nullable = metaDataOut->isNullable(recNumber);
	record->octetLength = metaDataOut->getColumnDisplaySize(recNumber);
	record->precision = metaDataOut->getPrecision(recNumber);
	record->numPrecRadix = metaDataOut->getNumPrecRadix(recNumber);
	record->scale = metaDataOut->getScale(recNumber);
	record->schemaName = "";
	record->searchable = SQL_PRED_NONE;
	record->tableName = metaDataOut->getTableName(recNumber);
	record->baseTableName = metaDataOut->getTableName(recNumber);
	record->type = metaDataOut->getColumnType(recNumber, realSqlType);
	record->conciseType = getConciseType(realSqlType);
	record->typeName = metaDataOut->getColumnTypeName(recNumber);
	record->unNamed = !record->name.IsEmpty() ? SQL_NAMED : SQL_UNNAMED;
	record->unSigned = SQL_FALSE;
	record->updaTable = SQL_ATTR_WRITE;
	record->MbsToWcs = metaDataOut->getAdressMbsToWcs( recNumber );
	record->WcsToMbs = metaDataOut->getAdressWcsToMbs( recNumber );
	record->isDefined = true;

	metaDataOut->getSqlData(recNumber, record->dataBlobPtr, record->headSqlVarPtr);
	record->dataPtr = (SQLPOINTER)record->headSqlVarPtr->getSqlData();
	record->indicatorPtr = (SQLLEN*)record->headSqlVarPtr->getSqlInd();
}

OdbcConnection* OdbcDesc::getConnection()
{
	return connection;
}

OdbcObjectType OdbcDesc::getType()
{
	return odbcTypeDescriptor;
}

SQLRETURN OdbcDesc::sqlGetDescField(int recNumber, int fieldId, SQLPOINTER ptr, int bufferLength, SQLINTEGER *lengthPtr)
{
    clearErrors();
	SQLINTEGER size = 0;
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
				if (ptr)
					*(SQLSMALLINT*) ptr = headAllocType,
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
					*(SQLLEN **)ptr = headBindOffsetPtr,
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
					*(SQLULEN**)ptr = headRowsProcessedPtr,
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

		case SQL_DESC_NUM_PREC_RADIX:
			if (record && ptr)
				*(SQLINTEGER*) ptr = record->numPrecRadix,
				size = sizeof (SQLINTEGER);	
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
					*(SQLLEN **)ptr = record->octetLengthPtr,
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
					*(SQLLEN **)ptr = record->indicatorPtr,
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
	catch ( std::exception &ex )
	{
		SQLException &exception = (SQLException&)ex;
		postError ("HY000", exception);
		return SQL_ERROR;
	}

	return sqlSuccess();
}

#ifdef DEBUG
#define __DebSetDescField(X) X,#X
struct infoDebSetDescField
{
	int kod;
	char * name;
} debSetDescField[]=
{
	__DebSetDescField(SQL_ERROR),
	__DebSetDescField(SQL_DESC_PARAMETER_TYPE),
	__DebSetDescField(SQL_DESC_CONCISE_TYPE),
	__DebSetDescField(SQL_DESC_COUNT),
	__DebSetDescField(SQL_DESC_TYPE),
	__DebSetDescField(SQL_DESC_LENGTH),
	__DebSetDescField(SQL_DESC_OCTET_LENGTH_PTR),
	__DebSetDescField(SQL_DESC_PRECISION),
	__DebSetDescField(SQL_DESC_SCALE),
	__DebSetDescField(SQL_DESC_DATETIME_INTERVAL_CODE),
	__DebSetDescField(SQL_DESC_NULLABLE),
	__DebSetDescField(SQL_DESC_INDICATOR_PTR),
	__DebSetDescField(SQL_DESC_DATA_PTR),
	__DebSetDescField(SQL_DESC_NAME),
	__DebSetDescField(SQL_DESC_UNNAMED),
	__DebSetDescField(SQL_DESC_NUM_PREC_RADIX),
	__DebSetDescField(SQL_DESC_OCTET_LENGTH),
	__DebSetDescField(SQL_DESC_ALLOC_TYPE)
};
#endif

SQLRETURN OdbcDesc::sqlSetDescField(int recNumber, int fieldId, SQLPOINTER value, int length)
{
#ifdef DEBUG
	char strTmp[128];
	int i;
	for(i=0;i<sizeof(debSetDescField)/sizeof(*debSetDescField);++i)
		if(debSetDescField[i].kod == fieldId)
			break;
	if(i==sizeof(debSetDescField)/sizeof(*debSetDescField))
		i=0;
	sprintf(strTmp,"\tid %4i - %s : recNumber %i : value %i\n",fieldId,debSetDescField[i].name,
								recNumber, value ? (intptr_t)value : 0);
	OutputDebugString(strTmp); 
#endif
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
				headCount = (SQLSMALLINT)(intptr_t)value;
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
				headArraySize = (uintptr_t)value;
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
				headBindOffsetPtr = (SQLLEN*)value;
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
				headBindType = (intptr_t)value;
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
				headRowsProcessedPtr = (SQLULEN*)value;
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
				{
#pragma FB_COMPILER_MESSAGE("This temporary decision. FIXME!")
					record->type = (SQLSMALLINT)(intptr_t)value;
					record->conciseType = (SQLSMALLINT)(intptr_t)value;
				}
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
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
					record->datetimeIntervalCode = (SQLSMALLINT)(intptr_t)value;
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
				{
#pragma FB_COMPILER_MESSAGE("This temporary decision. FIXME!")
					record->conciseType = (SQLSMALLINT)(intptr_t)value;
					record->type = (SQLSMALLINT)(intptr_t)value;
				}
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
					record->datetimeIntervalPrecision = (intptr_t)value;
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
					record->length = (uintptr_t)value;
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

		case SQL_DESC_NUM_PREC_RADIX:
			switch(headType)
			{ // DESC_MOST
			case odtApplication:
			case odtApplicationRow:
			case odtApplicationParameter:
			case odtImplementationParameter:
				if (record)
					record->numPrecRadix = (intptr_t)value;
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
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
					record->octetLength = (intptr_t)value;
					record->sizeColumnExtendedFetch = (intptr_t)value;
					if ( !record->length ) 
						record->length = (intptr_t)value;
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
					record->octetLengthPtr = (SQLLEN*)value;
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;
			
		case SQL_DESC_PARAMETER_TYPE:
			if(headType == odtImplementationParameter)
			{
				if (record)
					record->parameterType = (SQLSMALLINT)(intptr_t)value;
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
					record->precision = (SQLSMALLINT)(intptr_t)value;
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
					record->scale = (SQLSMALLINT)(intptr_t)value;
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
					record->indicatorPtr = (SQLLEN*)value;
				break;
			default:
				return sqlReturn (SQL_ERROR, "HY091", "Invalid descriptor field identifier");
			}
			break;

		case SQL_DESC_UNNAMED:
			if(headType == odtImplementationParameter)
			{
				if (record)
					record->unNamed = (SQLSMALLINT)(intptr_t)value;
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
				{	// help fn. SQLSetDescRec 
					record->dataPtr = value;
					record->isDefined = true;
					record->isPrepared = false;
				}
				break;
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

void OdbcDesc::allocBookmarkField()
{
	getDescRecord(0);
}

SQLRETURN OdbcDesc::sqlGetDescRec(	SQLSMALLINT recNumber, 
									SQLCHAR *name, 
									SQLSMALLINT bufferLength,
									SQLSMALLINT *stringLengthPtr, 
									SQLSMALLINT *typePtr, 
									SQLSMALLINT *subTypePtr, 
									SQLLEN  *lengthPtr, 
									SQLSMALLINT *precisionPtr, 
									SQLSMALLINT *scalePtr, 
									SQLSMALLINT *nullablePtr)
{
	SQLRETURN rc;
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
	catch ( std::exception &ex )
	{
		SQLException &exception = (SQLException&)ex;
		postError ("HY000", exception);
		return SQL_ERROR;
	}

	return sqlSuccess();
}

SQLRETURN OdbcDesc::sqlSetDescRec(	SQLSMALLINT	recNumber,
									SQLSMALLINT	type,
									SQLSMALLINT	subType,
									SQLINTEGER	length,
									SQLSMALLINT	precision,
									SQLSMALLINT	scale,
									SQLPOINTER	dataPtr,
									SQLLEN *stringLengthPtr,
									SQLLEN *indicatorPtr)
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
	catch ( std::exception &ex )
	{
		SQLException &exception = (SQLException&)ex;
		postError ("HY000", exception);
		return SQL_ERROR;
	}

	return sqlSuccess();
}

int OdbcDesc::getConciseType(int type)
{
	switch ( type )
	{
	case JDBC_LONGVARBINARY:
	case JDBC_BINARY:
	case JDBC_VARBINARY:
		return SQL_C_BINARY;

	case JDBC_LONGVARCHAR:
	case JDBC_CHAR:
	case JDBC_VARCHAR:
		return SQL_C_CHAR;

	case JDBC_WLONGVARCHAR:
	case JDBC_WCHAR:
	case JDBC_WVARCHAR:
		return SQL_C_WCHAR;

	case JDBC_BOOLEAN:
		return SQL_C_BIT;
		
	case JDBC_TINYINT:
		return SQL_C_STINYINT;

	case JDBC_SMALLINT:
		return SQL_C_SSHORT;

	case JDBC_INTEGER:
		return SQL_C_SLONG;

	case JDBC_BIGINT:
		return SQL_C_SBIGINT;

	case JDBC_REAL:
		return SQL_C_FLOAT;

	case JDBC_FLOAT:
	case JDBC_DOUBLE:
		return SQL_C_DOUBLE;

	case JDBC_DATE:
		return SQL_C_TYPE_DATE;

	case JDBC_SQL_DATE:
		return SQL_C_DATE;

	case JDBC_TIME:
		return SQL_C_TYPE_TIME;

	case JDBC_SQL_TIME:
		return SQL_C_TIME;

	case JDBC_TIMESTAMP:
		return SQL_C_TYPE_TIMESTAMP;

	case JDBC_SQL_TIMESTAMP:
		return SQL_C_TIMESTAMP;

	case JDBC_INTERVAL_YEAR:
		return SQL_C_INTERVAL_YEAR;

	case JDBC_INTERVAL_MONTH:
		return SQL_C_INTERVAL_MONTH;

	case JDBC_INTERVAL_DAY:
		return SQL_C_INTERVAL_DAY;

	case JDBC_INTERVAL_HOUR:
		return SQL_C_INTERVAL_HOUR;

	case JDBC_INTERVAL_MINUTE:
		return SQL_C_INTERVAL_MINUTE;

	case JDBC_INTERVAL_SECOND:
		return SQL_C_INTERVAL_SECOND;

	case JDBC_INTERVAL_YEAR_TO_MONTH:
		return SQL_C_INTERVAL_YEAR_TO_MONTH;

	case JDBC_INTERVAL_DAY_TO_HOUR:
		return SQL_C_INTERVAL_DAY_TO_HOUR;

	case JDBC_INTERVAL_DAY_TO_MINUTE:
		return SQL_C_INTERVAL_DAY_TO_MINUTE;

	case JDBC_INTERVAL_DAY_TO_SECOND:
		return SQL_C_INTERVAL_DAY_TO_SECOND;

	case JDBC_INTERVAL_HOUR_TO_MINUTE:
		return SQL_C_INTERVAL_HOUR_TO_MINUTE;

	case JDBC_INTERVAL_HOUR_TO_SECOND:
		return SQL_C_INTERVAL_HOUR_TO_SECOND;

	case JDBC_INTERVAL_MINUTE_TO_SECOND:
		return SQL_C_INTERVAL_MINUTE_TO_SECOND;
	}

	return type;
}

//
// This extremely for set type App from Firebird database
//
int OdbcDesc::getDefaultFromSQLToConciseType(int sqlType, int bufferLength)
{
	int cType;

	switch (sqlType)
	{
	case JDBC_CHAR:
	case JDBC_VARCHAR:
	case JDBC_LONGVARCHAR:
	case JDBC_DECIMAL:
	case JDBC_NUMERIC:
	case JDBC_WCHAR:
	case JDBC_WVARCHAR:
	case JDBC_WLONGVARCHAR:
		cType = SQL_C_CHAR;
		break;
/*
	case JDBC_WCHAR:
	case JDBC_WVARCHAR:
	case JDBC_WLONGVARCHAR:
		cType = SQL_C_WCHAR;
		break;
*/
	case JDBC_BOOLEAN:
		cType = SQL_C_BIT;
		break;
	case JDBC_TINYINT:
		cType = SQL_C_STINYINT;
		break;
	case JDBC_SMALLINT:
		cType = SQL_C_SSHORT;
		break;
	case JDBC_INTEGER:
		cType = SQL_C_SLONG;
		break;
	case JDBC_BIGINT:
		if ( bufferLength )
			cType = SQL_C_CHAR;
		else
			cType = SQL_C_SBIGINT;
		break;
	case JDBC_REAL:
		cType = SQL_C_FLOAT;
		break;
	case JDBC_FLOAT:
	case JDBC_DOUBLE:
		cType = SQL_C_DOUBLE;
		break;
	case JDBC_BINARY:
	case JDBC_VARBINARY:
	case JDBC_LONGVARBINARY:
		cType = SQL_C_BINARY;
		break;
	case JDBC_SQL_DATE:
		cType = SQL_C_DATE;
		break;
	case JDBC_SQL_TIME:
		cType = SQL_C_TIME;
		break;
	case JDBC_SQL_TIMESTAMP:
		cType = SQL_C_TIMESTAMP;
		break;
	case JDBC_DATE:
		if ( bufferLength == 11 ) // standart for string '1992-12-12' + '\0'
			cType = SQL_C_CHAR;
		else
			cType = SQL_C_TYPE_DATE;
		break;
	case JDBC_TIME:
		if ( bufferLength == 14 ) // standart for string 'hh:mm:ss.mmmm' + '\0'
			cType = SQL_C_CHAR;
		else
			cType = SQL_C_TYPE_TIME;
		break;
	case JDBC_TIMESTAMP:
		if ( bufferLength == 25 ) // standart for string '1992-12-12 hh:mm:ss.mmmm' + '\0'
			cType = SQL_C_CHAR;
		else
			cType = SQL_C_TYPE_TIMESTAMP;
		break;
	case JDBC_INTERVAL_YEAR:
		cType = SQL_C_INTERVAL_YEAR;
		break;
	case JDBC_INTERVAL_MONTH:
		cType = SQL_C_INTERVAL_MONTH;
		break;
	case JDBC_INTERVAL_DAY:
		cType = SQL_C_INTERVAL_DAY;
		break;
	case JDBC_INTERVAL_HOUR:
		cType = SQL_C_INTERVAL_HOUR;
		break;
	case JDBC_INTERVAL_MINUTE:
		cType = SQL_C_INTERVAL_MINUTE;
		break;
	case JDBC_INTERVAL_SECOND:
		cType = SQL_C_INTERVAL_SECOND;
		break;
	case JDBC_INTERVAL_YEAR_TO_MONTH:
		cType = SQL_C_INTERVAL_YEAR_TO_MONTH;
		break;
	case JDBC_INTERVAL_DAY_TO_HOUR:
		cType = SQL_C_INTERVAL_DAY_TO_HOUR;
		break;
	case JDBC_INTERVAL_DAY_TO_MINUTE:
		cType = SQL_C_INTERVAL_DAY_TO_MINUTE;
		break;
	case JDBC_INTERVAL_DAY_TO_SECOND:
		cType = SQL_C_INTERVAL_DAY_TO_SECOND;
		break;
	case JDBC_INTERVAL_HOUR_TO_MINUTE:
		cType = SQL_C_INTERVAL_HOUR_TO_MINUTE;
		break;
	case JDBC_INTERVAL_HOUR_TO_SECOND:
		cType = SQL_C_INTERVAL_HOUR_TO_SECOND;
		break;
	case JDBC_INTERVAL_MINUTE_TO_SECOND:
		cType = SQL_C_INTERVAL_MINUTE_TO_SECOND;
		break;
	default:
		cType = SQL_C_DEFAULT;
		break;
	}
	return cType;
}

int OdbcDesc::getConciseSize(int type, int length)
{
	switch ( type )
	{
	case SQL_C_CHAR:
		return length;

	case SQL_C_WCHAR:
		return length;

	case SQL_C_BIT:
	case SQL_C_TINYINT:
	case SQL_C_STINYINT:
	case SQL_C_UTINYINT:
		return sizeof(char);

	case SQL_C_SHORT:
	case SQL_C_SSHORT:
	case SQL_C_USHORT:
		return sizeof(short);

	case SQL_C_LONG:
	case SQL_C_SLONG:
	case SQL_C_ULONG:
		return sizeof(int);

	case SQL_C_FLOAT:
		return sizeof(float);

	case SQL_C_DOUBLE:
		return sizeof(double);

	case SQL_C_BINARY:
		return length;

	case SQL_C_DATE:
	case SQL_TYPE_DATE:
		return sizeof(DATE_STRUCT);

	case SQL_C_TIME:
	case SQL_TYPE_TIME:
		return sizeof(TIME_STRUCT);

	case SQL_C_TIMESTAMP:
	case SQL_TYPE_TIMESTAMP:
		return sizeof(TIMESTAMP_STRUCT);

	case SQL_C_SBIGINT:
	case SQL_C_UBIGINT:
		return 8;

	case SQL_DECIMAL:
	case SQL_C_NUMERIC:
		return sizeof(tagSQL_NUMERIC_STRUCT);

	default:
		if ( type >= SQL_C_INTERVAL_YEAR &&
			 type <= SQL_C_INTERVAL_MINUTE_TO_SECOND )
			return sizeof(SQL_INTERVAL_STRUCT);
	}

	return type;
}

}; // end namespace OdbcJdbcLibrary
