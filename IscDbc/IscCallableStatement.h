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
 *	2002-06-04 See comments in IscCallableStatement.cpp for details
 *
 */


// IscCallableStatement.h: interface for the IscCallableStatement class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ISCCALLABLESTATEMENT_H_)
#define _ISCCALLABLESTATEMENT_H_

#include "Connection.h"
#include "IscPreparedStatement.h"
#include "Values.h"
#include "DateTime.h"	// Added by ClassView
#include "SqlTime.h"
#include "TimeStamp.h"	// Added by ClassView
#include "JString.h"	// Added by ClassView

namespace IscDbcLibrary {

class IscCallableStatement : public IscPreparedStatement, public CallableStatement  
{
public:
//{{{ specification jdbc
//	virtual BigDecimal	getBigDecimal( int parameterIndex, int scale );
	virtual bool		getBoolean(int parameterIndex);
	virtual char		getByte(int parameterIndex);
//	virtual byte[]		getBytes(int parameterIndex);
	virtual DateTime	getDate(int parameterIndex);
	virtual double		getDouble(int parameterIndex);
	virtual float		getFloat(int parameterIndex);
	virtual int			getInt(int parameterIndex);
	virtual QUAD		getLong(int parameterIndex);
//	virtual Object		getObject( int parameterIndex );
	virtual short		getShort(int parameterIndex);
	virtual const char* getString(int parameterIndex);
	virtual SqlTime		getTime(int parameterIndex);
	virtual TimeStamp	getTimestamp(int parameterIndex);
	virtual void		registerOutParameter(int parameterIndex, int sqlType);
	virtual void		registerOutParameter(int parameterIndex, int sqlType, int scale);
	virtual bool		wasNull();
//}}} specification jdbc

// {{{ class PreparedStatement specification jdbc
	virtual void		clearParameters() { IscPreparedStatement::clearParameters (); }
	virtual bool		execute();
	virtual ResultSet*	executeQuery() { return IscPreparedStatement::executeQuery (); }
	virtual int			executeUpdate() { return IscPreparedStatement::executeUpdate (); }
//	virtual void		setAsciiStream( int parameterIndex, InputStream x, int length ) { IscPreparedStatement::setAsciiStream( parameterIndex, x, length ); }
//	virtual void		setBigDecimal( int parameterIndex, BigDecimal x ) { IscPreparedStatement::setBigDecimal( parameterIndex, x ); }
//	virtual void		setBinaryStream( int parameterIndex, InputStream x, int length ) { IscPreparedStatement::setBinaryStream( parameterIndex, x, length ); }
	virtual void		setBoolean( int parameterIndex, bool x ) { IscPreparedStatement::setBoolean( parameterIndex, x ); }
	virtual void		setByte (int index, char value) { IscPreparedStatement::setByte ( index, value); }
	virtual void		setBytes (int index, const void *bytes) { IscPreparedStatement::setBytes ( index, bytes); }
	virtual void		setDate (int index, DateTime value) { IscPreparedStatement::setDate ( index, value); }
	virtual void		setDouble (int index, double value) { IscPreparedStatement::setDouble ( index, value); }
	virtual void		setFloat (int index, float value) { IscPreparedStatement::setFloat ( index, value); }
	virtual void		setInt (int index, int value) { IscPreparedStatement::setInt ( index, value); }
	virtual void		setLong (int index, QUAD value) { IscPreparedStatement::setLong ( index, value); }
	virtual void		setNull (int index, int type) { IscPreparedStatement::setNull ( index, type); }
//	virtual void		setObject( int parameterIndex, Object x ) { IscPreparedStatement::setObject( parameterIndex, x ); }
//	virtual void		setObject( int parameterIndex, Object x, int targetSqlType ) { IscPreparedStatement::setObject( parameterIndex, x, targetSqlType ); }
//	virtual void		setObject( int parameterIndex, Object x, int targetSqlType, int scale ) { IscPreparedStatement::setObject( parameterIndex, x, targetSqlType, scale ); }
	virtual void		setShort (int index, short value) { IscPreparedStatement::setShort ( index, value); }
	virtual void		setString (int index, const char * string) { IscPreparedStatement::setString ( index, string); }
	virtual void		setTime (int index, SqlTime value) { IscPreparedStatement::setTime ( index, value); }
	virtual void		setTimestamp (int index, TimeStamp value) { IscPreparedStatement::setTimestamp ( index, value); }
//	virtual void		setUnicodeStream( int parameterIndex, InputStream x, int length ) { IscPreparedStatement::setUnicodeStream( parameterIndex, x, length ); }
// }}}end class PreparedStatement specification jdbc

	virtual void		executeMetaDataQuery(){ IscPreparedStatement::executeMetaDataQuery(); }

	virtual void		setBytes (int index, int length, const void *bytes){ IscPreparedStatement::setBytes ( index, length, bytes); }
    virtual void        setString(int index, const char * string, int length){ IscPreparedStatement::setString( index, string, length); }
	virtual void        convStringData(int index){ IscPreparedStatement::convStringData( index); }
	virtual void		setBlob (int index, Blob *value){ IscPreparedStatement::setBlob ( index, value); }
	virtual void		setArray (int index, Blob *value){ IscPreparedStatement::setArray ( index, value); }
    virtual void        beginBlobDataTransfer(int index){ IscPreparedStatement::beginBlobDataTransfer( index ); }
    virtual void        putBlobSegmentData (int length, const void *bytes){ IscPreparedStatement::putBlobSegmentData ( length, bytes); }
    virtual void        endBlobDataTransfer(){ IscPreparedStatement::endBlobDataTransfer(); }
	virtual StatementMetaData*
						getStatementMetaDataIPD(){ return IscPreparedStatement::getStatementMetaDataIPD(); }
	virtual StatementMetaData*
						getStatementMetaDataIRD(){ return IscPreparedStatement::getStatementMetaDataIRD(); }
	virtual	int			getNumParams(){ return IscPreparedStatement::getNumParams(); }
// }}} end class PreparedStatement

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
	IscCallableStatement(IscConnection *connection);

	void				getToken (const char **ptr, char *token);
	const char *		rewriteSql (const char *originalSql, char *buffer, int length);
	virtual Blob*		getBlob (int id);
	Value*				getValue (int index);
	virtual void		prepare (const char *sql);
	virtual int			objectVersion();

public:
	Values				values;
	bool				valueWasNull;
	int					minOutputVariable;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_ISCCALLABLESTATEMENT_H_)
