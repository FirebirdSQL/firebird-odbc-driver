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

// IscResultSet.h: interface for the IscResultSet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ISCRESULTSET_H_)
#define _ISCRESULTSET_H_

#include "Connection.h"
#include "LinkedList.h"
#include "Values.h"
#include "DateTime.h"	// Added by ClassView
#include "SqlTime.h"
#include "TimeStamp.h"	// Added by ClassView
#include "IscStatementMetaData.h"
#include "Sqlda.h"

namespace IscDbcLibrary {

class IscStatement;
class IscDatabaseMetaData;

enum enStatysActivePositionRow { enSUCCESS, enUNKNOWN, enINSERT_ROW, enAFTER_LAST, enBEFORE_FIRST };

class IscResultSet : public ResultSet, public IscStatementMetaData
{
public:
//{{{ specification jdbc
//	virtual void		clearWarnings();
	virtual void		close();
	virtual int			findColumn (const char *columName);
//	virtual	InputStream* getAsciiStream( int columnIndex );
//	virtual	InputStream* getAsciiStream( const char *columnName );
//	virtual	BigDecimal	getBigDecimal( int columnIndex, int scale );
//	virtual	BigDecimal	getBigDecimal( const char *columnName, int scale );
//	virtual	InputStream getBinaryStream( int columnIndex );
//	virtual	InputStream getBinaryStream( const char *columnName );
	virtual	bool		getBoolean( int columnIndex );
	virtual	bool		getBoolean( const char *columnName );
	virtual char		getByte (int columnIndex);
	virtual char		getByte (const char *columnName);
//	virtual byte[]		getBytes( int columnIndex );
//	virtual byte[]		getBytes( int columnIndex );
//	virtual byte[]		getBytes( const char *columnName );
	virtual const char* getCursorName();
	virtual DateTime	getDate (int columnIndex);
	virtual DateTime	getDate (const char *columnName);
	virtual double		getDouble (int columnIndex);
	virtual double		getDouble (const char *columnName);
	virtual float		getFloat (int columnIndex);
	virtual float		getFloat (const char *columnName);
	virtual int			getInt (int columnIndex);
	virtual int			getInt (const char *columnName);
	virtual QUAD		getLong (int columnIndex);
	virtual QUAD		getLong (const char *columnName);
	virtual StatementMetaData* getMetaData();
//	virtual Object		getObject( int columnIndex );
	virtual short		getShort (int columnIndex);
	virtual short		getShort (const char *columnName);
	virtual const char* getString (int columnIndex);
	virtual const char* getString (const char *columnName);
	virtual SqlTime		getTime (int columnIndex);
	virtual SqlTime		getTime (const char *columnName);
	virtual TimeStamp	getTimestamp (int columnIndex);
	virtual TimeStamp	getTimestamp (const char *columnName);
//	virtual InputStream getUnicodeStream( int columnIndex );
//	virtual InputStream getUnicodeStream( const char *columnName );
//	virtual void		getWarnings();
	virtual bool		next();
	virtual bool		wasNull();
//}}} end specification jdbc

public:
	IscResultSet(IscStatement *iscStatement);
	virtual ~IscResultSet();

	virtual int			objectVersion();

	virtual void		initResultSet(IscStatement *iscStatement);
	void				allocConversions();
	virtual void		freeHTML(const char *html);
	virtual const char* genHTML(const char *series, const char *type, Properties *context);
	virtual int			release();
	virtual void		addRef();
	virtual int			getColumnCount();

	void				deleteBlobs();
	void				reset();

	virtual Value*		getValue (int index);
	virtual Value*		getValue (const char *columnName);
	virtual void		setNull (int index);
	virtual Blob*		getBlob (int index);
	virtual Blob*		getBlob (const char * columnName);

	virtual bool		isBeforeFirst();
	virtual bool		isAfterLast();
	virtual bool		isCurrRowsetStart();
	virtual bool		isFirst();
	virtual bool		isLast();
	virtual void		beforeFirst();
	virtual void		afterLast();
	virtual void		currRowsetStart();
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
	virtual void		updateText (int columnIndex, const char* value);
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
	virtual size_t		*getSqlDataOffsetPtr();
	virtual bool		readStaticCursor();
	virtual bool		readFromSystemCatalog();
	virtual bool		nextFetch();
	virtual bool		setCurrentRowInBufferStaticCursor(int nRow);
	virtual void		copyNextSqldaInBufferStaticCursor();
	virtual void		copyNextSqldaFromBufferStaticCursor();
	virtual int			getCountRowsStaticCursor();
	virtual bool		getDataFromStaticCursor (int column);
	virtual bool		nextFromProcedure();

	void				setValue (int index, int value);
	void				setValue (int index, const char *value);

	int				numberColumns;
	void			*handle;
	int				useCount;
	Values			values;
	char			**conversions;
	char			**columnNames;
	bool			valueWasNull;
	bool			nextSimulateForProcedure;
	LinkedList		blobs;
	LinkedList		clobs;
	int				activePosRowInSet;
	size_t			sqldataOffsetPtr;
	enStatysActivePositionRow statysPositionRow;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_ISCRESULTSET_H_)
