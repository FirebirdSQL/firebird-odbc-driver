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
 *  The Original Code was created by Vladimir Tsvigun for IBPhoenix.
 *
 *  Copyright (c) 2003 Vladimir Tsvigun
 *  All Rights Reserved.
 */

//  
// IscOdbcStatement.h: interface for the IscArray class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_IscOdbcStatement_H_)
#define _IscOdbcStatement_H_

#include "Connection.h"
#include "IscStatement.h"

namespace IscDbcLibrary {

class IscConnection;
class IscStatementMetaData;
class IscOdbcStatement;

class IscOdbcStatement : public IscStatement, public InternalStatement
{
public:
	virtual int objectVersion();
	virtual StatementMetaData* getStatementMetaDataIPD();
	virtual StatementMetaData* getStatementMetaDataIRD();

	IscOdbcStatement(IscConnection *connect);
	~IscOdbcStatement();

	virtual bool		execute (const char *sqlString);
	virtual ResultSet*	executeQuery (const char *sqlString);
	virtual void		clearResults();
	virtual int			getUpdateCount();
	virtual bool		getMoreResults();
	virtual void		setCursorName (const char *name);
	virtual ResultSet*	getResultSet();
	virtual ResultList* search (const char *searchString);
	virtual int			executeUpdate (const char *sqlString);
	virtual void		close();
	virtual void		drop();
	virtual int			release();
	virtual void		addRef();
	virtual bool		isActiveSelect(){ return IscStatement::isActiveSelect(); }
	virtual bool		isActiveProcedure(){ return IscStatement::isActiveProcedure(); }

	virtual int			executeUpdate();
	virtual bool		executeStatement();
	virtual bool		executeProcedure();
	virtual ResultSet*	executeQuery();
	virtual void		executeMetaDataQuery();

	void				getInputParameters();
	int					getNumParams();
	void				prepareStatement(const char * sqlString);
	bool				isActive() { return !!statementHandle; }

	IscStatementMetaData	*statementMetaDataIPD;
	IscStatementMetaData	*statementMetaDataIRD;

	virtual int			getStmtPlan(const void * value, int bufferLength,long *lengthPtr)
	{ return getPlanStatement(connection, statementHandle,value,bufferLength,lengthPtr); }  
	virtual int			getStmtType(const void * value, int bufferLength,long *lengthPtr)
	{ return getTypeStatement(connection, statementHandle,value,bufferLength,lengthPtr); }  
	virtual int			getStmtInfoCountRecords(const void * value, int bufferLength,long *lengthPtr)
	{ return getInfoCountRecordsStatement(connection, statementHandle, value,bufferLength,lengthPtr); }  
};

}; // end namespace IscDbcLibrary

#endif // !defined(_IscOdbcStatement_H_)
