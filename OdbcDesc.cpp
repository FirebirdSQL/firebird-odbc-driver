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

// OdbcDesc.cpp: implementation of the OdbcDesc class.
//
//////////////////////////////////////////////////////////////////////

#include <memory.h>
#include "OdbcDesc.h"
#include "OdbcConnection.h"
#include "DescRecord.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

OdbcDesc::OdbcDesc(OdbcDescType type, OdbcConnection *connect)
{
	connection = connect;
	descType = type;
	recordSlots = 0;
	records = NULL;
	descArraySize = 0;
	rowsProcessedPtr = NULL;
}

OdbcDesc::~OdbcDesc()
{
	if (connection)
		connection->descriptorDeleted (this);

	if (records)
		{
		for (int n = 0; n < recordSlots; ++n)
			if (records [n])
				delete records [n];
		delete [] records;
		}
}

OdbcObjectType OdbcDesc::getType()
{
	return odbcTypeDescriptor;
}

RETCODE OdbcDesc::sqlSetDescField(int recNumber, int fieldId, SQLPOINTER value, int length)
{
	clearErrors();
	DescRecord *record = NULL;

	if (recNumber)
		record = getDescRecord (recNumber);

	switch (fieldId)
		{
		case SQL_DESC_OCTET_LENGTH_PTR:
			if (record)
				record->octetLengthPtr = (SQLINTEGER*) value;
			break;
			
		case SQL_DESC_INDICATOR_PTR:
			if (record)
				record->indicatorPtr = (SQLINTEGER*) value;
			break;
			
		case SQL_DESC_DATA_PTR:
			if (record)
				{
				record->bufferLength = length;
				record->dataPtr = value;
				}
			break;
			
		/***
		case SQL_DESC_COUNT                  1001
		case SQL_DESC_TYPE                   1002
		case SQL_DESC_LENGTH                 1003
		case SQL_DESC_OCTET_LENGTH_PTR       1004
		case SQL_DESC_PRECISION              1005
		case SQL_DESC_SCALE                  1006
		case SQL_DESC_DATETIME_INTERVAL_CODE 1007
		case SQL_DESC_NULLABLE               1008
		case SQL_DESC_INDICATOR_PTR          1009
		case SQL_DESC_DATA_PTR               1010
		case SQL_DESC_NAME                   1011
		case SQL_DESC_UNNAMED                1012
		case SQL_DESC_OCTET_LENGTH           1013
		case SQL_DESC_ALLOC_TYPE             1099
		***/
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

	DescRecord *record = records [number];

	if (record == NULL)
		records [number] = record = new DescRecord;

	return record;		
}
