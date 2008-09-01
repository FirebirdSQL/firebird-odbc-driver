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

// DescRecord.cpp: implementation of the DescRecord class.
//
//////////////////////////////////////////////////////////////////////

#include <memory.h>
#include <string.h>
#include "OdbcEnv.h"
#include "OdbcConnection.h"
#include "IscDbc/Connection.h"
#include "OdbcDesc.h"
#include "DescRecord.h"

namespace OdbcJdbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DescRecord::DescRecord()
{
	isDefined = false;
	isPrepared = false;
	isIndicatorSqlDa = false;
	isZeroColumn = false;
	isLocalDataPtr = false;
	localDataPtr = NULL;
	callType = 0;

	isBlobOrArray = 0;
	data_at_exec = false;
	startedTransfer	= false;
	sizeColumnExtendedFetch = 0;
	dataOffset = 0;
	currentFetched = 0;
	headSqlVarPtr = NULL;
	dataBlobPtr = NULL;

	type = SQL_C_DEFAULT;
	datetimeIntervalCode = 0;
	conciseType = SQL_C_DEFAULT;
	autoUniqueValue = 0;
	caseSensitive = 0;
	datetimeIntervalPrecision =0;
	displaySize = 0;
	fixedPrecScale = 0;
	numPrecRadix = 0;
	length = 0;
	nullable = 0;
	octetLength = 0;
	octetLengthPtr = (SQLLEN*)NULL;
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
#ifdef _WINDOWS
	WcsToMbs = _WcsToMbs;
	MbsToWcs = _MbsToWcs;
#else
	WcsToMbs = wcstombs;
	MbsToWcs = mbstowcs;
#endif
}

DescRecord::~DescRecord()
{
	releaseAllocMemory();
}

void DescRecord::releaseAllocMemory()
{
	if ( headSqlVarPtr )
	{
		headSqlVarPtr->release();
		headSqlVarPtr = NULL;
	}

	if ( dataBlobPtr )
	{
		dataBlobPtr->release();
		dataBlobPtr = NULL;
	}

	freeLocalDataPtr();
}

void DescRecord::setDefault(DescRecord *recTo)
{
	SQLLEN			*saveIndicatorPtr = recTo->indicatorPtr;
	SQLPOINTER		saveDataPtr = recTo->dataPtr;
	*recTo = this;
	recTo->indicatorPtr = saveIndicatorPtr;
	recTo->dataPtr = saveDataPtr;
}

void DescRecord::allocateLocalDataPtr(int length)
{
	freeLocalDataPtr();
	isLocalDataPtr = true;

	if ( !length )
		length = getBufferLength();

	localDataPtr = (char*)malloc( length );
}

void DescRecord::freeLocalDataPtr()
{
	if ( isLocalDataPtr )
	{
		isLocalDataPtr = false;
		free ( localDataPtr );
		localDataPtr = NULL;
	}
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

	if ( type != SQL_C_DEFAULT && conciseType == SQL_C_DEFAULT )
		conciseType = type;

	autoUniqueValue = rec->autoUniqueValue;
	caseSensitive = rec->caseSensitive;
	datetimeIntervalPrecision = rec->datetimeIntervalPrecision;
	displaySize = rec->displaySize;
	fixedPrecScale = rec->fixedPrecScale;
	numPrecRadix = rec->numPrecRadix;
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
	
	if ( indicatorPtr && !octetLengthPtr )
		octetLengthPtr = indicatorPtr;

	unNamed = rec->unNamed;
	dataPtr = rec->dataPtr;
	return true;
}

void DescRecord::initZeroColumn()
{
	autoUniqueValue = SQL_FALSE;
	caseSensitive = SQL_FALSE;
	catalogName = "";
	datetimeIntervalCode = 0;
	displaySize = 8;
	fixedPrecScale = SQL_FALSE;
	numPrecRadix = 0;
	label = "";
	literalPrefix = "";
	literalSuffix = "";
	localTypeName = "";
	name = "";
	nullable = SQL_NO_NULLS;
	octetLength = 4;
	precision = 4;
	scale = 0;
	schemaName = "";
	searchable = SQL_PRED_NONE;
	tableName = "";
	typeName = "";
	unNamed = SQL_UNNAMED;
	unSigned = SQL_FALSE;
	updaTable = SQL_ATTR_READONLY;
}

void DescRecord::beginBlobDataTransfer()
{
	dataBlobPtr->clear();
	startedTransfer = true;
}

void DescRecord::putBlobSegmentData (int length, const void *bytes)
{
	dataBlobPtr->putLongSegment (length, (char*)bytes);
}

void DescRecord::endBlobDataTransfer()
{
	startedTransfer = false;
}

}; // end namespace OdbcJdbcLibrary
