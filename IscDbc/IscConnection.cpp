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
 *	Changes
 *
 *	2003-03-24	IscConnection.cpp
 *				Contributed by Norbert Meyer
 *				o If ++attachment means attachment->addRef() 
 *				  then let's say so.
 *				o In close() set statement->connection to NULL.
 *				  This prevents connection->deleteStatement(this)
 *				  in destructor of IscStatement.
 *
 *
 *	2002-06-08  IscConnection::startTransaction()
 *				Contributed by Carlos Alvarez. New implementation
 *				to better support different transaction options.
 *
 *	2002-05-20	IscConnection.cpp
 *		
 *				Contributed by Robert Milharcic
 *				o better management of statements variable
 *
 */

// IscConnection.cpp: implementation of the IscConnection class.
//
//////////////////////////////////////////////////////////////////////

#include <time.h>
#include <string.h>
#include "IscDbc.h"
#include "EnvShare.h"
#include "IscConnection.h"
#include "IscProceduresResultSet.h"
#include "IscTablePrivilegesResultSet.h"
#include "SQLError.h"
#include "IscOdbcStatement.h"
#include "IscCallableStatement.h"
#include "IscDatabaseMetaData.h"
#include "Parameters.h"
#include "Attachment.h"
#include "Mlist.h"
#include "SupportFunctions.h"
#include "../SetupAttributes.h"

namespace IscDbcLibrary {

extern SupportFunctions supportFn;

extern char charTable [];

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern EnvShare environmentShare;

extern "C" Connection* createConnection()
{
	return new IscConnection;
}

IscConnection::IscConnection()
{
	init();
}

IscConnection::IscConnection(IscConnection * source)
{
	init();
	attachment = source->attachment;
	attachment->addRef();
}

void IscConnection::init()
{
	useCount = 1;
	metaData = NULL;
	transactionHandle = NULL;
	transactionIsolation = 0;
	transactionPending = false;
	autoCommit = true;
	shareConnected = false;
	attachment = NULL;
	transactionExtInit = 0;
	useAppOdbcVersion = 3; // SQL_OV_ODBC3
}

IscConnection::~IscConnection()
{
	if (metaData)
		delete metaData;

	if (attachment)
		attachment->release();
}

bool IscConnection::isConnected()
{
	return attachment != NULL;
}

void IscConnection::close()
{
	FOR_OBJECTS (IscStatement*, statement, &statements)
		statement->close();
		statement->freeStatementHandle();
		statement->connection = NULL; // NOMEY
	END_FOR;

	if ( shareConnected )
		connectionFromEnvShare();

	delete this;
}

PreparedStatement* IscConnection::prepareStatement(const char * sqlString)
{
	IscPreparedStatement *statement = NULL;

	try
	{
		statement = new IscPreparedStatement (this);
		statement->prepare (sqlString);
	}
	catch (...)
	{
		if (statement)
			delete statement;
		throw;
	}

	statements.append (statement);

	return statement;
}

void IscConnection::commit()
{
	if (transactionHandle)
	{
		ISC_STATUS statusVector [20];
		GDS->_commit_transaction (statusVector, &transactionHandle);

		if (statusVector [1])
		{
			rollback();
			throw SQLEXCEPTION (statusVector [1], getIscStatusText (statusVector));
		}
	}
	transactionPending = false;
}

void IscConnection::rollback()
{
	if (transactionHandle)
	{
		ISC_STATUS statusVector [20];
		GDS->_rollback_transaction (statusVector, &transactionHandle);

		if (statusVector [1])
			throw SQLEXCEPTION (statusVector [1], getIscStatusText (statusVector));
	}
	transactionPending = false;
}

void IscConnection::prepareTransaction()
{
}

bool IscConnection::getTransactionPending()
{
	return transactionPending;
}

void* IscConnection::getHandleDb()
{	
	return attachment->databaseHandle;
}

void* IscConnection::startTransaction()
{
	if ( shareConnected )
	{
		if ( !attachment->transactionHandle )
		{
			environmentShare.startTransaction();
			// ASSERT (!autoCommit)
			transactionPending = true;
		}

		return attachment->transactionHandle;
	}

    if (transactionHandle)
        return transactionHandle;

    ISC_STATUS statusVector [20];

    char    iscTpb[5];
	int		count = sizeof( iscTpb );

    iscTpb[0] = isc_tpb_version3;
    iscTpb[1] = transactionExtInit & TRA_ro ? isc_tpb_read : isc_tpb_write;
    iscTpb[2] = transactionExtInit & TRA_nw ? isc_tpb_nowait : isc_tpb_wait;
    /* Isolation level */
    switch( transactionIsolation )
    {
        case 0x00000008L:
            // SQL_TXN_SERIALIZABLE:
            iscTpb[3] = isc_tpb_consistency;
			count = 4;
            break;

        case 0x00000004L:
            // SQL_TXN_REPEATABLE_READ:
            iscTpb[3] = isc_tpb_concurrency;
			count = 4;
            break;

        case 0x00000001L:
            // SQL_TXN_READ_UNCOMMITTED:
			iscTpb[3] = isc_tpb_read_committed;
			iscTpb[4] = isc_tpb_rec_version;
            break;

        case 0x00000002L:
        default:
            // SQL_TXN_READ_COMMITTED:
			iscTpb[3] = isc_tpb_read_committed;
			iscTpb[4] = isc_tpb_no_rec_version;
            break;
    }

    GDS->_start_transaction( statusVector, &transactionHandle, 1, &attachment->databaseHandle,
            count, iscTpb );

    if (statusVector [1])
        throw SQLEXCEPTION (statusVector [1], getIscStatusText (statusVector));

	if (!autoCommit)
		transactionPending = true;

    return transactionHandle;
}

Statement* IscConnection::createStatement()
{
	IscStatement *statement = new IscStatement (this);
	statements.append (statement);

	return statement;
}

InternalStatement* IscConnection::createInternalStatement()
{
	IscOdbcStatement *statement = new IscOdbcStatement (this);
	statements.append (statement);

	return statement;
}

Blob* IscConnection::genHTML(Properties * parameters, long genHeaders)
{
	NOT_YET_IMPLEMENTED;

	return NULL;
}

/***
void IscConnection::freeHTML(const char * html)
{
	delete [] (char*) html;
}
***/

int IscConnection::buildParamProcedure ( char *& string, int numInputParam )
{
	char * ptSrc = string;

	SKIP_WHITE ( ptSrc );

	if ( *ptSrc == '}' )
	{
		if ( numInputParam )
		{
			int i = 0, offset = numInputParam * 2 - 1 + 2;
			memmove(ptSrc + offset, ptSrc, strlen(ptSrc) + 1 );

			*ptSrc++ = '(';
			while( i++ < numInputParam )
			{
				if ( i > 1 )
					*ptSrc++ = ',';
				*ptSrc++ = '?';
			}
			*ptSrc++ = ')';
		}
		return 0;
	}

	if ( *ptSrc != '(' )
		return -1;

	if ( !numInputParam )
	{
		char * ptCh = ptSrc++; // '('

		while ( *ptSrc && *ptSrc != ')' )
			ptSrc++;

		if ( *ptSrc != ')' )
			return -1;

		ptSrc++; // ')'
		memmove(ptCh, ptSrc, strlen(ptSrc) + 1 );
		return 0;
	}

	ptSrc++; // '('

	int i = 0;
	bool nextParam = false;
	char * ptCh;

	while( *ptSrc && i < numInputParam )
	{
		SKIP_WHITE ( ptSrc );

		if ( *ptSrc == ')' )
		{
			int offset = (numInputParam - i) * 2 - ( !i ? 1 : 0 );
			memmove(ptSrc + offset, ptSrc, strlen(ptSrc) + 1 );

			while( i++ < numInputParam )
			{
				if ( i > 1 )
					*ptSrc++ = ',';
				*ptSrc++ = '?';
			}
			return 0;
		}

		if ( *ptSrc == ',' )
		{
			if ( nextParam == true )
			{
				nextParam = false;
				ptSrc++;
			}
			else
			{
				i++;
				memmove(ptSrc + 1, ptSrc, strlen(ptSrc) + 1 );
				*ptSrc = '?';
				ptSrc += 2;
			}
			continue;
		}

		char delimiter;

		ptCh = ptSrc;

		if ( *ptCh == '\'' )
		{
			delimiter = *ptCh;
			++ptCh; // '\''
			ptSrc = ptCh;
			while ( *ptCh && *ptCh != ',' && *ptCh != ')' )
			{
				if ( *ptCh == delimiter )
				{
					if ( *(ptCh+1) == delimiter )
					{
						ptCh += 2;
						continue;
					}
					break;
				}
				++ptCh;
			}

			if ( *ptCh == delimiter )
				++ptCh;

			if ( *ptCh && *ptCh != ',' )
			{
				ptSrc = ptCh;
				i++;
				break;
			}

			if ( !*ptCh )
				break;

			nextParam = true;
		}
		else
		{
			delimiter = ',';
			ptSrc = ptCh;
			while ( *ptCh && *ptCh != delimiter && *ptCh != ')' )
				++ptCh;

			if ( *ptCh && *ptCh != delimiter )
			{
				ptSrc = ptCh;
				i++;
				break;
			}

			if ( !*ptCh )
				break;

			nextParam = false;
		}

		if( ptCh == ptSrc )
			ptSrc++;
		else
		{
			ptSrc = ptCh + 1;
			i++;
		}
	}

	if ( *(ptSrc-1) == ',' )
	{
		ptCh = --ptSrc;
		//  ok, it's output param

		while ( *ptCh && *ptCh != ')' )
			ptCh++;

		memmove(ptSrc, ptCh, strlen(ptCh) + 1 );
		string = ptSrc + 1;

		return 1; 
	}

	SKIP_WHITE(ptSrc);

	if ( *ptSrc == ')' )
	{
		if ( i < numInputParam )
		{
			int offset = (numInputParam - i) * 2;
			memmove(ptSrc + offset, ptSrc, strlen(ptSrc) + 1 );

			while( i++ < numInputParam )
			{
				*ptSrc++ = ',';
				*ptSrc++ = '?';
			}
		}
		return 0;
	}

// error query
	return -1;
}

class CSchemaIdentifier
{
public:
	char	*stringSql;
	bool	deleteNode;
	bool	quotedNode;
	int		begNameNode;
	short	lengthNameNode;

	CSchemaIdentifier()
	{
		remove();
	}
	void remove()
	{ 
		stringSql = NULL;
		deleteNode = false;
		quotedNode = false;
		begNameNode = 0;
		lengthNameNode = 0;
	}
	CSchemaIdentifier & operator =(const CSchemaIdentifier & src)
	{ 
		stringSql = src.stringSql;
		deleteNode = src.deleteNode;
		quotedNode = src.quotedNode;
		begNameNode = src.begNameNode;
		lengthNameNode = src.lengthNameNode;
		return  *this;
	}
};

typedef MList<CSchemaIdentifier> ListSchemaIdentifier;

bool IscConnection::removeSchemaFromSQL( char *strSql, int lenSql, char *strSqlOut, long &lenSqlOut )
{
	ListSchemaIdentifier listSchemaIdentifierAll;
	ListSchemaIdentifier listSchemaIdentifierTbl;
	int countNodesShema = 0;
	int countTblNodesShema = 0;
	int statysModify = 0;
	int statusQuote = 0;
	char *beg = strSql;
	char *ptIn = strSql;
	char *ptInEnd = strSql + lenSql;
	char *ptOut = strSqlOut;
	char quote;
	bool success = true;
	bool defTable = false;

	lenSqlOut = lenSql;

	SKIP_WHITE ( ptIn );

	if ( !IS_MATCH( ptIn, "SELECT" )
		&& !IS_MATCH( ptIn, "UPDATE" )
		&& !IS_MATCH( ptIn, "INSERT" )
		&& !IS_MATCH( ptIn, "DELETE" ) )
		return false;

	while ( ptIn < ptInEnd )
	{
		if ( !statusQuote )
		{
			if ( IS_MATCH( ptIn, "FROM" ) )
			{
				ptIn += 4;
				defTable = true;
			}
			else if ( defTable && IS_MATCH( ptIn, "WHERE" ) )
			{
				ptIn += 5;
				defTable = false;
			}
			else if ( defTable && IS_MATCH( ptIn, "ON" ) )
			{
				ptIn += 2;
				defTable = false;
			}
			else if ( IS_POINT( *ptIn ) && (*ptIn + 1) != '*' )
			{
				do
				{
					bool digit = true;
					char quoteTmp = 0;
					char *pt = ptIn - 1;

					if ( IS_QUOTE( *pt ) )
					{
						quoteTmp = *pt--;
						digit = false;
						while ( pt >= beg && IS_IDENT( *pt ) )
						{
							--pt;
						}
						if ( *pt != quoteTmp )
						{
							success = false;
							break;
						}
					}
					else
					{
						while ( pt >= beg && IS_IDENT( *pt ) )
						{
							if ( digit && !ISDIGIT( *pt ) )
								digit = false;
							--pt;
						}

						++pt;
					}

					if ( !digit )
					{
						bool deleteNode = false;
						char *ptEnd = pt;

						pt = ptIn + 1;

						while ( !(IS_END_TOKEN( *pt )) )
						{
							if ( IS_POINT( *pt ) && !deleteNode )
							{
								deleteNode = true;
							}
							++pt;
						}

						CSchemaIdentifier &node = listSchemaIdentifierAll( countNodesShema++ );

						node.stringSql = strSql;
						node.deleteNode = deleteNode;
						node.quotedNode = !!quoteTmp;
						node.begNameNode = ptEnd - beg;
						node.lengthNameNode = ptIn - ptEnd;

						if ( defTable )
						{
							CSchemaIdentifier &nodeDef = listSchemaIdentifierTbl( countTblNodesShema++ );
							nodeDef = node;
							node.deleteNode = true;
						}

						ptIn = pt;
					}

				} while ( false );
			}
			else if ( IS_QUOTE( *ptIn ) )
			{
				quote = *ptIn;
				statusQuote ^= 1;
			}
		}
		else if ( quote == *ptIn )
		{
			quote = 0;
			statusQuote ^= 1;
		}

		++ptIn;
	}

	if ( countTblNodesShema )
	{
		int offset = 0;
		int offsetNode = 0;
		int length;

		CSchemaIdentifier *node = listSchemaIdentifierAll.GetRoot();

		while( countNodesShema-- )
		{
			bool itsDdelete = false;

			length = node->begNameNode - offsetNode;
			memcpy( &ptOut[offset], &beg[offsetNode], length );
			offset += length;
			offsetNode += length;
			
			if ( node->deleteNode )
				itsDdelete = true;
			else 
			{
				int countTbl = countTblNodesShema;

				CSchemaIdentifier *nodeTbl = listSchemaIdentifierTbl.GetRoot();

				while( countTbl-- )
				{
					if ( node->lengthNameNode == nodeTbl->lengthNameNode 
						&& !strncasecmp( &beg[node->begNameNode],
										 &beg[nodeTbl->begNameNode],
										 node->lengthNameNode ) )
					{
						itsDdelete = true;
						break;
					}

					nodeTbl++;
				}
			}

			if ( itsDdelete )
				offsetNode += node->lengthNameNode + 1;

			node++;
		}

		if ( lenSql > offsetNode )
		{
			length = lenSql - offsetNode;
			memcpy( &ptOut[offset], &beg[offsetNode], lenSql - offsetNode );
			offset += length;
		}

		ptOut[offset] = '\0';
		lenSqlOut = offset;
	}
	else
	{
		lenSqlOut = lenSql;
		memcpy( ptOut, beg, lenSqlOut );
		ptOut[lenSqlOut] = '\0';
	}

	return success;
}

int IscConnection::getNativeSql (const char * inStatementText, long textLength1,
								char * outStatementText, long bufferLength,
								long * textLength2Ptr)
{
	int statysModify = 0;
	char * ptIn = (char*)inStatementText;
	char * ptInEnd = ptIn + textLength1;
	char * ptOut = outStatementText;
	char * ptEndBracket = NULL;
	int ignoreBracket = 0;
	int statusQuote = 0;
	char quote;
	char delimiter = *metaData->getIdentifierQuoteString();
	delimiter = delimiter == ' ' || attachment->databaseDialect < 3 ? 0 : delimiter;

	bool autoRemoveSchemaFromIdentifier = attachment->useSchemaIdentifier == 1;
	bool autoQuoted = delimiter && attachment->autoQuotedIdentifier;

#pragma FB_COMPILER_MESSAGE("IscConnection::getNativeSql - The temporary decision; FIXME!")

	if ( autoRemoveSchemaFromIdentifier )
	{
		statysModify = removeSchemaFromSQL( ptIn, textLength1, ptOut, *textLength2Ptr );

		if ( statysModify )
		{
			ptIn = ptOut;
			textLength1= *textLength2Ptr;
			ptInEnd = ptIn + textLength1;
		}
	}

	while ( ptIn < ptInEnd )
	{
		if ( !statusQuote )
		{
			if ( IS_QUOTE( *ptIn ) )
			{
				quote = *ptIn;
				statusQuote ^= 1;
			}
			else if ( *ptIn == '{' )
				ptEndBracket = ptOut;
			else if ( autoQuoted && IS_IDENT ( *ptIn ) )
			{
				bool mixed = false;
				char * pt = ptIn;
				
				if ( ISUPPER ( *ptIn ) )
				{
					while ( IS_IDENT ( *pt ) )
					{
						if ( ISLOWER ( *pt ) )
						{
							mixed = true;
							break;
						}
						pt++;
					}
				}
				else
				{
					while ( IS_IDENT ( *pt ) )
					{
						if ( ISUPPER ( *pt ) )
						{
							mixed = true;
							break;
						}
						pt++;
					}
				}

				if ( mixed )
				{
					*ptOut++ = delimiter;
					 
					while ( IS_IDENT ( *ptIn ) )
						*ptOut++ = *ptIn++;

					*ptOut++ = delimiter;
					statysModify++;
				}
				else
					while ( IS_IDENT ( *ptIn ) )
						*ptOut++ = *ptIn++;
				continue;
			}
		}
		else if ( quote == *ptIn )
		{
			quote = 0;
			statusQuote ^= 1;
		}

		*ptOut++ = *ptIn++;
	}

	if ( statusQuote ) // There is no '"' or '\'' a syntactic mistake
	{
		if ( textLength2Ptr )
			*textLength2Ptr = textLength1;
		return statysModify;
	}

	*ptOut = '\0';

	if ( !ptEndBracket )
	{
		if ( textLength2Ptr )
			*textLength2Ptr = ptOut - outStatementText;

		ptOut = outStatementText;

		SKIP_WHITE ( ptOut );

		if ( !strncasecmp (ptOut, "COMMIT", 6) && IS_END_TOKEN(*(ptOut + 6)) )
			statysModify = -1;
		else if ( !strncasecmp (ptOut, "ROLLBACK", 8) && IS_END_TOKEN(*(ptOut + 8)) )
			statysModify = -2;
		else if ( !strncasecmp (ptOut, "CREATE", 6) || !strncasecmp (ptOut, "ALTER", 5) )
		{
			bool bContinue = true;

			if ( UPPER(*ptOut) == 'A' )
				ptOut += 5;
			else
			{
				ptOut += 6;

				#define LENSTR_DATABASE 8 
				SKIP_WHITE ( ptOut );

				if ( !strncasecmp (ptOut, "DATABASE", LENSTR_DATABASE) && IS_END_TOKEN(*(ptOut+LENSTR_DATABASE)) )
				{
					statysModify = -3;
					bContinue = false;
				}
			}

			if ( bContinue )
			{
				while ( *ptOut )
				{
					SKIP_WHITE ( ptOut );

					if ( *ptOut )
					{
						#define LENSTR_TINYINT 7 

						if ( !strncasecmp (ptOut, "TINYINT", LENSTR_TINYINT) && IS_END_TOKEN(*(ptOut+LENSTR_TINYINT)) )
						{
							const char * nameTinyint = "CHAR CHARACTER SET OCTETS";
							int lenNameTinyint = 25;
							int offset = lenNameTinyint - LENSTR_TINYINT;

							memmove(ptOut + offset, ptOut, strlen(ptOut) + 1 );
							memcpy(ptOut, nameTinyint, lenNameTinyint);
							ptOut += lenNameTinyint;
						}
						else 
							SKIP_NO_WHITE ( ptOut );
					}
				}

				if ( textLength2Ptr )
					*textLength2Ptr = ptOut - outStatementText;
			}
		}
	}
	else
	{
		while ( ptEndBracket )
		{
			ptIn = ptEndBracket;

			ptIn++; // '{'
			
			while( *ptIn == ' ' )ptIn++;

//	On a note		++ignoreBracket; // ignored { }
			if ( *ptIn == '?' || IS_MATCH( ptIn, "CALL" ) )
			{	
				if ( *ptIn == '?' )
				{
					ptIn++;
					while( *ptIn == ' ' )ptIn++;

					if(*ptIn != '=')
						return statysModify;

					ptIn++; // '='
					while( *ptIn == ' ' )ptIn++;
				}

				if ( !IS_MATCH( ptIn, "CALL" ) )
					return statysModify;

				ptIn += 4; // 'call'

				while( *ptIn == ' ' )ptIn++;

				ptOut = ptEndBracket;
				int ignoreBr = ignoreBracket;
				char * savePtOut;

				const int lenSpase = 18;
				int offset = lenSpase - ( ptIn - ptOut );

				memmove(ptOut + offset, ptOut, strlen(ptOut) + 1 );
				memset(ptOut, ' ', lenSpase);
				savePtOut = ptOut;
				ptIn += offset; 
				ptOut += lenSpase;

				char procedureName[256];
				char * end = procedureName;
				bool repeatWhile;
				char quote = 0;

				do
				{
					repeatWhile = false;

					SKIP_WHITE ( ptIn );

					end = procedureName;
					char *ptTmp = ptIn;

					if ( IS_QUOTE( *ptIn ) )
						quote = *ptIn++;

					if ( autoRemoveSchemaFromIdentifier )
					{
						while ( !(IS_END_TOKEN(*ptIn)) )
						{
							if ( quote && quote == *ptIn )
								break;

							if ( IS_POINT( *ptIn ) )
								break;

							*end++ = *ptIn++;
						}

						if ( quote && quote == *ptIn )
						{
							++ptIn;
							if ( IS_POINT( *ptIn ) )
								quote = 0;
						}

						if ( IS_POINT( *ptIn ) )
						{
							++ptIn;
							memmove( quote ? ptTmp + 1 : ptTmp, ptIn, strlen( ptIn ) + 1 );
							ptIn = ptTmp;
							repeatWhile = true;
						}
					}
					else
					{
						while ( !(IS_END_TOKEN(*ptIn)) )
						{
							if ( quote && quote == *ptIn )
							{
								++ptIn;
								quote = 0;
								break;
							}

							*end++ = *ptIn++;
						}
					}

				} while ( repeatWhile );
				
				*end = '\0';

				int numIn, numOut;
				bool canSelect;

				if ( !getCountInputParamFromProcedure ( procedureName, numIn, numOut, canSelect ) )
					return statysModify; // not found

				int ret = buildParamProcedure ( ptIn, numIn );
				
				if ( ret == -1 ) 
					return statysModify;
				else if ( canSelect )
					memcpy(savePtOut, "select * from ", 14);
				else
					memcpy(savePtOut, "execute procedure ", 18);

				ptOut = ptIn;

				statusQuote = 0;
				quote = 0;

				do
				{
					while( *ptIn )
					{
						if ( !statusQuote )
						{
							if ( IS_QUOTE( *ptIn ) )
							{
								quote = *ptIn;
								statusQuote ^= 1;
							}
							else if ( *ptIn == '}' )
								break;
						}
						else if ( quote == *ptIn )
						{
							quote = 0;
							statusQuote ^= 1;
						}

						*ptOut++ = *ptIn++;
					}

					if( ignoreBr )
						*ptOut++ = *ptIn++;

				}while ( ignoreBr-- );

				if(*ptIn != '}')
					return statysModify;

				ptIn++; // '}'

				while( *ptIn )
					*ptOut++ = *ptIn++;

				*ptOut = '\0';
				statysModify = ret == 1 ? 2 : 1;
			}
			else
			{
				ptOut = ptEndBracket;

				// Check 'oj' or 'OJ'
				if ( *(short*)ptIn == 0x6a6f || *(short*)ptIn == 0x4a4f )
					ptIn += 2; // 'oj'
				else
				{
					ptIn += 2; // 'fn'
//
// select "FIRST_NAME" from "EMPLOYEE" where { fn UCASE("FIRST_NAME") } = { fn UCASE('robert') }
// to
// select "FIRST_NAME" from "EMPLOYEE" where UPPER("FIRST_NAME") = UPPER('robert')
//
// ATTENTION! ptIn and ptOut pointer of outStatementText
					supportFn.translateNativeFunction ( ptIn, ptOut );
				}

				int ignoreBr = ignoreBracket;
				statusQuote = 0;
				quote = 0;

				do
				{
					while( *ptIn )
					{
						if ( !statusQuote )
						{
							if ( IS_QUOTE( *ptIn ) )
							{
								quote = *ptIn;
								statusQuote ^= 1;
							}
							else if ( *ptIn == '}' )
								break;
						}
						else if ( quote == *ptIn )
						{
							quote = 0;
							statusQuote ^= 1;
						}

						*ptOut++ = *ptIn++;
					}

					if( ignoreBr )
						*ptOut++ = *ptIn++;

				}while ( ignoreBr-- );

				if(*ptIn != '}')
					return statysModify;

				ptIn++; // '}'

				while( *ptIn )
					*ptOut++ = *ptIn++;

				*ptOut = '\0';
				statysModify = 1;
			}

			--ptEndBracket; // '{'

			statusQuote = 0;
			quote = 0;

			while ( ptEndBracket > outStatementText )
			{
				if ( !statusQuote )
				{
					if ( IS_QUOTE( *ptEndBracket ) )
					{
						quote = *ptEndBracket;
						statusQuote ^= 1;
					}
					else if ( *ptEndBracket == '{' )
						break;
				}
				else if ( quote == *ptEndBracket )
				{
					quote = 0;
					statusQuote ^= 1;
				}

				--ptEndBracket;
			}

			if(*ptEndBracket != '{')
				ptEndBracket = NULL;
		}

		if ( textLength2Ptr )
			*textLength2Ptr = ptOut - outStatementText;
	}

	return statysModify;
}

bool IscConnection::getCountInputParamFromProcedure ( const char* procedureName, int &numIn, int &numOut, bool &canSelect )
{
	bool ret = false; // not found
	numIn = numOut = 0;
	canSelect = false;

	IscProceduresResultSet resultSet ( (IscDatabaseMetaData *)getMetaData() );
	resultSet.addBlr = true;
	resultSet.allTablesAreSelectable = true;
	resultSet.getProcedures ( NULL, NULL, procedureName );

	if ( resultSet.getCountRowsStaticCursor() )
	{
		numIn = resultSet.sqlda->getShort(4); // NUM_INPUT_PARAM
		numOut = resultSet.sqlda->getShort(5); // NUM_OUTPUT_PARAM
		if ( numOut )
			canSelect = resultSet.canSelectFromProcedure();
		ret = true;
	}

	return ret;
}

DatabaseMetaData* IscConnection::getMetaData()
{
	if (metaData)
		return metaData;

	metaData = new IscDatabaseMetaData (this);

	return metaData;
}

int IscConnection::hasRole(const char * schemaName, const char * roleName)
{
	NOT_YET_IMPLEMENTED;

	return false;
}

void IscConnection::ping()
{
}

void IscConnection::sqlExecuteCreateDatabase(const char * sqlString)
{
	isc_db_handle   newdb = NULL;
	isc_tr_handle   trans = NULL;
	ISC_STATUS      statusVector[20];

	if ( GDS->_dsql_execute_immediate( statusVector, &newdb, &trans, 0, (char*)sqlString, 3, NULL ) )
    {
		if ( statusVector[1] )
			throw SQLEXCEPTION ( statusVector [1], getIscStatusText( statusVector ) );
	}
	
	GDS->_commit_transaction( statusVector, &trans );
	GDS->_detach_database( statusVector, &newdb );
}

void IscConnection::createDatabase(const char * dbName, Properties * properties)
{
	try
	{
		attachment = new Attachment;
		attachment->createDatabase( dbName, properties );
		databaseHandle = attachment->databaseHandle;
		GDS = attachment->GDS;
	}
	catch ( SQLException& exception )
	{
		delete attachment;
		attachment = NULL;
		GDS = NULL;
		throw SQLEXCEPTION ( exception.getSqlcode() , exception.getText() );
	}
	catch (...)
	{
		delete attachment;
		attachment = NULL;
		GDS = NULL;
		throw;
	}
}

void IscConnection::openDatabase(const char * dbName, Properties * properties)
{
	try
	{
		attachment = new Attachment;
		attachment->openDatabase (dbName, properties);
		databaseHandle = attachment->databaseHandle;
		GDS = attachment->GDS;

		if ( !attachment->isRoles && !attachment->admin )
		{
			IscTablePrivilegesResultSet resultSet ( (IscDatabaseMetaData *)getMetaData() );
			resultSet.allTablesAreSelectable = true;
			resultSet.getTablePrivileges( NULL, NULL, "RDB$ROLES" );

			if ( resultSet.getCountRowsStaticCursor() )
			{
				int len1 = strlen( attachment->userName );
				int len2;
				char *beg = resultSet.sqlda->getVarying( 5, len2 );
				char *end = beg + len2;
				char *save = end;

				while ( end > beg && *(--end) == ' ');

				if ( save != end )
				{
					len2 = end - beg + 1;
					*(end+1) = '\0';
				}

				if( len1 == len2 && !strncmp( attachment->userName, beg, len1 ) )
					attachment->admin = true;
			}
		}
	}
	catch ( SQLException& exception )
	{
		delete attachment;
		attachment = NULL;
		GDS = NULL;
		throw SQLEXCEPTION ( exception.getSqlcode() , exception.getText() );
	}
	catch (...)
	{
		delete attachment;
		attachment = NULL;
		GDS = NULL;
		throw;
	}
}


void IscConnection::deleteStatement(IscStatement * statement)
{
//From R. Milharcic
 	statements.deleteItem (statement);
}


JString IscConnection::getIscStatusText(ISC_STATUS * statusVector)
{
	return attachment->getIscStatusText(statusVector);
}


int IscConnection::getInfoItem(char * buffer, int infoItem, int defaultValue)
{
	for (char *p = buffer; *p != isc_info_end;)
		{
		char item = *p++;
		int length = GDS->_vax_integer (p, 2);
		p += 2;
		if (item == infoItem)
			return GDS->_vax_integer (p, length);
		p += length;
		}

	return defaultValue;			
}

JString IscConnection::getInfoString(char * buffer, int infoItem, const char * defaultString)
{
	for (char *p = buffer; *p != isc_info_end;)
		{
		char item = *p++;
		int length = GDS->_vax_integer (p, 2);
		p += 2;
		if (item == infoItem)
			return JString (p, length);
		p += length;
		}

	return defaultString;			
}

Properties* IscConnection::allocProperties()
{
	return new Parameters;
}

int IscConnection::objectVersion()
{
	return CONNECTION_VERSION;
}

void IscConnection::clearWarnings()
{
	NOT_YET_IMPLEMENTED;
}

Connection* IscConnection::clone()
{
	return new IscConnection (this);
}

void IscConnection::setAutoCommit(bool setting)
{
	if( !autoCommit && setting && transactionPending )
		commitAuto();

	autoCommit = setting;
}

bool IscConnection::getAutoCommit()
{
	return autoCommit;
}

const char*	IscConnection::getCatalog()
{
	NOT_YET_IMPLEMENTED;
	return "";
}

void IscConnection::setCatalog(const char* catalog)
{
	NOT_YET_IMPLEMENTED;
}

void IscConnection::setTransactionIsolation(int level)
{
	transactionIsolation = level;
}

int IscConnection::getTransactionIsolation()
{
	return transactionIsolation;
}

bool IscConnection::isClosed()
{
	NOT_YET_IMPLEMENTED;
	return false;
}

bool IscConnection::isReadOnly()
{
	NOT_YET_IMPLEMENTED;
	return false;
}

void IscConnection::setReadOnly(bool readOnly)
{
	NOT_YET_IMPLEMENTED;
}

const char*	IscConnection::nativeSQL(const char *sqlString)
{
	NOT_YET_IMPLEMENTED;
	return sqlString;
}

void IscConnection::setExtInitTransaction(int optTpb)
{
	transactionExtInit = optTpb;
}

EnvironmentShare* IscConnection::getEnvironmentShare()
{
	return (EnvironmentShare*)&environmentShare;
}

void IscConnection::connectionToEnvShare()
{
	shareConnected = environmentShare.addConnection (this);
}

void IscConnection::connectionFromEnvShare()
{
	environmentShare.removeConnection (this);
	shareConnected = false;
}

int	IscConnection::getDriverBuildKey()
{
	return MAJOR_VERSION * 1000000 + MINOR_VERSION * 10000 + BUILDNUM_VERSION;
}

void IscConnection::addRef()
{
	++useCount;
}

int IscConnection::release()
{
	if (--useCount == 0)
		{
		close();
		return 0;
		}

	return useCount;
}

CallableStatement* IscConnection::prepareCall(const char * sqlString)
{
	IscCallableStatement *statement = NULL;

	try
		{
		statement = new IscCallableStatement (this);
		statement->prepare (sqlString);
		}
	catch (...)
		{
		if (statement)
			delete statement;
		throw;
		}

	statements.append (statement);
	return statement;
}

void IscConnection::commitAuto()
{
	FOR_OBJECTS (IscStatement*, statement, &statements)
		if ( statement->isActiveCursor() )
		{
			commitRetaining();
			return;
		}
	END_FOR;

	commit();
}

void IscConnection::rollbackAuto()
{
	FOR_OBJECTS (IscStatement*, statement, &statements)
		if ( statement->isActiveCursor() )
		{
			rollbackRetaining();
			return;
		}
	END_FOR;

	rollback();
}

int IscConnection::getDatabaseDialect()
{
	return attachment->getDatabaseDialect();
}

void IscConnection::commitRetaining()
{
	if (transactionHandle)
	{
		ISC_STATUS statusVector [20];
		GDS->_commit_retaining (statusVector, &transactionHandle);

		if (statusVector [1])
		{
			rollbackRetaining();
			throw SQLEXCEPTION (statusVector [1], getIscStatusText (statusVector));
		}
	}
	transactionPending = false;
}

void IscConnection::rollbackRetaining()
{
	if (transactionHandle)
	{
		ISC_STATUS statusVector [20];
		GDS->_rollback_retaining (statusVector, &transactionHandle);

		if (statusVector [1])
		{
			rollback();
			throw SQLEXCEPTION (statusVector [1], getIscStatusText (statusVector));
		}
	}
	transactionPending = false;
}

}; // end namespace IscDbcLibrary
