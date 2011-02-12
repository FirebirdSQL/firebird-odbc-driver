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
	enum TypeStatement {	stmtNone		= 0, 
							stmtDDL			= 1, 
							stmtSelect		= 2,
							stmtSelectForUpdate = 4,
							stmtInsert		= 8, 
							stmtUpdate		= 16, 
							stmtDelete		= 32, 
							stmtProcedure	= 64,
							stmtModify		= 128,
							stmtPrepare		= 256
						};

	void freeStatementHandle();
	void clearSelect();
	void rollbackLocal();
	void commitLocal();
	void setReadOnlyTransaction();
	isc_tr_handle startTransaction();
	static ISC_TIME getIscTime (SqlTime value);
	static ISC_TIMESTAMP getIscTimeStamp (TimeStamp value);
	static ISC_DATE getIscDate (DateTime date);
	void setValue(Value *value, XSQLVAR *var);
	int getUpdateCounts();
	virtual int objectVersion();
	void clearResults();
	virtual bool execute();
	virtual bool executeProcedure();
	virtual void prepareStatement (const char *sqlString);
	void deleteResultSet (IscResultSet *resultSet);
	IscStatement(IscConnection *connect);
	virtual int getUpdateCount();
	virtual bool getMoreResults();
	virtual void setCursorName (const char *name);
	virtual void setEscapeProcessing(bool enable);
	virtual ResultSet* executeQuery (const char *sqlString);
	virtual ResultSet* getResultSet();
	virtual ResultList* search (const char *searchString);
	virtual int executeUpdate (const char *sqlString);
	virtual int	getMaxFieldSize();
	virtual int	getMaxRows();
	virtual int	getQueryTimeout();
	virtual void cancel();
	virtual bool execute (const char *sqlString);
	virtual void close();
	virtual void setMaxFieldSize(int max);
	virtual void setMaxRows(int max);
	virtual void setQueryTimeout(int seconds);

	virtual int release();
	virtual void addRef();
	virtual bool isActiveDDL(){ return typeStmt == stmtDDL; }
	virtual bool isActiveSelect(){ return typeStmt == stmtSelect || typeStmt == stmtSelectForUpdate; }
	virtual bool isActiveSelectForUpdate(){ return typeStmt == stmtSelectForUpdate; }
	virtual bool isActiveCursor(){ return isActiveSelect() && openCursor; }
	virtual bool isActiveProcedure(){ return typeStmt == stmtProcedure; }
	virtual bool isActiveModify(){ return !!(typeStmt & stmtModify); }
	virtual bool isActiveNone(){ return typeStmt == stmtNone; }
	virtual ~IscStatement();

	virtual int getStmtPlan(const void * value, int bufferLength,int *lengthPtr)
	{ return getPlanStatement(connection, statementHandle,value,bufferLength,lengthPtr); }  
	virtual int			getStmtType(const void * value, int bufferLength,int *lengthPtr)
	{ return getTypeStatement(connection, statementHandle,value,bufferLength,lengthPtr); }  
	virtual int			getStmtInfoCountRecords(const void * value, int bufferLength,int *lengthPtr)
	{ return getInfoCountRecordsStatement(connection, statementHandle, value,bufferLength,lengthPtr); }  
	virtual bool		isActiveLocalTransaction(){ return transactionLocal; }
	virtual void		setActiveLocalParamTransaction();
	virtual void		delActiveLocalParamTransaction();
	virtual void		declareLocalParamTransaction();
	virtual void		switchTransaction( bool local );

	IscResultSet*	createResultSet();
	LinkedList		resultSets;
	IscConnection	*connection;
	JString			sql;
	int				useCount;
	int				numberColumns;
	int				resultsCount;
	int				resultsSequence;
	isc_stmt_handle	statementHandle;
	InfoTransaction	transactionInfo;
	bool			transactionLocal;
	bool			transactionStatusChange;
	bool			transactionStatusChangingToLocal;

	Sqlda			inputSqlda;
	Sqlda			outputSqlda;
	int				summaryUpdateCount;
	int				typeStmt;
	bool			openCursor;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_ISCSTATEMENT_H_)
