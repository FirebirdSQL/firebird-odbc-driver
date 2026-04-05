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
#include "CDataStaticCursor.h"
#include "SQLError.h"
#include "Value.h"
#include "IscConnection.h"
#include "IscStatement.h"
#include "IscBlob.h"

using namespace Firebird;

namespace IscDbcLibrary {

static short sqlNull = -1;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction Sqlda
//////////////////////////////////////////////////////////////////////

Sqlda::Sqlda(IscConnection* conn, e_sqlda_dir dir) : connection{ conn }, buffer{}, execBuffer{}, useExecBufferMeta{ false }, SqldaDir{ dir }
{
	sqlda_init(*this);
}

Sqlda::~Sqlda()
{
	sqlda_remove(*this);
}

void sqlda_init(Sqlda& s)
{
	s.execMeta = s.meta = nullptr;
	s.sqlvar.clear();
	s.dataStaticCursor = nullptr;
	s.columnsCount = 0;
}

void sqlda_remove(Sqlda& s)
{
	delete s.dataStaticCursor;
	s.dataStaticCursor = nullptr;
	sqlda_delete(s);
}

void sqlda_clear(Sqlda& s)
{
	sqlda_remove(s);
	sqlda_init(s);
}

void sqlda_delete(Sqlda& s)
{
	s.sqlvar.clear();
	s.buffer.clear();
	s.execBuffer.clear();
	s.externalBuffer_ = nullptr;
	s.externalBufferSize_ = 0;

	if( s.meta ) {
		s.meta->release();
		s.meta = nullptr;
	}
	if ( s.execMeta ) {
		s.execMeta->release();
		s.execMeta = nullptr;
	}
	s.useExecBufferMeta = false;
}

void sqlda_alloc_buffer(Sqlda& s, IscStatement *stmt, IMessageMetadata* msgMetadata)
{
	if( !msgMetadata )
		throw SQLEXCEPTION (RUNTIME_ERROR, "sqlda_alloc_buffer(): no metadata");

	if( s.meta ) s.meta->release();
	s.meta = msgMetadata;

	if (s.execMeta)
	{
		s.execMeta->release();
		s.execMeta = nullptr;
	}
	s.execBuffer.clear();
	s.useExecBufferMeta = false;

	ThrowStatusWrapper status( s.connection->GDS->_status );
	try
	{
		s.lengthBufferRows = s.meta->getMessageLength( &status );
		s.columnsCount     = s.meta->getCount( &status );
	}
	catch( const FbException& error )
	{
		THROW_ISC_EXCEPTION ( s.connection, error.getStatus() );
	}

	s.buffer.resize( s.lengthBufferRows );
	sqlda_map_sql_attributes( s, stmt );

	// Reset external buffer state on fresh allocBuffer
	s.externalBuffer_ = nullptr;
	s.externalBufferSize_ = 0;
}

void sqlda_remap_to_external_buffer(Sqlda& s, char* externalBuf, size_t externalBufSize)
{
	if (!externalBuf || externalBufSize < static_cast<size_t>(s.lengthBufferRows))
		throw SQLEXCEPTION(RUNTIME_ERROR, "sqlda_remap_to_external_buffer(): invalid external buffer");

	s.externalBuffer_ = externalBuf;
	s.externalBufferSize_ = externalBufSize;

	for (auto& var : s.sqlvar)
		var.assignBuffer(s.externalBuffer_, s.externalBufferSize_);

	// Release the internal buffer — all sqlvar now point to the external one.
	s.buffer.clear();
	s.buffer.shrink_to_fit();
}

/*
*	This function is used to build meta & buffer just before execution,
*	taking into account sqlvar change during bind (Phase 14.4.7.5d).
*/
bool sqlda_check_and_rebuild(Sqlda& sqlda)
{
	const bool overrideFlag = sqlda.isExternalOverriden();

	if (overrideFlag)
	{
		IMetadataBuilder* metaBuilder = nullptr;
		ThrowStatusWrapper status(sqlda.connection->GDS->_status);

		auto build_new_meta = [&]()->IMessageMetadata*
		{
			if (!sqlda.meta)
				throw SQLEXCEPTION(RUNTIME_ERROR, "sqlda_check_and_rebuild(): meta==null before execute!");

			metaBuilder = sqlda.meta->getBuilder(&status);

			for (const auto& var : sqlda.sqlvar)
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
			if (sqlda.execMeta)
			{
				sqlda.execMeta->release();
			}

			sqlda.execMeta = build_new_meta();

			if (sqlda.execMeta->getCount(&status) != sqlda.columnsCount)
			{
				throw SQLEXCEPTION(RUNTIME_ERROR, "sqlda_check_and_rebuild(): incorrect columns count");
			}

			sqlda.lengthBufferRows = sqlda.execMeta->getMessageLength(&status);
			sqlda.execBuffer.clear();
			sqlda.execBuffer.resize(sqlda.lengthBufferRows);

			for (auto& var : sqlda.sqlvar)
			{
				const auto i = var.index - 1;
				//save effective sqldata&sqlind, pointing to the new execBuffer
				var.eff_sqldata = &sqlda.execBuffer.at(sqlda.execMeta->getOffset(&status, i));
				var.eff_sqlind = (short*)&sqlda.execBuffer.at(sqlda.execMeta->getNullOffset(&status, i));
				//update last sqlvars to avoid buffers rebuilt next time
				var.lastSqlProperties = var;
			}

			sqlda.useExecBufferMeta = true;
		}
		catch (const FbException& error)
		{
			if (metaBuilder) metaBuilder->release();
			THROW_ISC_EXCEPTION(sqlda.connection, error.getStatus());
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
	if (sqlda.useExecBufferMeta)
	{
		// 10.3.1: On re-execute (!overrideFlag), the eff pointers are known-different
		// from the original pointers (they were set during a prior rebuild). Skip the
		// per-column pointer comparison and copy unconditionally — the branch prediction
		// savings outweigh the cost of a small unconditional memcpy.
		if (!overrideFlag)
		{
			for (const auto& var : sqlda.sqlvar)
			{
				memcpy(var.eff_sqlind, var.sqlind, sizeof(short));
				if (*var.eff_sqlind != sqlNull)
					memcpy(var.eff_sqldata, var.sqldata, var.sqllen);
			}
		}
		else
		{
			for (const auto& var : sqlda.sqlvar)
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

void sqlda_map_sql_attributes(Sqlda& s, IscStatement *stmt)
{
	if( s.buffer.size() != s.lengthBufferRows )
		throw SQLEXCEPTION (RUNTIME_ERROR, "sqlda_map_sql_attributes(): incorrect buffer size");
	if( !s.meta )
		throw SQLEXCEPTION (RUNTIME_ERROR, "sqlda_map_sql_attributes(): no metadata");
	if( !s.sqlvar.empty() && s.sqlvar.size() != s.columnsCount )
		throw SQLEXCEPTION(RUNTIME_ERROR, "sqlda_map_sql_attributes(): incorrect columnsCount");

	if (s.sqlvar.empty()) {
		s.sqlvar.resize(s.columnsCount);
	}
	else {
		std::fill(s.sqlvar.begin(), s.sqlvar.end(), CAttrSqlVar{});
	}

	ThrowStatusWrapper status( s.connection->GDS->_status );
	try
	{
		for( unsigned n = 0; n < s.columnsCount; ++n )
		{
			auto * var = &s.sqlvar.at(n);

			var->assign( status, s.meta, s.buffer, n );

			//arrays
			if( stmt && var->sqltype == SQL_ARRAY ) {
				var->array = new CAttrArray;
				if (*var->relname && *var->sqlname)
				{
					var->array->loadAttributes(stmt, var->relname, var->sqlname, var->sqlsubtype);
				}
			}
		}
	}
	catch( const FbException& error )
	{
		THROW_ISC_EXCEPTION ( s.connection, error.getStatus() );
	}
}

Sqlda::buffer_t& sqlda_init_static_cursor(Sqlda& s, IscStatement *stmt)
{
	if ( s.dataStaticCursor )
		delete 	s.dataStaticCursor;

	// Phase 14.4.7.2c: If the output sqlvar was remapped to an external buffer
	// (e.g., fbcpp::outMessage), the internal buffer was released.  Static
	// cursor needs the internal buffer for row staging, so re-allocate it and
	// restore sqlvar pointers to the internal buffer.
	if (s.externalBuffer_)
	{
		s.buffer.resize(s.lengthBufferRows);
		for (auto& var : s.sqlvar)
			var.assignBuffer(s.buffer);
		s.externalBuffer_ = nullptr;
		s.externalBufferSize_ = 0;
	}

	s.dataStaticCursor = new CDataStaticCursor( stmt, s.buffer, s.sqlvar, s.columnsCount, s.lengthBufferRows );
	return *s.dataStaticCursor->itCurrentRow;
}

Sqlda::buffer_t& sqlda_add_row_static_cursor(Sqlda& s)
{
	return s.dataStaticCursor->addRow();
}

void sqlda_restore_org_address_fields_static_cursor(Sqlda& s)
{
	s.dataStaticCursor->restoreOriginalAdressFieldsSqlDa();
}

bool sqlda_set_current_row_static_cursor(Sqlda& s, int nRow)
{
	return s.dataStaticCursor->current(nRow);
}

void sqlda_get_address_field_static_cursor(Sqlda& s, int column, char *& sqldata, short *& sqlind)
{
	s.dataStaticCursor->getAdressFieldFromCurrentRowInBufferStaticCursor(column, sqldata, sqlind);
}

void sqlda_copy_next_in_buffer_static_cursor(Sqlda& s)
{
	*s.dataStaticCursor << s.buffer;
}

void sqlda_copy_next_from_buffer_static_cursor(Sqlda& s)
{
	*s.dataStaticCursor >> s.buffer;
}

void sqlda_save_current_to_buffer(Sqlda& s)
{
	s.dataStaticCursor->copyToBuffer( s.buffer );
}

void sqlda_restore_buffer_to_current(Sqlda& s)
{
	s.dataStaticCursor->copyToCurrentSqlda( s.buffer );
}

int sqlda_get_count_rows_static_cursor(Sqlda& s)
{
	return s.dataStaticCursor->getCountRowsStaticCursor();
}

void sqlda_print(Sqlda& s)
{
	for (unsigned n = 0; n < s.columnsCount; ++n)
	{
		auto * var = &s.sqlvar.at( n );
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

const char* sqlda_get_table_name(Sqlda& s, int index)
{
	return s.Var( index )->relname;
}

void sqlda_set_value(Sqlda& s, int slot, Value * value, IscStatement *stmt)
{
	const auto index = slot + 1;

	auto * var = s.Var( index );

	switch (var->sqltype)
		{
		case SQL_BLOB:	
			sqlda_set_blob(s, var, value, stmt);
			return;
		case SQL_ARRAY:	
			sqlda_set_array(s, var, value, stmt);
			return;
		}

	if (var->sqlscale != 0)
		var->sqlscale = 0;
	
	if( value->type == Null ) {
		sqlda_set_null(s, index);
		return;
	}
	sqlda_set_not_null(s, index);

	char * src_buf = (char*) &value->data;
	char * dst_buf = var->sqldata;

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

void sqlda_set_blob(Sqlda& s, CAttrSqlVar* var, Value * value, IscStatement *stmt)
{
	if (value->type == Null || !stmt)
	{
		sqlda_set_null(s, var->index);
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

	char *address = s.buffer.data() + var->offsetData;
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

void sqlda_set_array(Sqlda& s, CAttrSqlVar* var, Value *value, IscStatement *stmt)
{
	if (value->type == Null || !stmt)
	{
		sqlda_set_null(s, var->index);
		return;
	}

	char* buf = var->sqldata;
	IscArray arr( stmt, var );
	arr.writeArray(value);
	*(ISC_QUAD*)buf = *arr.arrayId;
}

int sqlda_find_column(Sqlda& s, const char * columnName)
{
	for( const auto & var : s.sqlvar )
		if (strcasecmp (var.sqlname, columnName) == 0)
			return var.index - 1;

	return -1;
}

const char* sqlda_get_owner_name(Sqlda& s, int index)
{
	return "";
}

int sqlda_is_blob_or_array(Sqlda& s, int index)
{
	int type = s.Var(index)->sqltype;

	switch (type)
	{
	case SQL_BLOB:		// 520
	case SQL_ARRAY:		// 540
		return type;
	}

	return 0;
}

bool sqlda_is_null(Sqlda& s, int index)
{
	return *( s.Var( index )->sqlind ) == sqlNull;
}

void sqlda_set_null(Sqlda& s, int index)
{
	*( s.Var( index )->sqlind ) = sqlNull;
}

void sqlda_set_not_null(Sqlda& s, int index)
{
	*( s.Var( index )->sqlind ) = 0;
}

bool sqlda_get_boolean(Sqlda& s, int index)
{
	CONVERSION_CHECK_DEBUG((s.Var(index)->sqltype) == SQL_BOOLEAN);
	if ( sqlda_is_null(s, index) )
		return 0;
	return !!*(TYPE_BOOLEAN*)( s.Var(index)->sqldata );
}

short sqlda_get_short(Sqlda& s, int index)
{
	CONVERSION_CHECK_DEBUG((s.Var(index)->sqltype) == SQL_SHORT);
	if ( sqlda_is_null(s, index) )
		return 0;
	return *(short*)( s.Var(index)->sqldata );
}

int sqlda_get_int(Sqlda& s, int index)
{
	CONVERSION_CHECK_DEBUG((s.Var(index)->sqltype) == SQL_LONG);
	if ( sqlda_is_null(s, index) )
		return 0;
	return *(int*)( s.Var(index)->sqldata );
}

char * sqlda_get_text(Sqlda& s, int index, int &len)
{
	CONVERSION_CHECK_DEBUG((s.Var(index)->sqltype) == SQL_TEXT);
	if( sqlda_is_null(s, index) )
	{
		len = 0;
		return "";
	}
	len = s.Var(index)->sqllen;
	return s.Var(index)->sqldata;
}

char * sqlda_get_varying(Sqlda& s, int index, int &len)
{
	CONVERSION_CHECK_DEBUG((s.Var(index)->sqltype) == SQL_VARYING);
	if( sqlda_is_null(s, index) )
	{
		len = 0;
		return "";
	}
	auto * buf = s.Var(index)->sqldata;
	len = *(short*)buf;
	return  buf + 2;
}

////////////////////////////////////////////////////////

void sqlda_update_boolean(Sqlda& s, int index, bool value)
{
	CONVERSION_CHECK_DEBUG((s.Var(index)->sqltype) == SQL_BOOLEAN);
	*(TYPE_BOOLEAN*)( s.Var(index)->sqldata ) = value;
	sqlda_set_not_null(s, index);
}

void sqlda_update_short(Sqlda& s, int index, short value)
{
	CONVERSION_CHECK_DEBUG((s.Var(index)->sqltype) == SQL_SHORT);
	*(short*)( s.Var(index)->sqldata ) = value;
	sqlda_set_not_null(s, index);
}

void sqlda_update_int(Sqlda& s, int index, int value)
{
	CONVERSION_CHECK_DEBUG((s.Var(index)->sqltype) == SQL_LONG);
	*(int*)( s.Var(index)->sqldata ) = value;
	sqlda_set_not_null(s, index);
}

void sqlda_update_text(Sqlda& s, int index, const char* dst)
{
	CONVERSION_CHECK_DEBUG((s.Var(index)->sqltype) == SQL_TEXT);
    char * src = s.Var(index)->sqldata;
	int n = s.Var(index)->sqllen;
	sqlda_set_not_null(s, index);

    if ( n < 1)
       return;

    while ( n-- && *dst)
        *src++ = *dst++;

	*src = '\0';
}

void sqlda_update_varying(Sqlda& s, int index, const char* dst)
{
	CONVERSION_CHECK_DEBUG((s.Var(index)->sqltype) == SQL_VARYING);
	char * buf = s.Var(index)->sqldata;
    char * src = buf + sizeof(short);
	int n = s.Var(index)->sqllen;
	sqlda_set_not_null(s, index);

    if ( n < 1)
       return;

    while ( n-- && *dst)
        *src++ = *dst++;

	*(unsigned short*)buf = (unsigned short)(s.Var(index)->sqllen - n - 1);
}

}; // end namespace IscDbcLibrary
