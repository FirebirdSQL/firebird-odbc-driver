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
 *  2002-11-25	Sqlda.cpp
 *				Contributed by C. G. Alvarez
 *				Changes to support better handling of 
 *				NUMERIC and DECIMAL
 *
 *  2002-10-11	Sqlda.cpp
 *				Contributed by C. G. Alvarez
 *              Extensive modifications to getDisplaySixe()
 *              and getPrecision() to take advantage of MAX_****
 *              constants. Other mods. to getSqlType()
 *
 *  2002-08-12	Sqlda.cpp
 *				Contributed by C. G. Alvarez
 *				Added getColumnTypeName()
 *
 *  2002-08-02	Sqlda.cpp
 *				Contributed by C. G. Alvarez
 *				Change getColumnType to pass var->sqlscale to getSQLType.   
 *				Change getSQLTypeName to keep in sync with this. 
 *				The purpose is to allow return of DECIMAL as JDBC_DECIMAL 
 *				instead of JDBC_BIGINT.
 *
 *	2002-06-04	Sqlda.cpp
 *				Contributed by Robert Milharcic
 *				Amended getColumnDisplaySize() and getPrecision()
 *				to return char and varchar lengths more correctly.
 *
 */

// Sqlda.cpp: implementation of the Sqlda class.
//
//////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include "IscDbc.h"
#include "Sqlda.h"
#include "SQLError.h"
#include "Value.h"
#include "IscConnection.h"
#include "IscStatement.h"
#include "IscBlob.h"

using namespace Firebird;

namespace IscDbcLibrary {

#define SET_INFO_FROM_SUBTYPE( a, b, c ) \
		var->sqlsubtype == 1 || (!var->sqlsubtype && var->sqlscale) ? (a) : \
		var->sqlsubtype == 2 ? (b) : (c)

static short sqlNull = -1;

class CDataStaticCursor
{
public:
	Sqlda::orgsqlvar_t & ptSqlVars;
	bool	bYesBlob;
	static constexpr int nMAXROWBLOCK = 40;
	int		lenRow;
	int		countBlocks;
	int		countAllRows;
	int		curBlock;

	using vchar_t = Sqlda::buffer_t;
	using rowBlock_t = std::vector<vchar_t>;

	struct RowBlock {
		rowBlock_t rows;
		size_t size() { return rows.size(); }
		RowBlock(size_t row_count = 0, int row_len = 0) :
			rows{ row_count, static_cast<vchar_t>(row_len) }
		{}
	};

	std::vector<RowBlock> listBlocks;
	rowBlock_t::reverse_iterator itCurrentRow; //we use reverse iterator because we need to set it _before_ begin()

	vchar_t & ptOrgRowBlock;
	unsigned numberColumns;
	int		minRow;
	int		maxRow;
	int		curRow;
	std::vector<short> numColumnBlob;
	short	countColumnBlob;
	IscStatement	*statement;

public:

	CDataStaticCursor ( IscStatement *stmt, vchar_t & buffer, Sqlda::orgsqlvar_t & sqlVars, unsigned columnsCount, int lnRow)
		: ptSqlVars{ sqlVars },
		  ptOrgRowBlock{ buffer }
	{
		statement = stmt;
		bYesBlob = false;
		lenRow = lnRow;

		countBlocks = 10;
		countAllRows = 0;
		listBlocks.resize( countBlocks );
		listBlocks.at(0) = { nMAXROWBLOCK, lnRow };
		itCurrentRow = std::prev( listBlocks.at(0).rows.rend() ); // == begin()

		curBlock = 0;
		minRow = 0;
		maxRow = nMAXROWBLOCK;
		curRow = 0;
		numberColumns = columnsCount;

		for( auto & var : ptSqlVars )
		{
			switch (var.sqltype)
			{
			case SQL_ARRAY:
			case SQL_BLOB:
				numColumnBlob.push_back( var.index - 1 );
				break;
			}
			var.assignBuffer( *itCurrentRow );
		}
		countColumnBlob = (short)numColumnBlob.size();
		bYesBlob = countColumnBlob > 0;
	}

	~CDataStaticCursor()
	{
		if ( bYesBlob )
		{
			for ( auto i = 0; i < countColumnBlob; ++i )
			{
				auto & var = ptSqlVars.at( numColumnBlob[i] );

				if (var.sqltype == SQL_ARRAY || var.sqltype == SQL_BLOB)
				{
					for (auto& rowBlock : listBlocks)
					{
						for (unsigned i = 0; i < rowBlock.size(); ++i)
						{
							auto& row = rowBlock.rows.at(i);
							auto* pt = &row.at(var.offsetData);

							if (pt && *(intptr_t*)pt)
							{
								if (var.sqltype == SQL_ARRAY)
								{
									free(((CAttrArray*)*(intptr_t*)pt)->arrBufData);
									delete (CAttrArray*)*(intptr_t*)pt;
								}
								else
								{
									delete (IscBlob*)*(intptr_t*)pt;
								}
							}
						}
					}
				}
			}
		}
	}

	vchar_t& addRow ()
	{
		if ( bYesBlob )
		{
			int n;
			auto & sqlvar = ptSqlVars;
			for ( n = 0; n < countColumnBlob; ++n )
			{
				auto & var = sqlvar.at( numColumnBlob[n] );
				if ( *var.sqlind == -1 )
					*(intptr_t*)var.sqldata = 0;
				else if ( (var.sqltype) == SQL_ARRAY )
				{
					CAttrArray * ptArr = new CAttrArray;
					IscArray iscArr(statement,&var);
					iscArr.getBytesFromArray();
					iscArr.detach(ptArr);
					*(intptr_t*)var.sqldata = (intptr_t)ptArr;
				}
				else if ( var.sqltype == SQL_BLOB )
				{
					IscBlob * ptBlob = new IscBlob (statement, var.sqldata, var.sqlsubtype);
					ptBlob->fetchBlob();
					*(intptr_t*)var.sqldata = (intptr_t)ptBlob;
				}
			}
		}

		this->copyToCurrentSqlda(ptOrgRowBlock);	//save fetched buffer to current row
		nextPosition();								//scroll to the next position
		auto& row = *itCurrentRow;

		for( auto & var : ptSqlVars ) var.assignBuffer( row );

		++countAllRows;
		return row;
	}

	inline void restoreOriginalAdressFieldsSqlDa()
	{
		for( auto & var : ptSqlVars ) var.assignBuffer( ptOrgRowBlock );
	}

	bool current(int nRow)
	{
		int i, n;

		assert( nRow >= 0 );

		if( !(nRow >= minRow && nRow < maxRow) )
		{
			for (i = 0, n = listBlocks.at(i).size();
				nRow > n && i < countBlocks;
				n += listBlocks.at(++i).size());

			curBlock = i;
			maxRow = n;
			minRow = maxRow - listBlocks.at(curBlock).size();
		}

		curRow = nRow;
		itCurrentRow = listBlocks.at(curBlock).rows.rend();
		std::advance( itCurrentRow, -(curRow - minRow) );
		--curRow; // We put previous for use next() !!!

		return true;
	}

	void getAdressFieldFromCurrentRowInBufferStaticCursor(int column, char *& sqldata, short *& sqlind)
	{
		auto it = std::prev(itCurrentRow);
		auto& row = *it;
		auto& var = ptSqlVars.at(column - 1);
		sqldata = &row.at(var.offsetData);
		sqlind = (short*)&row.at(var.offsetNull);
	}

	vchar_t& nextPosition()
	{
		if (++curRow < maxRow)
		{
			std::advance( itCurrentRow, -1 ); // == ++it
		}
		else
		{
			if ( ++curBlock == countBlocks )
			{
				countBlocks += 10;
				listBlocks.resize( countBlocks );
			}
			
			if ( listBlocks.at(curBlock).rows.size() == 0 )
			{
				listBlocks.at(curBlock) = { nMAXROWBLOCK, lenRow };
			}

			itCurrentRow = std::prev( listBlocks.at(curBlock).rows.rend() );
			minRow = curRow;
			maxRow = minRow + listBlocks.at(curBlock).size();
		}

		return *itCurrentRow;
	}

	inline int getCountRowsStaticCursor()
	{
		return countAllRows;
	}

	inline void operator << (Sqlda::buffer_t & buf)
	{
		nextPosition() = buf;
	}

	inline void operator >> (Sqlda::buffer_t & buf)
	{
		buf = nextPosition();
	}

	inline void copyToBuffer(Sqlda::buffer_t & buf)
	{
		buf = *itCurrentRow;
	}

	inline void copyToCurrentSqlda(Sqlda::buffer_t & buf)
	{
		*itCurrentRow = buf;
	}
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction Sqlda
//////////////////////////////////////////////////////////////////////

Sqlda::Sqlda(IscConnection* conn, e_sqlda_dir dir) : connection{ conn }, buffer{}, execBuffer{}, useExecBufferMeta{ false }, SqldaDir{ dir }
{
	init();
}

Sqlda::~Sqlda()
{
	remove();
}

void Sqlda::init()
{
	execMeta = meta = nullptr;
	sqlvar.clear();
	dataStaticCursor = nullptr;
	columnsCount = 0;
}

void Sqlda::remove()
{
	delete dataStaticCursor;
	deleteSqlda(); // Should stand only here!!!
}

void Sqlda::clearSqlda()
{
	remove();
	init();
}

void Sqlda::deleteSqlda()
{
	sqlvar.clear();
	buffer.clear();
	execBuffer.clear();

	if( meta ) {
		meta->release();
		meta = nullptr;
	}
	if ( execMeta ) {
		execMeta->release();
		execMeta = nullptr;
	}
	useExecBufferMeta = false;
}

void Sqlda::allocBuffer ( IscStatement *stmt, IMessageMetadata* msgMetadata )
{
	if( !msgMetadata )
		throw SQLEXCEPTION (RUNTIME_ERROR, "Sqlda::allocBuffer(): no metadata");

	if( meta ) meta->release();
	meta = msgMetadata;

	if (execMeta)
	{
		execMeta->release();
		execMeta = nullptr;
	}
	execBuffer.clear();
	useExecBufferMeta = false;

	ThrowStatusWrapper status( connection->GDS->_status );
	try
	{
		lengthBufferRows = meta->getMessageLength( &status );
		columnsCount     = meta->getCount( &status );
	}
	catch( const FbException& error )
	{
		THROW_ISC_EXCEPTION ( connection, error.getStatus() );
	}

	buffer.resize( lengthBufferRows );
	mapSqlAttributes( stmt );
}

/*
*	This method is used to build meta & buffer just before the execution, taking into account sqlvar change during bind
*/
bool Sqlda::checkAndRebuild()
{
	const bool overrideFlag = isExternalOverriden();

	if (overrideFlag)
	{
		IMetadataBuilder* metaBuilder = nullptr;
		ThrowStatusWrapper status(connection->GDS->_status);

		auto build_new_meta = [&]()->IMessageMetadata*
		{
			if (!meta)
				throw SQLEXCEPTION(RUNTIME_ERROR, "Sqlda::ExecBuilder(): sqlda.meta==null before execute!");

			metaBuilder = meta->getBuilder(&status);

			for (const auto& var : sqlvar)
			{
				const auto i = var.index - 1;
				metaBuilder->setType(&status, i, var.sqltype | (short)(var.isNullable ? 1 : 0));

				//ooapi provides charset in separated field, so we should set subtype=0 for text types
				const auto subtype = (var.sqltype == SQL_TEXT || var.sqltype == SQL_VARYING) ? 0 : var.sqlsubtype;
				metaBuilder->setSubType(&status, i, subtype);

				metaBuilder->setCharSet(&status, i, var.sqlcharset);
				metaBuilder->setScale(&status, i, var.sqlscale);
				metaBuilder->setLength(&status, i, var.sqllen);
			}

			auto* res = metaBuilder->getMetadata(&status);
			metaBuilder->release();
			metaBuilder = nullptr;
			return res;
		};

		try
		{
			//printf("Rebuilding metadata due to sqlvar changes...\n");

			if (execMeta)
			{
				execMeta->release();
			}

			execMeta = build_new_meta();

			if (execMeta->getCount(&status) != columnsCount)
			{
				throw SQLEXCEPTION(RUNTIME_ERROR, "Sqlda::checkAndRebuild(): incorrect columns count");
			}

			lengthBufferRows = execMeta->getMessageLength(&status);
			execBuffer.clear();
			execBuffer.resize(lengthBufferRows);

			for (auto& var : sqlvar)
			{
				const auto i = var.index - 1;
				//save effective sqldata&sqlind, pointing to the new execBuffer
				var.eff_sqldata = &execBuffer.at(execMeta->getOffset(&status, i));
				var.eff_sqlind = (short*)&execBuffer.at(execMeta->getNullOffset(&status, i));
				//update last sqlvars to avoid buffers rebuilt next time
				var.lastSqlProperties = var;
			}

			useExecBufferMeta = true;
		}
		catch (const FbException& error)
		{
			if (metaBuilder) metaBuilder->release();
			THROW_ISC_EXCEPTION(connection, error.getStatus());
		}
		catch (...)
		{
			if (metaBuilder) metaBuilder->release();
			throw;
		}
	}
	else
	{
		// Metadata not overridden — effective pointers == original pointers,
		// no copy needed (Phase 9.8 optimization).
	}

	// Only copy data when we have a separate exec buffer (i.e., metadata was rebuilt)
	if (useExecBufferMeta)
	{
		// 10.3.1: On re-execute (!overrideFlag), the eff pointers are known-different
		// from the original pointers (they were set during a prior rebuild). Skip the
		// per-column pointer comparison and copy unconditionally — the branch prediction
		// savings outweigh the cost of a small unconditional memcpy.
		if (!overrideFlag)
		{
			for (const auto& var : sqlvar)
			{
				memcpy(var.eff_sqlind, var.sqlind, sizeof(short));
				if (*var.eff_sqlind != sqlNull)
					memcpy(var.eff_sqldata, var.sqldata, var.sqllen);
			}
		}
		else
		{
			for (const auto& var : sqlvar)
			{
				if (var.eff_sqlind != var.sqlind)
					memcpy(var.eff_sqlind, var.sqlind, sizeof(short));

				if (var.eff_sqldata != var.sqldata && *var.eff_sqlind != sqlNull)
					memcpy(var.eff_sqldata, var.sqldata, var.sqllen);
			}
		}
	}

	return overrideFlag;
}

void Sqlda::mapSqlAttributes( IscStatement *stmt )
{
	if( buffer.size() != lengthBufferRows )
		throw SQLEXCEPTION (RUNTIME_ERROR, "Sqlda::mapSqlAttributes(): incorrect buffer size");
	if( !meta )
		throw SQLEXCEPTION (RUNTIME_ERROR, "Sqlda::mapSqlAttributes(): no metadata");
	if( !sqlvar.empty() && sqlvar.size() != columnsCount )
		throw SQLEXCEPTION(RUNTIME_ERROR, "Sqlda::mapSqlAttributes(): incorrect columnsCount");

	if (sqlvar.empty()) {
		sqlvar.resize(columnsCount);
	}
	else {
		std::fill(sqlvar.begin(), sqlvar.end(), CAttrSqlVar{});
	}

	ThrowStatusWrapper status( connection->GDS->_status );
	try
	{
		for( unsigned n = 0; n < columnsCount; ++n )
		{
			auto * var = &sqlvar.at(n);

			var->assign( status, meta, buffer, n );

			//arrays
			if( stmt && var->sqltype == SQL_ARRAY ) {
				var->array = new CAttrArray;
				/* 
				* loadAttributes() raises an SQLError exc on empty relname / sqlname
				* Empty names are common case for input parameters.
				* But it seems to me, we have no need for var->array attributes for inputs.
				*/
				if (*var->relname && *var->sqlname)
				{
					var->array->loadAttributes(stmt, var->relname, var->sqlname, var->sqlsubtype);
				}
			}
		}
	}
	catch( const FbException& error )
	{
		THROW_ISC_EXCEPTION ( connection, error.getStatus() );
	}
}

Sqlda::buffer_t& Sqlda::initStaticCursor(IscStatement *stmt)
{
	if ( dataStaticCursor )
		delete 	dataStaticCursor;

	dataStaticCursor = new CDataStaticCursor( stmt, buffer, sqlvar, columnsCount, lengthBufferRows );
	return *dataStaticCursor->itCurrentRow;
}

Sqlda::buffer_t& Sqlda::addRowSqldaInBufferStaticCursor()
{
	return dataStaticCursor->addRow();
}

void Sqlda::restoreOrgAdressFieldsStaticCursor()
{
	dataStaticCursor->restoreOriginalAdressFieldsSqlDa();
}

bool Sqlda::setCurrentRowInBufferStaticCursor(int nRow)
{
	return dataStaticCursor->current(nRow);
}

void Sqlda::getAdressFieldFromCurrentRowInBufferStaticCursor(int column, char *& sqldata, short *& sqlind)
{
	dataStaticCursor->getAdressFieldFromCurrentRowInBufferStaticCursor(column, sqldata, sqlind);
}

void Sqlda::copyNextSqldaInBufferStaticCursor()
{
	*dataStaticCursor << buffer;
}

void Sqlda::copyNextSqldaFromBufferStaticCursor()
{
	*dataStaticCursor >> buffer;
}

void Sqlda::saveCurrentSqldaToBuffer()
{
	dataStaticCursor->copyToBuffer( buffer );
}

void Sqlda::restoreBufferToCurrentSqlda()
{
	dataStaticCursor->copyToCurrentSqlda( buffer );
}

int Sqlda::getCountRowsStaticCursor()
{
	return dataStaticCursor->getCountRowsStaticCursor();
}

int Sqlda::getColumnCount()
{
	return columnsCount;
}

void Sqlda::print()
{
	for (unsigned n = 0; n < columnsCount; ++n)
	{
		auto * var = &sqlvar.at( n );
		char *p = var->sqldata;

		printf ("%d. type %d, len %d, addr %p (%p) ",
				n, var->sqltype, var->sqllen, p, var->sqlind);

		if ( *var->sqlind == sqlNull )
			printf ("<null>");
		else
			switch (var->sqltype)
				{
				case SQL_TEXT:
					printf ("'%.*s'", var->sqllen, p);
					break;

				case SQL_VARYING:
					printf ("'%.*s'", *(short*) p, p + 2);
					break;

				case SQL_BOOLEAN:
					printf ("%d", *(TYPE_BOOLEAN*) p);
					break;

				case SQL_SHORT:
					printf ("%d", *(short*) p);
					break;

				case SQL_LONG:
					printf ("%ld", *(int*) p);
					break;

				case SQL_FLOAT:
					printf ("%g", *(float*) p);
					break;

				case SQL_D_FLOAT:
				case SQL_DOUBLE:
					printf ("%g", *(double*) p);
					break;

				case SQL_QUAD:
				case SQL_INT64:
					printf ("big");
					break;

				case SQL_BLOB:
					printf ("blob");
					break;

				case SQL_TIMESTAMP:
					printf ("timestamp");
					break;

				case SQL_TYPE_TIME:
					printf ("time");
					break;

				case SQL_TYPE_DATE:
					printf ("date");
					break;

				case SQL_ARRAY:
					printf ("array");
					break;
				}
		printf ("\n");
	}
}

// Warning!
// It's hack, for exclude system filed description
// Return SQLDA for all system field has 31 length 
// and charsetId 3 (UNICODE_FSS) it's error!
//
//	if ( !(var->sqllen % getCharsetSize( var->sqlsubtype )) )
//		return var->sqllen / getCharsetSize( var->sqlsubtype );
//
int Sqlda::getColumnDisplaySize(int index)
{
	const SqlProperties *var = (SqldaDir == SQLDA_INPUT) ? orgVarSqlProperties(index) : Var(index);

	switch (var->sqltype)
	{
	case SQL_TEXT:
		if ( var->sqllen == 1 && var->sqlsubtype == 1 )
			return MAX_TINYINT_LENGTH + 1;
		if ( !(var->sqllen % getCharsetSize( var->sqlsubtype )) )
			return var->sqllen / getCharsetSize( var->sqlsubtype );
		return var->sqllen;

	case SQL_BOOLEAN:
		return MAX_BOOLEAN_LENGTH;

	case SQL_SHORT:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_SHORT_LENGTH + 2,
										MAX_DECIMAL_SHORT_LENGTH + 2,
										MAX_SMALLINT_LENGTH + 1);
		
	case SQL_LONG:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_LONG_LENGTH + 2,
										MAX_DECIMAL_LONG_LENGTH + 2,
										MAX_INT_LENGTH + 1);

	case SQL_FLOAT:
		return MAX_FLOAT_LENGTH + 4;			

	case SQL_D_FLOAT:
	case SQL_DOUBLE:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_DOUBLE_LENGTH + 2,
										MAX_DECIMAL_DOUBLE_LENGTH + 2,
										MAX_DOUBLE_LENGTH + 4);

	case SQL_QUAD:
	case SQL_INT64:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_LENGTH + 2,
										MAX_DECIMAL_LENGTH + 2,
										MAX_QUAD_LENGTH + 1);
		
	case SQL_ARRAY:
		return Var(index)->array->arrOctetLength;
//		return MAX_ARRAY_LENGTH;

	case SQL_BLOB:
		return MAX_BLOB_LENGTH;

	case SQL_TYPE_TIME:
		return MAX_TIME_LENGTH;

	case SQL_TYPE_DATE:
		return MAX_DATE_LENGTH;

	case SQL_TIMESTAMP:
		return MAX_TIMESTAMP_LENGTH;
	}

	if ( !(var->sqllen % getCharsetSize( var->sqlsubtype )) )
		return var->sqllen / getCharsetSize( var->sqlsubtype );

	return var->sqllen;
}

const char* Sqlda::getColumnLabel(int index)
{
	auto * var = Var(index);
	return ( var->aliasname && *var->aliasname ) ? var->aliasname : var->sqlname;
}

const char* Sqlda::getColumnName(int index)
{
	return Var(index)->sqlname;
}

int Sqlda::getPrecision(int index)
{
	CAttrSqlVar *var = Var(index);

	switch (var->sqltype)
	{
	case SQL_TEXT:
		if ( var->sqllen == 1 && var->sqlsubtype == 1 )
			return MAX_TINYINT_LENGTH;
		if ( !(var->sqllen % getCharsetSize( var->sqlsubtype )) )
			return var->sqllen / getCharsetSize( var->sqlsubtype );
		return var->sqllen;

	case SQL_BOOLEAN:
		return MAX_BOOLEAN_LENGTH;

	case SQL_SHORT:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_SHORT_LENGTH,
										MAX_DECIMAL_SHORT_LENGTH,
										MAX_SMALLINT_LENGTH);

	case SQL_LONG:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_LONG_LENGTH,
										MAX_DECIMAL_LONG_LENGTH,
										MAX_INT_LENGTH);

	case SQL_FLOAT:
		return MAX_FLOAT_LENGTH;

	case SQL_D_FLOAT:
	case SQL_DOUBLE:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_DOUBLE_LENGTH,
										MAX_DECIMAL_DOUBLE_LENGTH,
										MAX_DOUBLE_LENGTH);

	case SQL_QUAD:
	case SQL_INT64:
		return SET_INFO_FROM_SUBTYPE(	MAX_NUMERIC_LENGTH,
										MAX_DECIMAL_LENGTH,
										MAX_QUAD_LENGTH);

	case SQL_ARRAY:	
		return var->array->arrOctetLength;
//		return MAX_ARRAY_LENGTH;
	
	case SQL_BLOB:		
		return MAX_BLOB_LENGTH;

	case SQL_TYPE_TIME:
		return MAX_TIME_LENGTH;

	case SQL_TYPE_DATE:
		return MAX_DATE_LENGTH;

	case SQL_TIMESTAMP:
		return MAX_TIMESTAMP_LENGTH;
	}

	if ( !(var->sqllen % getCharsetSize( var->sqlsubtype )) )
		return var->sqllen / getCharsetSize( var->sqlsubtype );

	return var->sqllen;
}

int Sqlda::getNumPrecRadix(int index)
{
	CAttrSqlVar *var = Var(index);

	switch (var->sqltype)
	{
	case SQL_SHORT:
	case SQL_LONG:
	case SQL_QUAD:
	case SQL_INT64:
		return 10;
	case SQL_FLOAT:
	case SQL_DOUBLE:
	case SQL_D_FLOAT:
		return 2;
	}

	return 0;
}

int Sqlda::getScale(int index)
{
	CAttrSqlVar *var = Var(index);

	switch (var->sqltype)
	{
	case SQL_TIMESTAMP:
	case SQL_TYPE_TIME:
		return ISC_TIME_SECONDS_PRECISION_SCALE;
	}

	return var->sqlscale;
}

bool Sqlda::isNullable(int index)
{
	return Var( index )->isNullable;
}

int Sqlda::getColumnType(int index, int &realSqlType)
{
	return getSqlType ( Var(index), realSqlType );
}

const char* Sqlda::getColumnTypeName(int index)
{
	return getSqlTypeName ( Var(index) );
}

short Sqlda::getSubType(int index)
{
	return Var( index )->sqlsubtype;
}

int Sqlda::getSqlType(CAttrSqlVar *var, int &realSqlType)
{
	switch (var->sqltype)
	{
	case SQL_TEXT:
		if ( var->sqllen == 1 && var->sqlsubtype == 1 )
			return (realSqlType = JDBC_TINYINT);
		else if ( var->sqllen == 16 && var->sqlsubtype == 1 )
			return (realSqlType = JDBC_GUID);
		else if ( ( var->sqlsubtype == 3 // UNICODE_FSS
				    || var->sqlsubtype == 4 ) // UTF8
			&& !(var->sqllen % getCharsetSize( var->sqlsubtype )) )
			return (realSqlType = JDBC_WCHAR);
		return (realSqlType = JDBC_CHAR);

	case SQL_VARYING:
		if ( ( var->sqlsubtype == 3 // UNICODE_FSS
				    || var->sqlsubtype == 4 ) // UTF8
			&& !(var->sqllen % getCharsetSize( var->sqlsubtype )) )
			return (realSqlType = JDBC_WVARCHAR);
		return (realSqlType = JDBC_VARCHAR);

	case SQL_BOOLEAN:
		return (realSqlType = JDBC_BOOLEAN);

	case SQL_SHORT:
		realSqlType = JDBC_SMALLINT;
		return SET_INFO_FROM_SUBTYPE ( JDBC_NUMERIC, JDBC_DECIMAL, realSqlType);

	case SQL_LONG:
		realSqlType = JDBC_INTEGER;
		return SET_INFO_FROM_SUBTYPE ( JDBC_NUMERIC, JDBC_DECIMAL, realSqlType);

	case SQL_FLOAT:
		return (realSqlType = JDBC_REAL);

	case SQL_DOUBLE:
		realSqlType = JDBC_DOUBLE;
		return SET_INFO_FROM_SUBTYPE ( JDBC_NUMERIC, JDBC_DECIMAL, realSqlType);

	case SQL_QUAD:
		return JDBC_BIGINT;

	case SQL_INT64:
		realSqlType = JDBC_BIGINT;
		return SET_INFO_FROM_SUBTYPE ( JDBC_NUMERIC, JDBC_DECIMAL, realSqlType);

	case SQL_BLOB:
		if (var->sqlsubtype == 1)
			return (realSqlType = JDBC_LONGVARCHAR);
		return (realSqlType = JDBC_LONGVARBINARY);

	case SQL_TIMESTAMP:
		return (realSqlType = JDBC_TIMESTAMP);

	case SQL_TYPE_TIME:
		return (realSqlType = JDBC_TIME);

	case SQL_TYPE_DATE:
		return (realSqlType = JDBC_DATE);

	case SQL_ARRAY:
		if ( var->array->arrOctetLength < MAX_VARCHAR_LENGTH )
			return (realSqlType = JDBC_VARCHAR);
		return (realSqlType = JDBC_LONGVARCHAR);
	}

	return (realSqlType = 0);
}

const char* Sqlda::getSqlTypeName ( CAttrSqlVar *var )
{
	switch (var->sqltype)
	{
	case SQL_TEXT:
		if ( var->sqllen == 1 && var->sqlsubtype == 1 )
			return "TINYINT";
		else if ( var->sqllen == 16 && var->sqlsubtype == 1 )
			return "GUID";
		return "CHAR";

	case SQL_VARYING:
		return "VARCHAR";

	case SQL_BOOLEAN:
		return "BOOLEAN";

	case SQL_SHORT:
		return SET_INFO_FROM_SUBTYPE ( "NUMERIC", "DECIMAL", "SMALLINT");

	case SQL_LONG:
		return SET_INFO_FROM_SUBTYPE ( "NUMERIC", "DECIMAL", "INTEGER");

	case SQL_FLOAT:
		return "FLOAT";

	case SQL_D_FLOAT:
	case SQL_DOUBLE:
		return SET_INFO_FROM_SUBTYPE ( "NUMERIC", "DECIMAL", "DOUBLE PRECISION");

	case SQL_QUAD:
		return "BIGINT";

	case SQL_INT64:
		return SET_INFO_FROM_SUBTYPE ( "NUMERIC", "DECIMAL", "BIGINT");

	case SQL_BLOB:
		if ( var->sqlsubtype == 1 )
			return "BLOB SUB_TYPE TEXT";
		return "BLOB SUB_TYPE 0";

	case SQL_TIMESTAMP:
		return "TIMESTAMP";

	case SQL_TYPE_TIME:
		return "TIME";

	case SQL_TYPE_DATE:
		return "DATE";

	case SQL_ARRAY:
		return "ARRAY";

	default:
		NOT_YET_IMPLEMENTED;
	}

	return "*unknown type*";
}

const char* Sqlda::getTableName(int index)
{
	return Var( index )->relname;
}

void Sqlda::setValue(int slot, Value * value, IscStatement	*stmt)
{
	const auto index = slot + 1;

	auto * var = Var( index );

	// Check to see if we need to do the conversion.  Otherwise, the
	// InterBase do it.

	switch (var->sqltype)
		{
		case SQL_BLOB:	
			setBlob (var, value, stmt);
			return;
		case SQL_ARRAY:	
			setArray (var, value, stmt);
			return;
		}

	// 10.3.3: Only write sqlscale if changed. Avoids cache-line dirtying and
	// prevents propertiesOverriden() from returning true on re-execute with same types.
	if (var->sqlscale != 0)
		var->sqlscale = 0;
	
	if( value->type == Null ) {
		setNull( index );
		return;
	}
	setNotNull( index );

	char * src_buf = (char*) &value->data;
	char * dst_buf = var->sqldata;

	// 10.3.3: Helper to avoid redundant writes to sqltype/sqllen.
	// Only modifies var members when the new value differs, preventing
	// propertiesOverriden() from detecting false changes on re-execute.
	auto setTypeAndLen = [](CAttrSqlVar* v, short type, unsigned len) {
		if (v->sqltype != type) v->sqltype = type;
		if (v->sqllen != len) v->sqllen = len;
	};

	switch (value->type)
		{
		case String:
			setTypeAndLen(var, SQL_TEXT, value->data.string.length);
			memcpy( dst_buf, value->data.string.string, var->sqllen );
			break;

		case Boolean:
			setTypeAndLen(var, SQL_BOOLEAN, sizeof(TYPE_BOOLEAN));
			memcpy( dst_buf, src_buf, var->sqllen );
			break;

		case Short:
			setTypeAndLen(var, SQL_SHORT, sizeof(short));
			memcpy( dst_buf, src_buf, var->sqllen );
			break;

		case Long:
			setTypeAndLen(var, SQL_LONG, sizeof(int));
			memcpy( dst_buf, src_buf, var->sqllen );
			break;

		case Quad:
			setTypeAndLen(var, SQL_INT64, sizeof(QUAD));
			memcpy( dst_buf, src_buf, var->sqllen );
			break;

		case Float:
			setTypeAndLen(var, SQL_FLOAT, sizeof(float));
			memcpy( dst_buf, src_buf, var->sqllen );
			break;

		case Double:
			setTypeAndLen(var, SQL_DOUBLE, sizeof(double));
			memcpy( dst_buf, src_buf, var->sqllen );
			break;

		case Date:
			setTypeAndLen(var, SQL_TYPE_DATE, sizeof(ISC_DATE));
			*(ISC_DATE*)dst_buf = IscStatement::getIscDate (value->data.date);
			break;
									
		case TimeType:
			setTypeAndLen(var, SQL_TYPE_TIME, sizeof(ISC_TIME));
			*(ISC_TIME*)dst_buf = IscStatement::getIscTime (value->data.time);
			break;
									
		case Timestamp:
			setTypeAndLen(var, SQL_TIMESTAMP, sizeof(ISC_TIMESTAMP));
			*(ISC_TIMESTAMP*)dst_buf = IscStatement::getIscTimeStamp (value->data.timestamp);
			break;
									
		default:
			NOT_YET_IMPLEMENTED;
		}			

}

void Sqlda::setBlob(CAttrSqlVar* var, Value * value, IscStatement *stmt)
{
	if (value->type == Null || !stmt)
	{
		setNull( var->index );
		return;
	}

	IscConnection * connection = stmt->connection;
	CFbDll * GDS = connection->GDS;
	IBlob* blobHandle = NULL;
	ITransaction* transactionHandle = stmt->startTransaction();

	ThrowStatusWrapper status( connection->GDS->_status );
	try
	{
		blobHandle = connection->databaseHandle->createBlob( &status, transactionHandle, (ISC_QUAD*) var->sqldata, 0, NULL );
	}
	catch( const FbException& error )
	{
		THROW_ISC_EXCEPTION ( connection, error.getStatus() );
	}

	char *address = buffer.data() + var->offsetData;
	int length = 0;

	switch (value->type)
	{
	case String:
		address = value->data.string.string;
		length = value->data.string.length;
		break;

	case Short:
		length = sizeof (short);
		break;

	case Long:
		length = sizeof (int);
		break;

	case Quad:
		length = sizeof (QUAD);
		break;

	case Float:
		length = sizeof (float);
		break;

	case Double:
		length = sizeof (double);
		break;

	case BlobPtr:
		{
		length = 0;
		Blob *blob = value->data.blob;
		try
		{
			for (int len, offset = 0; len = blob->getSegmentLength (offset); offset += len)
			{
				blobHandle->putSegment( &status, len, (char*) blob->getSegment (offset) );
			}
		}
		catch( const FbException& error )
		{
			THROW_ISC_EXCEPTION ( connection, error.getStatus() );
		}
		}
		break;

	case Date:
								
	default:
		NOT_YET_IMPLEMENTED;
	}			

	if ( length )
	{
		int post = DEFAULT_BLOB_BUFFER_LENGTH;

		try
		{
			while ( length > post )
			{
				blobHandle->putSegment( &status, post, address );
				address+=post;
				length-=post;
			}
		}
		catch( const FbException& error )
		{
			THROW_ISC_EXCEPTION ( connection, error.getStatus() );
		}

		if( length > 0 )
		try
		{
			blobHandle->putSegment( &status, length, address );
		}
		catch( const FbException& error )
		{
			THROW_ISC_EXCEPTION ( connection, error.getStatus() );
		}
	}

	try
	{
		blobHandle->close( &status );
		blobHandle = nullptr;
	}
	catch( const FbException& )
	{
		if( blobHandle ) blobHandle->release();
	}
}

void WriteToArray(IscConnection *connect,XSQLVAR *var,Value * value);

void Sqlda::setArray(CAttrSqlVar* var, Value *value, IscStatement *stmt)
{
	if (value->type == Null || !stmt)
	{
		setNull( var->index );
		return;
	}

	char* buf = var->sqldata;
	IscArray arr( stmt, var );
	arr.writeArray(value);
	*(ISC_QUAD*)buf = *arr.arrayId;
}

int Sqlda::findColumn(const char * columnName)
{
	for( const auto & var : sqlvar )
		if (strcasecmp (var.sqlname, columnName) == 0)
			return var.index - 1;

	return -1;
}

const char* Sqlda::getOwnerName(int index)
{
//	XSQLVAR *var = Var(index);
//	return var->ownname;
//	ATTENTION! Fb can not at present execute a design
//	INSERT INTO "OWNER"."EVCT" ("CNT") VALUES (?)
	return "";
}

int Sqlda::isBlobOrArray(int index)
{
	int type = Var(index)->sqltype;

	switch (type)
	{
	case SQL_BLOB:		// 520
	case SQL_ARRAY:		// 540
		return type;
	}

	return 0;
}

bool Sqlda::isNull(int index)
{
	return *( Var( index )->sqlind ) == sqlNull;
}

void Sqlda::setNull(int index)
{
	*( Var( index )->sqlind ) = sqlNull;
}

void Sqlda::setNotNull(int index)
{
	*( Var( index )->sqlind ) = 0;
}

bool Sqlda::getBoolean (int index)
{
	CONVERSION_CHECK_DEBUG((Var(index)->sqltype) == SQL_BOOLEAN);
	if ( isNull ( index ) )
		return 0;
	return !!*(TYPE_BOOLEAN*)( Var(index)->sqldata );
}

short Sqlda::getShort (int index)
{
	CONVERSION_CHECK_DEBUG((Var(index)->sqltype) == SQL_SHORT);
	if ( isNull ( index) )
		return 0;
	return *(short*)( Var(index)->sqldata );
}

int Sqlda::getInt (int index)
{
	CONVERSION_CHECK_DEBUG((Var(index)->sqltype) == SQL_LONG);
	if ( isNull ( index) )
		return 0;
	return *(int*)( Var(index)->sqldata );
}

char * Sqlda::getText (int index, int &len)
{
	CONVERSION_CHECK_DEBUG((Var(index)->sqltype) == SQL_TEXT);
	if( isNull ( index) )
	{
		len = 0;
		return "";
	}
	len = Var(index)->sqllen;
	return Var(index)->sqldata;
}

char * Sqlda::getVarying (int index, int &len)
{
	CONVERSION_CHECK_DEBUG((Var(index)->sqltype) == SQL_VARYING);
	if( isNull ( index) )
	{
		len = 0;
		return "";
	}
	auto * buf = Var(index)->sqldata;
	len = *(short*)buf;
	return  buf + 2;
}

////////////////////////////////////////////////////////

void Sqlda::updateBoolean (int index, bool value)
{
	CONVERSION_CHECK_DEBUG((Var(index)->sqltype) == SQL_BOOLEAN);
	*(TYPE_BOOLEAN*)( Var(index)->sqldata ) = value;
	setNotNull( index );
}

void Sqlda::updateShort (int index, short value)
{
	CONVERSION_CHECK_DEBUG((Var(index)->sqltype) == SQL_SHORT);
	*(short*)( Var(index)->sqldata ) = value;
	setNotNull( index );
}

void Sqlda::updateInt (int index, int value)
{
	CONVERSION_CHECK_DEBUG((Var(index)->sqltype) == SQL_LONG);
	*(int*)( Var(index)->sqldata ) = value;
	setNotNull( index );
}

void Sqlda::updateText (int index, const char* dst)
{
	CONVERSION_CHECK_DEBUG((Var(index)->sqltype) == SQL_TEXT);
    char * src = Var(index)->sqldata;
	int n = Var(index)->sqllen;
	setNotNull( index );

    if ( n < 1)
       return;

    while ( n-- && *dst) //TODO: slow code.
        *src++ = *dst++;

	*src = '\0';
}

void Sqlda::updateVarying (int index, const char* dst)
{
	CONVERSION_CHECK_DEBUG((Var(index)->sqltype) == SQL_VARYING);
	char * buf = Var(index)->sqldata;
    char * src = buf + sizeof(short);
	int n = Var(index)->sqllen;
	setNotNull( index );

    if ( n < 1)
       return;

    while ( n-- && *dst) //TODO: slow code
        *src++ = *dst++;

	*(unsigned short*)buf = (unsigned short)(Var(index)->sqllen - n - 1);
}

}; // end namespace IscDbcLibrary
