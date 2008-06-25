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
 *  The Original Code was created by Vladimir Tsvigun for IBPhoenix.
 *
 *  Copyright (c) 2003 Vladimir Tsvigun
 *  All Rights Reserved.
 */

//  
// IscOdbcStatement.cpp: interface for the IscOdbcStatement class.
//
//////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include "IscDbc.h"
#include "IscOdbcStatement.h"
#include "SQLError.h"
#include "IscResultSet.h"
#include "IscConnection.h"
#include "BinaryBlob.h"
#include "Value.h"
#include "IscStatementMetaData.h"

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscOdbcStatement::IscOdbcStatement(IscConnection *connection) : IscStatement (connection)
{
	statementMetaDataIPD = NULL;
	statementMetaDataIRD = NULL;
}

IscOdbcStatement::~IscOdbcStatement()
{
	delete statementMetaDataIPD;
	delete statementMetaDataIRD;
}

ResultSet* IscOdbcStatement::executeQuery()
{
	if (outputSqlda.sqlda->sqld < 1)
		throw SQLEXCEPTION (RUNTIME_ERROR, "statement is not a Select");

	IscStatement::execute();
	getMoreResults();

	return getResultSet();
}

void IscOdbcStatement::executeMetaDataQuery()
{
	if (outputSqlda.sqlda->sqld < 1)
		throw SQLEXCEPTION (RUNTIME_ERROR, "statement is not a Select");

	IscStatement::execute();
	getMoreResults();
}

void IscOdbcStatement::drop()
{
	IscStatement::close ();
	resultsCount = 0;
	resultsSequence	= 0;
	freeStatementHandle();
	inputSqlda.clearSqlda();
	outputSqlda.clearSqlda();
	numberColumns = 0;
}

void IscOdbcStatement::prepareStatement(const char * sqlString)
{
	IscStatement::prepareStatement (sqlString);
	getInputParameters();

	char * tempSql = NULL;
	int * labelParamArray = NULL;
	int replaceParamArray;

	if ( (replaceParamArray = replacementArrayParamForStmtUpdate( tempSql, labelParamArray )) )
	{
		freeStatementHandle();
		IscStatement::prepareStatement ( (const char*)tempSql );
		getInputParameters();
	}

	inputSqlda.allocBuffer ( this );

	if ( replaceParamArray )
	{
		int * label = labelParamArray;
		while ( replaceParamArray-- )
		{
			CAttrSqlVar *var = inputSqlda.orgVar ( *label++ );
			var->replaceForParamArray = true;
		}

		free ( tempSql );
		free ( labelParamArray );
	}
}

void IscOdbcStatement::getInputParameters()
{
	ISC_STATUS statusVector [20];

	int dialect = connection->getDatabaseDialect ();
	connection->GDS->_dsql_describe_bind (statusVector, &statementHandle, dialect, inputSqlda);

	if (statusVector [1])
		THROW_ISC_EXCEPTION (connection, statusVector);

	if (inputSqlda.checkOverflow())
	{
		connection->GDS->_dsql_describe_bind (statusVector, &statementHandle, dialect, inputSqlda);
		if (statusVector [1])
			THROW_ISC_EXCEPTION (connection, statusVector);
	}
}

int IscOdbcStatement::getNumParams()
{
	if ( isActiveProcedure() )
		return inputSqlda.getColumnCount() + outputSqlda.getColumnCount();

	return inputSqlda.getColumnCount();
}

StatementMetaData* IscOdbcStatement::getStatementMetaDataIPD()
{
	if (statementMetaDataIPD)
		return statementMetaDataIPD;

	statementMetaDataIPD = new IscStatementMetaData (this, &inputSqlda);

	return statementMetaDataIPD;
}

StatementMetaData* IscOdbcStatement::getStatementMetaDataIRD()
{
	if (statementMetaDataIRD)
		return statementMetaDataIRD;

	statementMetaDataIRD = new IscStatementMetaData (this, &outputSqlda);

	return statementMetaDataIRD;
}

//  
//  UPDATE "TESTTBL" SET "KOD"=?,"ARRAYFLD1"=?,"ARRAYFLD2"=? WHERE "KOD" = ? AND "ARRAYFLD1" = ? AND "ARRAYFLD2" = ?
//  "ARRAYFLD1","ARRAYFLD2" - it's array
//	Our purpose:
//		set attributes(relname,sqlname) of param 2 to param 5
//			and param 3 to param 6
// 
int IscOdbcStatement::replacementArrayParamForStmtUpdate( char *& tempSql, int *& labelParamArray )
{
	const char *strSql = sql, *ch;
	int numberColumns = inputSqlda.sqlda->sqld;
	XSQLVAR *var = inputSqlda.sqlda->sqlvar;
	int *offsetParam = NULL;
	int *offsetNameParam = NULL;
	int countDefined = 0;

	for (int n = 0; n < numberColumns; ++n, ++var)
	{
		switch ( var->sqltype & ~1 )
		{
		case SQL_ARRAY:
			if ( !var->sqlname_length )
			{
				if ( !offsetParam )
				{
					offsetParam = new int[numberColumns];
					offsetNameParam = new int[numberColumns];
			        memset ( offsetNameParam, 0, sizeof(int) * numberColumns );

					int *param = offsetParam;

					ch = strSql;

					while ( *ch )
					{
						if ( *ch == '?' )
							*param++ = ch - strSql;
						ch++;
					}
				}

				const char *end = strSql + offsetParam[n];

				while ( end > strSql && *end != '=' ) --end;
				--end; // '='
				while ( end > strSql && *end == ' ' ) --end;

				char delimiter = '"';

				if ( *end != '"' )
				{
					delimiter = ' ';
					end++;
				}

				const char *start = end;

				while ( start-- > strSql )
					if ( *start == delimiter )
					{
						start++;
						break;
					}

				XSQLVAR *varIn = inputSqlda.sqlda->sqlvar;
				int len = end - start;

				for ( int m = 0; m < n; ++m, ++varIn )
				{
					if ( varIn->sqlname_length == len && !strncasecmp ( varIn->sqlname, start, len ) )
					{
						memcpy ( var->sqlname, varIn->sqlname, len );
						var->sqlname_length = len;
						memcpy ( var->relname, varIn->relname, varIn->relname_length );
						var->relname_length = varIn->relname_length;
						offsetNameParam[n] = end - strSql;

						if ( delimiter == '"' )
							offsetNameParam[n]++;

						countDefined++;
						break;
					}
				}
				
			}
			break;
		}
	}

	if ( countDefined )
	{
		int lenOldSql = (int)strlen(strSql);
		tempSql = (char *)malloc ( countDefined * 3 + lenOldSql + 1);
		labelParamArray = (int *)malloc ( countDefined * sizeof(int) );
		int n, offset = 0;
		int * label = labelParamArray;

		ch = strSql;
	
		for ( n = 0; n < numberColumns; ++n, ++var)
		{
			int &offsetEndName = offsetNameParam[n];

			if ( offsetEndName )
			{
				memcpy ( &tempSql[offset], ch, offsetEndName - offset );
				ch += offsetEndName;
				offset += offsetEndName;
				memcpy ( &tempSql[offset], "[1]", 3 );
				offset += 3;
				*label++ = n + 1;
			}
		}
		
		n = lenOldSql - ( ch - strSql );

		if ( n != 0 )
		{
			memcpy ( &tempSql[offset], ch, n );
			offset += n;
		}

		tempSql[offset] = '\0';
		delete [] offsetParam;
		delete [] offsetNameParam;
		return countDefined;
	}
	return 0;
}

int IscOdbcStatement::objectVersion()
{
	return INTERNALSTATEMENT_VERSION;
}

}; // end namespace IscDbcLibrary
