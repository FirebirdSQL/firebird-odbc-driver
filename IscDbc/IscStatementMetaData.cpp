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

// IscStatementMetaData.cpp: implementation of the IscStatementMetaData class.
//
//////////////////////////////////////////////////////////////////////

#include "IscDbc.h"
#include "IscStatementMetaData.h"
#include "IscPreparedStatement.h"
#include "Sqlda.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscStatementMetaData::IscStatementMetaData(IscPreparedStatement *preparedStatement)
{
	statement = preparedStatement;
}

IscStatementMetaData::~IscStatementMetaData()
{

}

int IscStatementMetaData::getParameterCount()
{
	return statement->inputSqlda.getColumnCount();
}

int IscStatementMetaData::getParameterType(int index)
{
	return statement->inputSqlda.getColumnType (index);
}

int IscStatementMetaData::getPrecision(int index)
{
	return statement->inputSqlda.getPrecision (index);
}

int IscStatementMetaData::getScale(int index)
{
	return -statement->inputSqlda.getScale (index);
}

bool IscStatementMetaData::isNullable(int index)
{
	return statement->inputSqlda.isNullable (index);
}

int IscStatementMetaData::objectVersion()
{
	return STATEMENTMETADATA_VERSION;
}
