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

// IscResultSet.h: interface for the IscResultSet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISCRESULTSET_H__C19738BA_1C87_11D4_98DF_0000C01D2301__INCLUDED_)
#define AFX_ISCRESULTSET_H__C19738BA_1C87_11D4_98DF_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Connection.h"
#include "LinkedList.h"
#include "Values.h"
#include "DateTime.h"	// Added by ClassView
#include "SqlTime.h"
#include "TimeStamp.h"	// Added by ClassView

class IscStatement;
class IscDatabaseMetaData;
class Sqlda;

enum enStatysActivePositionRow { enSUCCESS, enUNKNOWN, enINSERT_ROW, enAFTER_LAST, enBEFORE_FIRST };

class IscResultSet : public ResultSet, public StatementMetaData
{
public:
	void allocConversions();
	IscResultSet(IscStatement *iscStatement);
	virtual ~IscResultSet();
	void				initResultSet(IscStatement *iscStatement);
	virtual int			findColumn (const char *columName);
	virtual void		freeHTML(const char *html);
	virtual const char* genHTML(const char *series, const char *type, Properties *context);
	virtual void		close();
	virtual bool		next();
	virtual StatementMetaData* getMetaData();
	virtual int			release();
	virtual void		addRef();
	virtual bool		wasNull();

//public StatementMetaData
	virtual int			getColumnCount();
	virtual int			getColumnType (int index, int &realSqlType);
	virtual int			getPrecision(int index);
	virtual int			getScale(int index);
	virtual bool		isNullable (int index);
	virtual int			getColumnDisplaySize(int index);
	virtual const char* getColumnLabel(int index);
	virtual const char* getSqlTypeName(int index);
	virtual const char* getColumnName(int index);
	virtual const char* getTableName(int index);
	virtual const char* getColumnTypeName(int index);
	virtual bool		isSigned (int index);
	virtual bool		isReadOnly (int index);
	virtual bool		isWritable (int index);
	virtual bool		isDefinitelyWritable (int index);
	virtual bool		isCurrency (int index);
	virtual bool		isCaseSensitive (int index);
	virtual bool		isAutoIncrement (int index);
	virtual bool		isSearchable (int index);
	virtual int			isBlobOrArray(int index);
	virtual const char*	getSchemaName (int index);
	virtual const char*	getCatalogName (int index);

	virtual void		getSqlData(int index, char *& ptData, short *& ptIndData);
	virtual void		setSqlData(int index, long ptData, long ptIndData);
	virtual void		saveSqlData(int index, long ptData, long ptIndData);
	virtual void		restoreSqlData(int index);
// end public StatementMetaData

	void		deleteBlobs();
	void		reset();

public:
	void				setNull (int index);
	virtual int			objectVersion();
	virtual Value*		getValue (int index);
	virtual Value*		getValue (const char *columnName);
	virtual bool		isNull(int index);
	virtual const char* getString (int index);
	virtual const char* getString (const char *columnName);
	virtual TimeStamp	getTimestamp (int index);
	virtual TimeStamp	getTimestamp (const char * columnName);
	virtual SqlTime		getTime (int index);
	virtual SqlTime		getTime (const char * columnName);
	virtual DateTime	getDate (int index);
	virtual DateTime	getDate (const char * columnName);
	virtual long		getInt (int index);
	virtual long		getInt (const char *columnName);
	virtual float		getFloat (int index);
	virtual float		getFloat (const char * columnName);
	virtual char		getByte (int index);
	virtual char		getByte (const char *columnName);
	virtual Blob*		getBlob (int index);
	virtual Blob*		getBlob (const char * columnName);
	virtual double		getDouble (int index);
	virtual double		getDouble (const char * columnName);
	virtual QUAD		getQuad (int index);
	virtual QUAD		getQuad (const char *columnName);
	virtual short		getShort (int index);
	virtual short		getShort (const char * columnName);

	virtual bool		isBeforeFirst();
	virtual bool		isAfterLast();
	virtual bool		isFirst();
	virtual bool		isLast();
	virtual void		beforeFirst();
	virtual void		afterLast();
	virtual bool		first();
	virtual bool		last();
	virtual int			getRow();
	virtual bool		absolute (int row);
	virtual bool		relative (int rows);
	virtual bool		previous();
	virtual void		setFetchDirection (int direction);
	virtual int			getFetchDirection ();
	virtual int			getFetchSize();
	virtual int			getType();
	virtual bool		rowUpdated();
	virtual bool		rowInserted();
	virtual bool		rowDeleted();
	virtual void		updateNull (int columnIndex);
	virtual void		updateBoolean (int columnIndex, bool value);
	virtual void		updateByte (int columnIndex, char value);
	virtual void		updateShort (int columnIndex, short value);
	virtual void		updateInt (int columnIndex, int value);
	virtual void		updateLong (int columnIndex, QUAD value);
	virtual void		updateFloat (int columnIndex, float value);
	virtual void		updateDouble (int columnIndex, double value);
	virtual void		updateString (int columnIndex, const char* value);
	virtual void		updateBytes (int columnIndex, int length, const void *bytes);
	virtual void		updateDate (int columnIndex, DateTime value);
	virtual void		updateTime (int columnIndex, SqlTime value);
	virtual void		updateTimeStamp (int columnIndex, TimeStamp value);
	virtual void		updateBlob (int columnIndex, Blob* value);
	virtual void		updateNull (const char *columnName);
	virtual void		updateBoolean (const char *columnName, bool value);
	virtual void		updateByte (const char *columnName, char value);
	virtual void		updateShort (const char *columnName, short value);
	virtual void		updateInt (const char *columnName, int value);
	virtual void		updateLong (const char *columnName, QUAD value);
	virtual void		updateFloat (const char *columnName, float value);
	virtual void		updateDouble (const char *columnName, double value);
	virtual void		updateString (const char *columnName, const char* value);
	virtual void		updateBytes (const char *columnName, int length, const void *bytes);
	virtual void		updateDate (const char *columnName, DateTime value);
	virtual void		updateTime (const char *columnName, SqlTime value);
	virtual void		updateTimeStamp (const char *columnName, TimeStamp value);
	virtual void		updateBlob (const char *columnName, Blob* value);
	virtual void		insertRow();
	virtual void		updateRow();
	virtual void		deleteRow();
	virtual void		refreshRow();
	virtual void		cancelRowUpdates();
	virtual void		moveToInsertRow();
	virtual void		moveToCurrentRow();
	virtual Statement	*getStatement();
	virtual void		setPosRowInSet(int posRow);
	virtual int			getPosRowInSet();
	virtual bool		readStaticCursor();
	virtual bool		readForwardCursor();
	virtual bool		setCurrentRowInBufferStaticCursor(int nRow);
	virtual void		copyNextSqldaInBufferStaticCursor();
	virtual void		copyNextSqldaFromBufferStaticCursor();
	virtual int			getCountRowsStaticCursor();
	virtual bool		getDataFromStaticCursor (int column, int cType, void * pointer, int bufferLength, long * indicatorPointer);

	void setValue (int index, long value);
	void setValue (int index, const char *value);

	int				numberColumns;
	void			*handle;
	int				useCount;
	Values			values;
	char			**conversions;
	char			**columnNames;
	bool			valueWasNull;
	LinkedList		blobs;
	LinkedList		clobs;
	Sqlda			*sqlda;
	IscStatement	*statement;
	int				activePosRowInSet;
	enStatysActivePositionRow statysPositionRow;
};

#endif // !defined(AFX_ISCRESULTSET_H__C19738BA_1C87_11D4_98DF_0000C01D2301__INCLUDED_)
