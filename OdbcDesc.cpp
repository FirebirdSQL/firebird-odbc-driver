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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "IscDbc/Connection.h"
#include "IscDbc/SQLException.h"
#include "OdbcJdbc.h"
#include "OdbcConnection.h"
#include "OdbcStatement.h"
#include "DescRecord.h"

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
	headBindOffsetPtr = (SQLINTEGER*)NULL;
	headRowsProcessedPtr = (SQLUINTEGER*)NULL;
	headCount = 0;
	headBindType = SQL_BIND_BY_COLUMN;

	if( headType == odtImplementationRow )
		bDefined = false;
	else
		bDefined = true;

	switch( headType )
	{
	case odtImplementationParameter:
	case odtImplementationRow:
	case odtImplementationGetData:
		convert = new OdbcConvert(parentStmt);
		listBind = new ListBindColumn;
		break;

	default:
		convert = NULL;
		listBind = NULL;
	}
}

void OdbcDesc::setParent(OdbcStatement *parent)
{ 
	parentStmt = parent;
	if ( convert )
		convert->setParent(parent);
}

void OdbcDesc::setDefaultImplDesc (StatementMetaData * ptMetaData)
{
	metaData = ptMetaData;

	if( headType == odtImplementationParameter )
		return;

	bDefined = false;
	removeRecords();
	delAllBindColumn();

	headAllocType = SQL_DESC_ALLOC_AUTO;
	headArraySize = 1;
	headArrayStatusPtr = (SQLUSMALLINT*)NULL;
	headBindOffsetPtr = (SQLINTEGER*)NULL;
	headRowsProcessedPtr = (SQLUINTEGER*)NULL;
	headCount = 0;

	if(	metaData == NULL )
		return;

	bDefined = true;

	headCount = metaData->getColumnCount();
	getDescRecord (headCount);
}

// Use to odtImplementationParameter and odtImplementationRow
void OdbcDesc::setBindOffsetPtrTo(SQLINTEGER *bindOffsetPtr, SQLINTEGER *bindOffsetPtrInd)
{ 
	convert->setBindOffsetPtrTo(bindOffsetPtr, bindOffsetPtrInd);
}

void OdbcDesc::setBindOffsetPtrFrom(SQLINTEGER	*bindOffsetPtr)
{ 
	convert->setBindOffsetPtrFrom(bindOffsetPtr);
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
 
void OdbcDesc::clearPrepared()
{
	if (records)
	{
		for (int n = 0; n < recordSlots; ++n)
			if ( records [n] )
				records [n]->isPrepared = false;
	}
}
 
void OdbcDesc::updateDefined()
{
	if (records)
	{
		int countColumn = metaData->getColumnCount();
		for (int n = 1; n <= countColumn; ++n)
			if (records [n] && records [n]->isDefined == false )
				defFromMetaData(n, records [n]);
	}
	bDefined = true;
}
 
void OdbcDesc::clearDefined()
{
	if (records)
	{
		for (int n = 0; n < recordSlots; ++n)
			if ( records [n] )
				records [n]->isDefined = false;
	}
	bDefined = false;
}
 
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

	delete convert;
	delete listBind;
}

// 	Info -> ASSERT( headType == odtImplementationGetData )
int OdbcDesc::setConvFnForGetData(int recNumber, DescRecord * recordTo, DescRecord *& recordIRD)
{
	recordIRD = getDescRecord(recNumber);

	if ( !recordIRD->isDefined )
		defFromMetaData(recNumber,recordIRD);

	if( recordTo->conciseType == SQL_C_DEFAULT )
	{
		recordIRD->setDefault(recordTo);
		recordTo->conciseType = getDefaultFromSQLToConciseType(recordIRD->type);
	}
//	else if ( !recordTo->isDefined )
//		defFromMetaData(recNumber,recordTo);

	recordTo->fnConv = convert->getAdresFunction(recordIRD,recordTo);
	recordTo->isPrepared = true;

	return convert->isIdentity() && recNumber;
}

void OdbcDesc::defFromMetaData(int recNumber, DescRecord * record)
{
	int realSqlType;

	record->autoUniqueValue = SQL_FALSE;
	record->caseSensitive = SQL_FALSE;
	record->catalogName = "";
	record->datetimeIntervalCode = 0;
	record->displaySize = metaData->getColumnDisplaySize(recNumber);
	record->fixedPrecScale = SQL_FALSE;
	record->label = metaData->getColumnLabel(recNumber);
	record->length = metaData->getColumnDisplaySize(recNumber);
	record->literalPrefix = "\"";
	record->literalSuffix = "\"";
	record->localTypeName = metaData->getSqlTypeName(recNumber);
	record->name = metaData->getColumnName(recNumber);
	record->baseColumnName = metaData->getColumnName(recNumber);
	record->nullable = metaData->isNullable(recNumber);
	record->octetLength = metaData->getPrecision(recNumber);
	record->precision = metaData->getPrecision(recNumber);
	record->scale = metaData->getScale(recNumber);
	record->schemaName = "";
	record->searchable = SQL_PRED_NONE;
	record->tableName = metaData->getTableName(recNumber);
	record->baseTableName = metaData->getTableName(recNumber);
	record->type = metaData->getColumnType(recNumber, realSqlType);
	record->conciseType = getConciseType(realSqlType);
	record->typeName = metaData->getColumnTypeName(recNumber);
	record->unNamed = !record->name.IsEmpty() ? SQL_NAMED : SQL_UNNAMED;
	record->unSigned = SQL_FALSE;
	record->updaTable = SQL_ATTR_WRITE;
	record->isDefined = true;

	metaData->getSqlData(recNumber, (char *&)record->dataPtr, (short *&)record->indicatorPtr, record->dataBlobPtr);
}

// 	Info -> ASSERT( headType == odtImplementationParameter )
int OdbcDesc::setConvFn(int recNumber, DescRecord * recordTo)
{
	if ( !metaData )
		return -1;

	DescRecord *record = getDescRecord(recNumber);

	if( recNumber == 0 )
		recordTo->setDefault(record);
	else
	{
		if ( !record->isDefined )
			defFromMetaData(recNumber,record);
		
		if( recordTo->conciseType == SQL_C_DEFAULT )
		{
			record->setDefault(recordTo);
			recordTo->conciseType = getDefaultFromSQLToConciseType(record->type);
		}
	}

	record->fnConv = convert->getAdresFunction(record,recordTo);
	record->isPrepared = true;
	recordTo->sizeColumnExtendedFetch = getConciseSize(recordTo->conciseType, recordTo->length);
	recordTo->isPrepared = true;

	return convert->isIdentity() && recNumber;
}

void OdbcDesc::addGetDataColumn(int recNumber, DescRecord * recordImp)
{
	DescRecord *record = getDescRecord(recNumber);
	CBindColumn bindCol(recNumber,recordImp,record);
	(*listBind)(recNumber) = bindCol;
}

RETCODE OdbcDesc::returnGetData(int recNumber)
{
	CBindColumn &bindCol = (*listBind)[recNumber];
	return (convert->*bindCol.appRecord->fnConv)(bindCol.impRecord,bindCol.appRecord);
}

void OdbcDesc::addBindColumn(int recNumber, DescRecord * recordApp)
{
	DescRecord *recordImp = getDescRecord(recNumber);
	CBindColumn bindCol(recNumber,recordImp,recordApp);

	int j = listBind->SearchAndInsert( &bindCol );
	if( j < 0 )
		(*listBind)[-j-1] = bindCol;
}

void OdbcDesc::delBindColumn(int recNumber)
{
}

void OdbcDesc::delAllBindColumn()
{
	listBind->removeAll();
}

// 	Info -> ASSERT( headType == odtImplementationRow )
void OdbcDesc::setZeroColumn(int rowNumber)
{
	CBindColumn * bindCol = listBind->GetHeadPosition();
	if ( bindCol && !bindCol->column )
		convert->setZeroColumn(bindCol->appRecord, rowNumber);
}

RETCODE OdbcDesc::returnData()
{
	RETCODE retCode, ret = SQL_SUCCESS;
	CBindColumn * bindCol = listBind->GetHeadPosition();
	while( bindCol )
	{
		retCode = (convert->*bindCol->impRecord->fnConv)(bindCol->impRecord,bindCol->appRecord);
		if ( retCode != SQL_SUCCESS )
		{
			ret = retCode;
			if ( ret != SQL_SUCCESS_WITH_INFO )
				break;
		}
		bindCol = listBind->GetNext();
	}
	return ret;
}

RETCODE OdbcDesc::returnDataFromExtededFetch()
{
	RETCODE retCode, ret = SQL_SUCCESS;
	SQLINTEGER	&bindOffsetPtrTo = convert->getBindOffsetPtrTo();
	SQLINTEGER	&currentRow = *parentStmt->bindOffsetPtr;

	CBindColumn * bindCol = listBind->GetHeadPosition();
	while( bindCol )
	{
		DescRecord *& appRecord = bindCol->appRecord;
		bindOffsetPtrTo = appRecord->sizeColumnExtendedFetch * currentRow;

		retCode = (convert->*bindCol->impRecord->fnConv)(bindCol->impRecord, appRecord);
		if ( retCode != SQL_SUCCESS )
		{
			ret = retCode;
			if ( ret != SQL_SUCCESS_WITH_INFO )
				break;
		}
		bindCol = listBind->GetNext();
	}
	return ret;
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

#ifdef DEBUG
#define __DebSetDescField(X) X,#X
struct infoDebSetDescField
{
	int kod;
	char * name;
} debSetDescField[]=
{
	__DebSetDescField(SQL_ERROR),
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
	__DebSetDescField(SQL_DESC_OCTET_LENGTH),
	__DebSetDescField(SQL_DESC_ALLOC_TYPE)
};
#endif

RETCODE OdbcDesc::sqlSetDescField(int recNumber, int fieldId, SQLPOINTER value, int length)
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
								recNumber, value ? (int)value : 0);
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
				headCount = (SQLSMALLINT)(int)value;
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
					record->type = (SQLSMALLINT)(int)value;
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
					record->datetimeIntervalCode = (SQLSMALLINT)(int)value;
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
					record->conciseType = (SQLSMALLINT)(int)value;
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
					record->parameterType = (SQLSMALLINT)(int)value;
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
					record->precision = (SQLSMALLINT)(int)value;
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
					record->scale = (SQLSMALLINT)(int)value;
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
					record->unNamed = (SQLSMALLINT)(int)value;
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
				{
					// help fn. SQLSetDescRec 
					record->dataPtr = value;
					if ( headType == odtApplicationRow )
					{
						int ret = parentStmt->implementationRowDescriptor->setConvFn(recNumber, record);
						if( ret == 1 )// isIdentity
						{ 
							// Temp
							// if ( isStaticCursor() )
								parentStmt->implementationRowDescriptor->addBindColumn(recNumber, record);
						}
						else if( ret == 0 )
							parentStmt->implementationRowDescriptor->addBindColumn(recNumber, record);
					}
				}
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

DescRecord* OdbcDesc::getDescRecord(int number, bool bCashe)
{
	if (number >= recordSlots)
	{
		int oldSlots = recordSlots;
		DescRecord **oldRecords = records;
		recordSlots = number + (bCashe ? 20 : 0);
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

int OdbcDesc::getConciseType(int type)
{
	switch ( type )
		{
		case JDBC_LONGVARBINARY:
		case JDBC_LONGVARCHAR:
			return SQL_C_BINARY;

		case JDBC_CHAR:
		case JDBC_VARCHAR:
			return SQL_C_CHAR;

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

		case JDBC_ARRAY:
			return SQL_C_BINARY;
		}

	return type;
}

int OdbcDesc::getDefaultFromSQLToConciseType(int sqlType)
{
	int cType;

	switch (sqlType)
	{
	case JDBC_CHAR:
	case JDBC_VARCHAR:
	case JDBC_LONGVARCHAR:
	case JDBC_DECIMAL:
	case JDBC_NUMERIC:
	case JDBC_ARRAY:
		cType = SQL_C_CHAR;
		break;
	case JDBC_SMALLINT:
		cType = SQL_C_SSHORT;
		break;
	case JDBC_INTEGER:
		cType = SQL_C_SLONG;
		break;
	case JDBC_BIGINT:
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
		cType = SQL_C_TYPE_DATE;
		break;
	case JDBC_TIME:
		cType = SQL_C_TYPE_TIME;
		break;
	case JDBC_TIMESTAMP:
		cType = SQL_C_TYPE_TIMESTAMP;
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

	case SQL_C_SHORT:
	case SQL_C_SSHORT:
	case SQL_C_USHORT:
		return sizeof(short);

	case SQL_C_LONG:
	case SQL_C_SLONG:
	case SQL_C_ULONG:
		return sizeof(long);

	case SQL_C_FLOAT:
		return sizeof(float);

	case SQL_C_DOUBLE:
		return sizeof(double);

	case SQL_C_BIT:
	case SQL_C_SBIGINT:
	case SQL_C_UBIGINT:
	case SQL_C_BINARY:
		return length;

	case SQL_C_DATE:
	case SQL_C_TIME:
	case SQL_C_TIMESTAMP:
	case SQL_TYPE_DATE:
	case SQL_TYPE_TIME:
	case SQL_TYPE_TIMESTAMP:
		return sizeof(TIMESTAMP_STRUCT);

	case SQL_C_NUMERIC:
	case SQL_DECIMAL:
		return 8;
	}

	return type;
}

}; // end namespace OdbcJdbcLibrary
