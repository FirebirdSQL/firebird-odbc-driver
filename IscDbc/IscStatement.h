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
 */

// IscStatement.h: interface for the IscStatement class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ISCSTATEMENT_H_)
#define _ISCSTATEMENT_H_

#include "Connection.h"
#include "LinkedList.h"
#include "Sqlda.h"

namespace IscDbcLibrary {

class IscConnection;
class IscResultSet;

class IscStatement : public Statement  
{
public:
	enum TypeStatement { stmtNone, stmtSelect, stmtProcedure };

	void freeStatementHandle();
	void clearSelect();
	//void rollbackAuto();
	//void* getTransaction();
	//void commitAuto();
	static ISC_TIME getIscTime (SqlTime value);
	static ISC_TIMESTAMP getIscTimeStamp (TimeStamp value);
	static ISC_DATE getIscDate (DateTime date);
	void setValue(Value *value, XSQLVAR *var);
	int getUpdateCounts();
	virtual int objectVersion();
	void clearResults();
	virtual bool execute();
	void prepareStatement (const char *sqlString);
	void deleteResultSet (IscResultSet *resultSet);
	IscStatement(IscConnection *connect);
	virtual int getUpdateCount();
	virtual bool getMoreResults();
	virtual void setCursorName (const char *name);
	virtual ResultSet* executeQuery (const char *sqlString);
	virtual ResultSet* getResultSet();
	virtual ResultList* search (const char *searchString);
	virtual int executeUpdate (const char *sqlString);
	virtual bool execute (const char *sqlString);
	virtual void close();
	virtual int release();
	virtual void addRef();
	virtual bool isActiveSelect(){ return typeStmt == stmtSelect; }
	virtual bool isActiveProcedure(){ return typeStmt == stmtProcedure; }
	~IscStatement();

	virtual int getStmtPlan(const void * value, int bufferLength,long *lengthPtr)
	{ return getPlanStatement(connection, statementHandle,value,bufferLength,lengthPtr); }  
	virtual int			getStmtType(const void * value, int bufferLength,long *lengthPtr)
	{ return getTypeStatement(connection, statementHandle,value,bufferLength,lengthPtr); }  
	virtual int			getStmtInfoCountRecords(const void * value, int bufferLength,long *lengthPtr)
	{ return getInfoCountRecordsStatement(connection, statementHandle, value,bufferLength,lengthPtr); }  

	IscResultSet*	createResultSet();
	LinkedList		resultSets;
	IscConnection	*connection;
	JString			sql;
	int				useCount;
	int				numberColumns;
	int				resultsCount;
	int				resultsSequence;
	isc_stmt_handle	statementHandle;
	//void*			transactionHandle;
	Sqlda			inputSqlda;
	Sqlda			outputSqlda;
	int				summaryUpdateCount;
	TypeStatement	typeStmt;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_ISCSTATEMENT_H_)
