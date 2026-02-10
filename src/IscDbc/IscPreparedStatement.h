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
 *
 *	2002-06-04	See comment in IscPreparedStatement.cpp
 *
 *
 */

// IscPreparedStatement.h: interface for the IscPreparedStatement class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ISCPREPAREDSTATEMENT_H_)
#define _ISCPREPAREDSTATEMENT_H_

#include "Connection.h"
#include "IscStatement.h"
#include "Values.h"

namespace IscDbcLibrary {

class IscConnection;
class IscStatementMetaData;
class BinaryBlob;
class AsciiBlob;

class IscPreparedStatement : public IscStatement, public PreparedStatement
{
public:
// {{{specification jdbc
	virtual void		clearParameters();
	virtual bool		execute();
	virtual ResultSet*	executeQuery();
	virtual int			executeUpdate();
//	virtual void		setAsciiStream( int parameterIndex, InputStream x, int length );
//	virtual void		setBigDecimal( int parameterIndex, BigDecimal x );
//	virtual void		setBinaryStream( int parameterIndex, InputStream x, int length );
	virtual void		setBoolean( int parameterIndex, bool x );
	virtual void		setByte (int index, char value);
	virtual void		setBytes (int index, const void *bytes);
	virtual void		setDate (int index, DateTime value);
	virtual void		setDouble (int index, double value);
	virtual void		setFloat (int index, float value);
	virtual void		setInt (int index, int value);
	virtual void		setLong (int index, QUAD value);
	virtual void		setNull (int index, int type);
//	virtual void		setObject( int parameterIndex, Object x );
//	virtual void		setObject( int parameterIndex, Object x, int targetSqlType );
//	virtual void		setObject( int parameterIndex, Object x, int targetSqlType, int scale );
	virtual void		setShort (int index, short value);
	virtual void		setString(int index, const char * string);
	virtual void		setTime (int index, SqlTime value);
	virtual void		setTimestamp (int index, TimeStamp value);
//	virtual void		setUnicodeStream( int parameterIndex, InputStream x, int length );
// }}}end specification jdbc

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
	virtual bool		isActiveSelect(){ return IscStatement::isActiveSelect(); }
	virtual bool		isActiveSelectForUpdate(){ return IscStatement::isActiveSelectForUpdate(); }
	virtual bool		isActiveProcedure(){ return IscStatement::isActiveProcedure(); }
	virtual bool		isActiveModify(){ return IscStatement::isActiveModify(); }
	virtual bool		isActiveNone(){ return IscStatement::isActiveNone(); }
	virtual int			getStmtPlan(const void * value, int bufferLength,int *lengthPtr) { return IscStatement::getStmtPlan( value, bufferLength, lengthPtr ); }  
	virtual int			getStmtType(const void * value, int bufferLength,int *lengthPtr) { return IscStatement::getStmtType( value, bufferLength, lengthPtr ); }  
	virtual int			getStmtInfoCountRecords(const void * value, int bufferLength,int *lengthPtr) { return IscStatement::getStmtInfoCountRecords( value, bufferLength, lengthPtr ); }  
	virtual bool		isActiveLocalTransaction(){ return IscStatement::isActiveLocalTransaction(); }
	virtual void		setActiveLocalParamTransaction(){ IscStatement::setActiveLocalParamTransaction(); }
	virtual void		delActiveLocalParamTransaction(){ IscStatement::delActiveLocalParamTransaction(); }
	virtual void		declareLocalParamTransaction(){ IscStatement::declareLocalParamTransaction(); }
	virtual void		switchTransaction(bool local){ IscStatement::switchTransaction( local ); }

//}}} end class Statement without specification jdbc

public:
	IscPreparedStatement(IscConnection *connect);
	virtual ~IscPreparedStatement();

	virtual StatementMetaData* getStatementMetaDataIPD();
	virtual StatementMetaData* getStatementMetaDataIRD();

    virtual void        setString(int index, const char * string, int length);
	virtual void        convStringData(int index);
	virtual void		setBlob (int index, Blob *value);
	virtual void		setBytes (int index, int length, const void *bytes);
	virtual void		setArray (int index, Blob *value);
    virtual void        beginBlobDataTransfer(int index);
    virtual void        putBlobSegmentData (int length, const void *bytes);
    virtual void        endBlobDataTransfer();	

	virtual void		executeMetaDataQuery();

	void				getInputParameters();
	virtual int			getNumParams();
	virtual void		prepare (const char *sqlString);

	Value* getParameter (int index);

	Values				parameters;
	IscStatementMetaData	
						*statementMetaDataIPD;
	IscStatementMetaData
						*statementMetaDataIRD;
    BinaryBlob          *segmentBlob;
	AsciiBlob			*segmentClob;

	virtual int			objectVersion();
};

}; // end namespace IscDbcLibrary

#endif // !defined(_ISCPREPAREDSTATEMENT_H_)
