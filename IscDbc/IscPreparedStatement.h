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
 *
 *	2002-06-04	See comment in IscPreparedStatement.cpp
 *
 *
 */

// IscPreparedStatement.h: interface for the IscPreparedStatement class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISCPREPAREDSTATEMENT_H__C19738B9_1C87_11D4_98DF_0000C01D2301__INCLUDED_)
#define AFX_ISCPREPAREDSTATEMENT_H__C19738B9_1C87_11D4_98DF_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Connection.h"
#include "IscStatement.h"
#include "Values.h"

#define DEFAULT_BLOB_BUFFER_LENGTH 16384

class IscConnection;
class IscStatementMetaData;
//Added by RM
class BinaryBlob;
class AsciiBlob;

class IscPreparedStatement : public IscStatement, public PreparedStatement
{
public:
	virtual int objectVersion();
	virtual StatementMetaData* getStatementMetaDataIPD();
	virtual StatementMetaData* getStatementMetaDataIRD();
	virtual ~IscPreparedStatement();
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
//Added by RM
    virtual void        beginBlobDataTransfer(int index);
    virtual void        putBlobSegmentData (int length, const void *bytes);
    virtual void        endBlobDataTransfer();	

	virtual void		setShort (int index, short value);
	virtual void		setInt (int index, long value);
	virtual void		setQuad (int index, QUAD value);
	virtual void		setFloat (int index, float value);
	virtual void		setDouble (int index, double value);
	virtual void		setDate (int index, DateTime value);
	virtual void		setTimestamp (int index, TimeStamp value);
	virtual void		setTime (int index, SqlTime value);
	virtual void		setBlob (int index, Blob *value);
	virtual void		setBytes (int index, int length, const void *bytes);
	virtual void		setArray (int index, Blob *value);

	virtual int			executeUpdate();
	virtual bool		execute();
	virtual ResultSet*	executeQuery();

	void				getInputParameters();
	virtual int			getNumParams();
	virtual void		prepare (const char *sqlString);
	IscPreparedStatement(IscConnection *connect);
	Value* getParameter (int index);

	Values				parameters;
	IscStatementMetaData	*statementMetaDataIPD;
	IscStatementMetaData	*statementMetaDataIRD;
    BinaryBlob              *segmentBlob;
	AsciiBlob				*segmentClob;
	virtual int			getStmtPlan(const void * value, int bufferLength,long *lengthPtr)
	{ return getPlanStatement(connection, statementHandle,value,bufferLength,lengthPtr); }  
	virtual int			getStmtType(const void * value, int bufferLength,long *lengthPtr)
	{ return getTypeStatement(connection, statementHandle,value,bufferLength,lengthPtr); }  
	virtual int			getStmtInfoCountRecords(const void * value, int bufferLength,long *lengthPtr)
	{ return getInfoCountRecordsStatement(connection, statementHandle, value,bufferLength,lengthPtr); }  
};

#endif // !defined(AFX_ISCPREPAREDSTATEMENT_H__C19738B9_1C87_11D4_98DF_0000C01D2301__INCLUDED_)
