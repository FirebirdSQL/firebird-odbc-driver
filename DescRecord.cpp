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

// DescRecord.cpp: implementation of the DescRecord class.
//
//////////////////////////////////////////////////////////////////////

#include "OdbcDesc.h"
#include "DescRecord.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DescRecord::DescRecord()
{
	data_at_exec = false;
	startedTransfer	= false;

	type = SQL_C_DEFAULT;
	datetimeIntervalCode = 0;
	conciseType = SQL_C_DEFAULT;
	autoUniqueValue = 0;
	caseSensitive = 0;
	datetimeIntervalPrecision =0;
	displaySize = 0;
	fixedPrecScale = 0;
	length = 0;
	nullable = 0;
	octetLength = 0;
	octetLengthPtr = (SQLINTEGER*)NULL;
	parameterType = SQL_PARAM_INPUT;
	precision = 0;
	scale = 0;
	searchable = 0;
	unSigned = 0;
	updaTable = 0;
	indicatorPtr = NULL;
	unNamed = SQL_NAMED;
	dataPtr = NULL;
	fnConv = NULL;
}

DescRecord::~DescRecord()
{

}

void DescRecord::setDefault(DescRecord *recTo)
{
	SQLINTEGER		*saveIndicatorPtr = recTo->indicatorPtr;
	SQLPOINTER		saveDataPtr = recTo->dataPtr;
	*recTo = this;
	recTo->indicatorPtr = saveIndicatorPtr;
	recTo->dataPtr = saveDataPtr;
}

bool DescRecord::operator =(DescRecord *rec)
{
	if ( !rec )
		return false;
	if ( !this )
		return false;

	data_at_exec = rec->data_at_exec;
	startedTransfer	= rec->startedTransfer;

	type = rec->type;
	datetimeIntervalCode = rec->datetimeIntervalCode;
	conciseType = rec->conciseType;
	autoUniqueValue = rec->autoUniqueValue;
	caseSensitive = rec->caseSensitive;
	datetimeIntervalPrecision = rec->datetimeIntervalPrecision;
	displaySize = rec->displaySize;
	fixedPrecScale = rec->fixedPrecScale;
	length = rec->length;
	nullable = rec->nullable;
	octetLength = rec->octetLength;
	octetLengthPtr = rec->octetLengthPtr;
//	parameterType = rec->parameterType;
	precision = rec->precision;
	scale = rec->scale;
	searchable = rec->searchable;
	unSigned = rec->unSigned;
	updaTable = rec->updaTable;
	indicatorPtr = rec->indicatorPtr;
	unNamed = rec->unNamed;
	dataPtr = rec->dataPtr;
	return true;
}

// Некоторые поля IPD определены только после того, 
// как IPD автоматически заполнился драйвером. 
// Если не, они неопределены. 
// Эти поля - 
//	SQL_DESC_CASE_SENSITIVE, 
//	SQL_DESC_FIXED_PREC_SCALE, 
//	SQL_DESC_TYPE_NAME, 
//	SQL_DESC_UNSIGNED, 
//	и SQL_DESC_LOCAL_TYPE_NAME. 

// IRD
// ==========================
// Після препаре визначити ці поля
// Record fields
// SQL_DESC_AUTO_UNIQUE_VALUE,	TEXT("SQL_DESC_AUTO_UNIQUE_VALUE"),			DESC_IRD,		DESC_NONE,	DESC_NONE,	0,								0,						sizeof(SQLINTEGER),		SQL_IS_INTEGER,
// SQL_DESC_BASE_COLUMN_NAME,	TEXT("SQL_DESC_BASE_COLUMN_NAME"),			DESC_IRD,		DESC_NONE,	DESC_NONE,	(SQLINTEGER)szParamName,		0,						sizeof(SQLCHAR),		0,
// SQL_DESC_CASE_SENSITIVE,		TEXT("SQL_DESC_CASE_SENSITIVE"),			DESC_ID,		DESC_NONE,	DESC_NONE,	0,								0,						sizeof(SQLINTEGER),		SQL_IS_INTEGER,
// SQL_DESC_CATALOG_NAME,		TEXT("SQL_DESC_CATALOG_NAME"),				DESC_IRD,		DESC_NONE,	DESC_NONE,	(SQLINTEGER)szParamName,		0,						sizeof(SQLCHAR),		0,
// SQL_DESC_DISPLAY_SIZE,		TEXT("SQL_DESC_DISPLAY_SIZE"),				DESC_IRD,		DESC_NONE, 	DESC_NONE,	0,								0,						sizeof(SQLINTEGER),		SQL_IS_INTEGER,
// SQL_DESC_FIXED_PREC_SCALE,	TEXT("SQL_DESC_FIXED_PREC_SCALE"),			DESC_ID,		DESC_NONE,	DESC_NONE,	0,								0,						sizeof(SQLSMALLINT),	SQL_IS_SMALLINT,
// SQL_DESC_LABEL,				TEXT("SQL_DESC_LABEL"),						DESC_IRD,		DESC_NONE,	DESC_NONE,	(SQLINTEGER)szParamName,		0,						sizeof(SQLCHAR),		0,
// SQL_DESC_LITERAL_PREFIX,		TEXT("SQL_DESC_LITERAL_PREFIX"),			DESC_IRD,		DESC_NONE,	DESC_NONE,	(SQLINTEGER)szParamName,		0,						sizeof(SQLCHAR),		0,
// SQL_DESC_LITERAL_SUFFIX,		TEXT("SQL_DESC_LITERAL_SUFFIX"),			DESC_IRD,		DESC_NONE,	DESC_NONE,	(SQLINTEGER)szParamName,		0,						sizeof(SQLCHAR),		0,
// SQL_DESC_LOCAL_TYPE_NAME,	TEXT("SQL_DESC_LOCAL_TYPE_NAME"),			DESC_IRD,		DESC_NONE,	DESC_NONE,	(SQLINTEGER)szParamName,		0,						sizeof(SQLCHAR),		0,
// SQL_DESC_NULLABLE,			TEXT("SQL_DESC_NULLABLE"),					DESC_ID,		DESC_NONE,	DESC_NONE,	0,								0,						sizeof(SQLSMALLINT),	SQL_IS_SMALLINT,
// SQL_DESC_SCHEMA_NAME,		TEXT("SQL_DESC_SCHEMA_NAME"),				DESC_IRD,		DESC_NONE,	DESC_NONE,	(SQLINTEGER)szParamName,		0,						sizeof(SQLCHAR),		0,
// SQL_DESC_SEARCHABLE,			TEXT("SQL_DESC_SEARCHABLE"),				DESC_IRD,		DESC_NONE,	DESC_NONE,	0,								0,						sizeof(SQLSMALLINT),	SQL_IS_SMALLINT,
// SQL_DESC_TABLE_NAME,			TEXT("SQL_DESC_TABLE_NAME"),				DESC_IRD,		DESC_NONE,	DESC_NONE,	(SQLINTEGER)szParamName,		0,						sizeof(SQLCHAR),		0,
// SQL_DESC_TYPE_NAME,			TEXT("SQL_DESC_TYPE_NAME"),					DESC_ID,		DESC_NONE,	DESC_NONE,	(SQLINTEGER)szParamName,		0,						sizeof(SQLCHAR),		0,
// SQL_DESC_UNSIGNED,			TEXT("SQL_DESC_UNSIGNED"),					DESC_ID,		DESC_NONE,	DESC_NONE,	0,								0,						sizeof(SQLSMALLINT),	SQL_IS_SMALLINT,
// SQL_DESC_UPDATABLE,			TEXT("SQL_DESC_UPDATABLE"),					DESC_IRD,		DESC_NONE,	DESC_NONE,	0,								0,						sizeof(SQLSMALLINT),	SQL_IS_SMALLINT,
