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
 */

// Sqlda.h: interface for the Sqlda class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_SQLDA_H_INCLUDED_)
#define _SQLDA_H_INCLUDED_

#include <vector>
#include "IscArray.h"
#include <sqltypes.h>

namespace IscDbcLibrary {

class CAttrSqlVar
{
public:
	CAttrSqlVar() :
		offsetData{ 0 },
		offsetNull{ 0 },
		isNullable{ false },
		sqlname{ nullptr },
		relname{ nullptr },
		aliasname{ nullptr },
		sqltype{ 0 },
		sqlscale{ 0 },
		sqlsubtype{ 0 },
		sqllen{ 0 },
		sqldata{ nullptr },
		sqlind{ nullptr },
		array{ nullptr },
		index{ 0 },
		replaceForParamArray{ false },
		wasExternalOverriden{ false }
	{}

	~CAttrSqlVar()	
	{ 
		if ( array ) 
			delete array; 
	}

	using buffer_t = std::vector<char>;

	inline void assign( Firebird::ThrowStatusWrapper& status, Firebird::IMessageMetadata* _meta, buffer_t& _buffer, unsigned _index ) {
		index = _index;
		//
		offsetData = _meta->getOffset    ( &status, index );
		offsetNull = _meta->getNullOffset( &status, index );
		isNullable = _meta->isNullable   ( &status, index );
		sqlname    = _meta->getField     ( &status, index );
		relname    = _meta->getRelation  ( &status, index );
		aliasname  = _meta->getAlias     ( &status, index );
		sqltype    = _meta->getType      ( &status, index ) & ~1;
		sqlscale   = _meta->getScale     ( &status, index );
		sqlsubtype = _meta->getSubType   ( &status, index );
		sqllen     = _meta->getLength    ( &status, index );

		assignBuffer( _buffer );
		//
		++index; //to make it 1-based)
	}

	inline void assignBuffer(buffer_t& buffer ) {
		sqldata = &buffer.at( offsetData );
		sqlind  = (short*)&buffer.at( offsetNull );
	}
	//TODO: deprecate it!
	inline void assignBuffer(char* buffer) {
		sqldata = buffer + offsetData;
		sqlind = (short*)(buffer + offsetNull);
	}

	unsigned		offsetData;
	unsigned		offsetNull;
	bool			isNullable;
	const char*		sqlname;
	const char*		relname;
	const char*		aliasname;
	short			sqltype;			/* datatype of field */
	short			sqlscale;			/* scale factor */
	short			sqlsubtype;			/* datatype subtype - BLOBs & Text types only */
	short			sqllen;				/* length of data area */
	char*           sqldata;
	short*          sqlind;
	CAttrArray		*array;
	unsigned		index;
	bool			replaceForParamArray;
	bool			wasExternalOverriden;
};

class Value;
class IscConnection;
class CDataStaticCursor;

class Sqlda  
{
protected:
	char* initStaticCursor(IscStatement *stmt);
	char* addRowSqldaInBufferStaticCursor();
	void restoreOrgAdressFieldsStaticCursor();

public:
	const char* getOwnerName (int index);
	int findColumn (const char *columnName);
	void setBlob (CAttrSqlVar* var, Value * value, IscStatement *stmt);
	void setArray (CAttrSqlVar* var, Value *value, IscStatement *stmt);
	void setValue (int slot, Value *value, IscStatement	*stmt);
	const char* getTableName (int index);
	int getSqlType (CAttrSqlVar *var, int &realSqlType);
	const char* getSqlTypeName (CAttrSqlVar *var);
	bool isNullable (int index);
	int getScale (int index);
	int getPrecision (int index);
	int getNumPrecRadix(int index);
	const char* getColumnLabel (int index);
	const char* getColumnName (int index);
	int getColumnDisplaySize (int index);
	short getSubType(int index);
	int getColumnType (int index, int &realSqlType);
	const char * getColumnTypeName (int index);
	void print();
	bool setCurrentRowInBufferStaticCursor(int nRow);
	void getAdressFieldFromCurrentRowInBufferStaticCursor(int column, char *& sqldata, short *& sqlind);
	void copyNextSqldaInBufferStaticCursor();
	void copyNextSqldaFromBufferStaticCursor();
	void saveCurrentSqldaToBuffer();
	void restoreBufferToCurrentSqlda();
	int getCountRowsStaticCursor();
	int getColumnCount();
	void init();
	void remove();
	void allocBuffer(IscStatement *stmt, Firebird::IMessageMetadata* msgMetadata);
	void mapSqlAttributes(IscStatement *stmt);
	void deleteSqlda();
	void clearSqlda();
	CAttrSqlVar* orgVar(int index) { return &orgsqlvar.at( index - 1 ); }

	Sqlda( IscConnection* conn );
	~Sqlda();

	int isBlobOrArray(int index);
	bool isNull(int index);
	void setNull(int index);
	void setNotNull(int index);

	bool getBoolean (int index);
	short getShort (int index);
	int getInt (int index);
	char * getText (int index, int &len);
	char * getVarying (int index, int &len);

	void updateBoolean (int index, bool value);
	void updateShort (int index, short value);
	void updateInt (int index, int value);
	void updateText (int index, const char* value);
	void updateVarying (int index, const char* dst);

	CDataStaticCursor * dataStaticCursor;
	int			lengthBufferRows;

	IscConnection* connection;
	Firebird::IMessageMetadata* meta;

	using buffer_t = std::vector<char>;
	buffer_t buffer;

	using orgsqlvar_t = std::vector<CAttrSqlVar>;
	orgsqlvar_t orgsqlvar;
	unsigned columnsCount;

	friend class IscResultSet;

	Firebird::IMessageMetadata* setStrProperties( int index, const char* fldName, const char* fldRelation, const char* fldAlias );

	inline bool isExternalOverriden() {
		for( auto & var : orgsqlvar ) if (var.wasExternalOverriden) return true;
		return false;
	}
	void rebuildMetaFromAttributes( IscStatement *stmt );
};

}; // end namespace IscDbcLibrary

#endif // !defined(_SQLDA_H_INCLUDED_)
