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
//{{{ class InternalStatement specification jdbc
	virtual bool		isActive() { return !!statementHandle; }
	virtual void		prepareStatement(const char * sqlString);
	virtual bool		executeStatement() { return IscStatement::execute(); }
	virtual bool		executeProcedure(){ return IscStatement::executeProcedure(); }
	virtual StatementMetaData*	
						getStatementMetaDataIPD();
	virtual StatementMetaData*	
						getStatementMetaDataIRD();
	virtual int			getNumParams();
	virtual void		drop();
	virtual int			objectVersion();
//}}} end class InternalStatement specification jdbc

//{{{ class Statement specification jdbc
	virtual bool		execute (const char *sqlString) { return IscStatement::execute (sqlString); }
	virtual ResultSet*	executeQuery (const char *sqlString) { return IscStatement::executeQuery ( sqlString ); }
	virtual int			getUpdateCount() { return IscStatement::getUpdateCount (); }
	virtual bool		getMoreResults(){ return IscStatement::getMoreResults(); }
	virtual void		setCursorName (const char *name){ IscStatement::setCursorName ( name ); }
	virtual void		setEscapeProcessing(bool enable){ IscStatement::setEscapeProcessing( enable ); }
	virtual ResultSet*	getResultSet(){ return IscStatement::getResultSet(); }
	virtual int			executeUpdate (const char *sqlString){ return IscStatement::executeUpdate ( sqlString ); }
	virtual int			getMaxFieldSize(){ return IscStatement::getMaxFieldSize(); }
	virtual int			getMaxRows(){ return IscStatement::getMaxRows(); }
	virtual int			getQueryTimeout(){ return IscStatement::getQueryTimeout(); }
	virtual void		cancel(){ IscStatement::cancel(); }
//	virtual void		clearWarnings(){ IscStatement::clearWarnings(); }
//	virtual void		getWarnings(){ IscStatement::getWarnings(); }
	virtual void		close(){ IscStatement::close(); }
	virtual void		setMaxFieldSize(int max){ IscStatement::setMaxFieldSize( max ); }
	virtual void		setMaxRows(int max){ IscStatement::setMaxRows( max ); }
	virtual void		setQueryTimeout(int seconds){ IscStatement::setQueryTimeout( seconds ); }
//}}} end class Statement specification jdbc

	virtual void		clearResults() { IscStatement::clearResults (); }
	virtual ResultList* search (const char *searchString) { return IscStatement::search (searchString); }
	virtual int			release() { return IscStatement::release (); }
	virtual void		addRef() { IscStatement::addRef (); }
	virtual bool		isActiveDDL(){ return IscStatement::isActiveDDL(); }
	virtual bool		isActiveSelect(){ return IscStatement::isActiveSelect(); }
	virtual bool		isActiveSelectForUpdate(){ return IscStatement::isActiveSelectForUpdate(); }
	virtual bool		isActiveProcedure(){ return IscStatement::isActiveProcedure(); }
	virtual bool		isActiveModify(){ return IscStatement::isActiveModify(); }
	virtual bool		isActiveNone(){ return IscStatement::isActiveNone(); }
	virtual int			getStmtPlan(const void * value, int bufferLength, int *lengthPtr) { return IscStatement::getStmtPlan( value, bufferLength, lengthPtr ); }  
	virtual int			getStmtType(const void * value, int bufferLength, int *lengthPtr) { return IscStatement::getStmtType( value, bufferLength, lengthPtr ); }  
	virtual int			getStmtInfoCountRecords(const void * value, int bufferLength,int *lengthPtr) { return IscStatement::getStmtInfoCountRecords( value, bufferLength, lengthPtr ); }  
	virtual void		rollbackLocal(){ IscStatement::rollbackLocal(); }
	virtual void		commitLocal(){ IscStatement::commitLocal(); }
	virtual bool		isActiveLocalTransaction(){ return IscStatement::isActiveLocalTransaction(); }
	virtual void		setActiveLocalParamTransaction(){ IscStatement::setActiveLocalParamTransaction(); }
	virtual void		delActiveLocalParamTransaction(){ IscStatement::delActiveLocalParamTransaction(); }
	virtual void		declareLocalParamTransaction(){ IscStatement::declareLocalParamTransaction(); }
	virtual void		switchTransaction(bool local){ IscStatement::switchTransaction( local ); }

//}}} end class Statement without specification jdbc

protected:
	int					replacementArrayParamForStmtUpdate( char *& tempSql, int *& labelParamArray );

public:
	IscOdbcStatement(IscConnection *connect);
	virtual ~IscOdbcStatement();

	virtual Statement*	getStatement() { return (IscStatement*)this; }
	virtual ResultSet*	executeQuery();
	virtual void		executeMetaDataQuery();
	void				getInputParameters();

	IscStatementMetaData	*statementMetaDataIPD;
	IscStatementMetaData	*statementMetaDataIRD;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_IscOdbcStatement_H_)
