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
 *
 *	2002-06-04 See comments in IscCallableStatement.cpp for details
 *
 */


// IscCallableStatement.h: interface for the IscCallableStatement class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISCCALLABLESTATEMENT_H__10B4AD63_3637_11D4_98E4_0000C01D2301__INCLUDED_)
#define AFX_ISCCALLABLESTATEMENT_H__10B4AD63_3637_11D4_98E4_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Connection.h"
#include "IscPreparedStatement.h"
#include "Values.h"
#include "DateTime.h"	// Added by ClassView
#include "SqlTime.h"
#include "TimeStamp.h"	// Added by ClassView
#include "JString.h"	// Added by ClassView

class IscCallableStatement : public IscPreparedStatement, public CallableStatement  
{
public:
	void getToken (const char **ptr, char *token);
	const char *rewriteSql (const char *originalSql, char *buffer, int length);
	typedef IscPreparedStatement	Parent;

	IscCallableStatement(IscConnection *connection);
	virtual ~IscCallableStatement();

	virtual void		registerOutParameter(int parameterIndex, int sqlType, int scale);
	virtual void		registerOutParameter(int parameterIndex, int sqlType);
	virtual const char*	getString (int id);
	virtual bool		wasNull();
	virtual TimeStamp	getTimestamp (int id);
	virtual DateTime	getDate(int id);
	virtual SqlTime		getTime (int id);
	virtual Blob*		getBlob (int id);
	virtual double		getDouble  (int id);
	virtual float		getFloat (int id);
	virtual QUAD		getQuad(int id);
	virtual long		getInt (int id);
	virtual char		getByte  (int id);
	virtual short		getShort (int id);

	virtual int			objectVersion();
	virtual StatementMetaData* getStatementMetaDataIPD();
	virtual StatementMetaData* getStatementMetaDataIRD();
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
	virtual int			release();
	virtual void		addRef();

	virtual void		setNull (int index, int type);
	virtual void		setString(int index, const char * string);
    virtual void        setString(int index, const char * string, int length);
	virtual void        convStringData(int index);
	virtual void		setByte (int index, char value);
	virtual void		setShort (int index, short value);
	virtual void		setInt (int index, long value);
	virtual void		setQuad (int index, QUAD value);
	virtual void		setFloat (int index, float value);
	virtual void		setDouble (int index, double value);
	virtual void		setDate (int index, DateTime value);
	virtual void		setTimestamp (int index, TimeStamp value);
	virtual void		setTime (int index, SqlTime value);
	virtual void		setArray (int index, Blob *value);

	virtual void		setBlob (int index, Blob *value);
	virtual void		setBytes (int index, int length, const void *bytes);

//Added by RM 2002-06-04
    virtual void        beginBlobDataTransfer(int index);
    virtual void        putBlobSegmentData (int length, const void *bytes);
    virtual void        endBlobDataTransfer();
	
	virtual int			executeUpdate();
	virtual bool		execute();
	virtual ResultSet*	executeQuery();

	Value*				getValue (int index);
	virtual void		prepare (const char *sql);
	virtual	int			getNumParams();

	Values			values;
	bool			valueWasNull;
	int				minOutputVariable;

//////////////////////////////////////////////
	virtual int			getStmtPlan(const void * value, int bufferLength,long *lengthPtr)
	{ return getPlanStatement(statementHandle,value,bufferLength,lengthPtr); }  
	virtual int			getStmtType(const void * value, int bufferLength,long *lengthPtr)
	{ return getTypeStatement(statementHandle,value,bufferLength,lengthPtr); }  
	virtual int			getStmtInfoCountRecords(const void * value, int bufferLength,long *lengthPtr)
	{ return getInfoCountRecordsStatement(statementHandle,value,bufferLength,lengthPtr); }  
};

#endif // !defined(AFX_ISCCALLABLESTATEMENT_H__10B4AD63_3637_11D4_98E4_0000C01D2301__INCLUDED_)
