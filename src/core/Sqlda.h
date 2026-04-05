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

// Sqlda.h: Sqlda data container and free-function interface.
//
// Phase 14.4.7.5c: CAttrSqlVar/SqlProperties/AlignedAllocator extracted
// to CAttrSqlVar.h. All methods converted to free functions except trivial
// inline accessors.
//////////////////////////////////////////////////////////////////////

#if !defined(_SQLDA_H_INCLUDED_)
#define _SQLDA_H_INCLUDED_

#include "CAttrSqlVar.h"
#include <sqltypes.h>

namespace IscDbcLibrary {

class Value;
class IscConnection;
class IscStatement;
class CDataStaticCursor;

/// Sqlda — Firebird message buffer container for input parameters or output columns.
/// After Phase 14.4.7.5c this is a plain data struct; all behavior lives in free functions.
struct Sqlda
{
	using buffer_t = CAttrSqlVar::buffer_t;
	using orgsqlvar_t = std::vector<CAttrSqlVar>;

	enum e_sqlda_dir {
		SQLDA_INPUT,
		SQLDA_OUTPUT,
	};

	Sqlda(IscConnection* conn, e_sqlda_dir dir);
	~Sqlda();

	// Trivial inline accessors
	CAttrSqlVar* Var(int index) { return &sqlvar.at(index - 1); }
	const SqlProperties* orgVarSqlProperties(int index) { return &sqlvar.at(index - 1).orgSqlProperties; }

	const SqlProperties* effectiveVarProperties(int index) {
		return (SqldaDir == SQLDA_INPUT) ? orgVarSqlProperties(index) : Var(index);
	}

	int getColumnCount() const { return columnsCount; }

	char* activeBufferData() { return externalBuffer_ ? externalBuffer_ : buffer.data(); }
	size_t activeBufferSize() const { return externalBuffer_ ? externalBufferSize_ : buffer.size(); }

	bool isExternalOverriden() {
		for (auto& var : sqlvar) if (var.propertiesOverriden()) return true;
		return false;
	}

	// Data members
	e_sqlda_dir SqldaDir;
	IscConnection* connection;
	CDataStaticCursor* dataStaticCursor;
	Firebird::IMessageMetadata* meta;
	Firebird::IMessageMetadata* execMeta;
	buffer_t buffer;
	buffer_t execBuffer;
	bool useExecBufferMeta;
	int	lengthBufferRows;
	char* externalBuffer_;
	size_t externalBufferSize_;
	orgsqlvar_t sqlvar;
	unsigned columnsCount;

	// ─── Inline wrappers (defined after free-function declarations) ──
	inline void clearSqlda();
	inline void deleteSqlda();
	inline void allocBuffer(IscStatement* st, Firebird::IMessageMetadata* m);
	inline void remapToExternalBuffer(char* b, size_t sz);

	inline buffer_t& initStaticCursor(IscStatement* st);
	inline buffer_t& addRowSqldaInBufferStaticCursor();
	inline void restoreOrgAdressFieldsStaticCursor();
	inline bool setCurrentRowInBufferStaticCursor(int r);
	inline void getAdressFieldFromCurrentRowInBufferStaticCursor(int c, char*& d, short*& i);
	inline void copyNextSqldaInBufferStaticCursor();
	inline void copyNextSqldaFromBufferStaticCursor();
	inline void saveCurrentSqldaToBuffer();
	inline void restoreBufferToCurrentSqlda();
	inline int  getCountRowsStaticCursor();

	inline void setValue(int slot, Value* v, IscStatement* st);
	inline int  findColumn(const char* name);
	inline const char* getTableName(int idx);
	inline const char* getOwnerName(int idx);
	inline int  isBlobOrArray(int idx);
	inline bool isNull(int idx);
	inline void setNull(int idx);
	inline void setNotNull(int idx);

	inline bool  getBoolean(int idx);
	inline short getShort(int idx);
	inline int   getInt(int idx);
	inline char* getText(int idx, int& len);
	inline char* getVarying(int idx, int& len);

	inline void updateBoolean(int idx, bool v);
	inline void updateShort(int idx, short v);
	inline void updateInt(int idx, int v);
	inline void updateText(int idx, const char* v);
	inline void updateVarying(int idx, const char* v);

	inline void print();
};

// ─── Lifecycle free functions ────────────────────────────────────────
void sqlda_init(Sqlda& s);
void sqlda_remove(Sqlda& s);
void sqlda_clear(Sqlda& s);
void sqlda_delete(Sqlda& s);

// ─── Buffer management free functions ────────────────────────────────
void sqlda_alloc_buffer(Sqlda& s, IscStatement* stmt, Firebird::IMessageMetadata* msgMetadata);
void sqlda_remap_to_external_buffer(Sqlda& s, char* externalBuf, size_t externalBufSize);
void sqlda_map_sql_attributes(Sqlda& s, IscStatement* stmt);

// ─── Static cursor free functions ────────────────────────────────────
Sqlda::buffer_t& sqlda_init_static_cursor(Sqlda& s, IscStatement* stmt);
Sqlda::buffer_t& sqlda_add_row_static_cursor(Sqlda& s);
void sqlda_restore_org_address_fields_static_cursor(Sqlda& s);
bool sqlda_set_current_row_static_cursor(Sqlda& s, int nRow);
void sqlda_get_address_field_static_cursor(Sqlda& s, int column, char*& sqldata, short*& sqlind);
void sqlda_copy_next_in_buffer_static_cursor(Sqlda& s);
void sqlda_copy_next_from_buffer_static_cursor(Sqlda& s);
void sqlda_save_current_to_buffer(Sqlda& s);
void sqlda_restore_buffer_to_current(Sqlda& s);
int sqlda_get_count_rows_static_cursor(Sqlda& s);

// ─── Data access free functions ──────────────────────────────────────
void sqlda_set_value(Sqlda& s, int slot, Value* value, IscStatement* stmt);
void sqlda_set_blob(Sqlda& s, CAttrSqlVar* var, Value* value, IscStatement* stmt);
void sqlda_set_array(Sqlda& s, CAttrSqlVar* var, Value* value, IscStatement* stmt);

int sqlda_find_column(Sqlda& s, const char* columnName);
const char* sqlda_get_table_name(Sqlda& s, int index);
const char* sqlda_get_owner_name(Sqlda& s, int index);
int sqlda_is_blob_or_array(Sqlda& s, int index);
bool sqlda_is_null(Sqlda& s, int index);
void sqlda_set_null(Sqlda& s, int index);
void sqlda_set_not_null(Sqlda& s, int index);

bool sqlda_get_boolean(Sqlda& s, int index);
short sqlda_get_short(Sqlda& s, int index);
int sqlda_get_int(Sqlda& s, int index);
char* sqlda_get_text(Sqlda& s, int index, int& len);
char* sqlda_get_varying(Sqlda& s, int index, int& len);

void sqlda_update_boolean(Sqlda& s, int index, bool value);
void sqlda_update_short(Sqlda& s, int index, short value);
void sqlda_update_int(Sqlda& s, int index, int value);
void sqlda_update_text(Sqlda& s, int index, const char* value);
void sqlda_update_varying(Sqlda& s, int index, const char* dst);

void sqlda_print(Sqlda& s);

/// Phase 14.4.7.5d: Free function for metadata rebuild before parameter execution.
bool sqlda_check_and_rebuild(Sqlda& sqlda);

// ─── Inline wrapper definitions ──────────────────────────────────────
// Thin forwarders for backward-compatible call sites.
// New code should prefer calling the sqlda_*() free functions directly.

inline void Sqlda::clearSqlda()                            { sqlda_clear(*this); }
inline void Sqlda::deleteSqlda()                           { sqlda_delete(*this); }
inline void Sqlda::allocBuffer(IscStatement* st, Firebird::IMessageMetadata* m) { sqlda_alloc_buffer(*this, st, m); }
inline void Sqlda::remapToExternalBuffer(char* b, size_t sz) { sqlda_remap_to_external_buffer(*this, b, sz); }

inline Sqlda::buffer_t& Sqlda::initStaticCursor(IscStatement* st) { return sqlda_init_static_cursor(*this, st); }
inline Sqlda::buffer_t& Sqlda::addRowSqldaInBufferStaticCursor()  { return sqlda_add_row_static_cursor(*this); }
inline void Sqlda::restoreOrgAdressFieldsStaticCursor()    { sqlda_restore_org_address_fields_static_cursor(*this); }
inline bool Sqlda::setCurrentRowInBufferStaticCursor(int r){ return sqlda_set_current_row_static_cursor(*this, r); }
inline void Sqlda::getAdressFieldFromCurrentRowInBufferStaticCursor(int c, char*& d, short*& i) { sqlda_get_address_field_static_cursor(*this, c, d, i); }
inline void Sqlda::copyNextSqldaInBufferStaticCursor()     { sqlda_copy_next_in_buffer_static_cursor(*this); }
inline void Sqlda::copyNextSqldaFromBufferStaticCursor()   { sqlda_copy_next_from_buffer_static_cursor(*this); }
inline void Sqlda::saveCurrentSqldaToBuffer()              { sqlda_save_current_to_buffer(*this); }
inline void Sqlda::restoreBufferToCurrentSqlda()           { sqlda_restore_buffer_to_current(*this); }
inline int  Sqlda::getCountRowsStaticCursor()              { return sqlda_get_count_rows_static_cursor(*this); }

inline void Sqlda::setValue(int slot, Value* v, IscStatement* st) { sqlda_set_value(*this, slot, v, st); }
inline int  Sqlda::findColumn(const char* name)            { return sqlda_find_column(*this, name); }
inline const char* Sqlda::getTableName(int idx)            { return sqlda_get_table_name(*this, idx); }
inline const char* Sqlda::getOwnerName(int idx)            { return sqlda_get_owner_name(*this, idx); }
inline int  Sqlda::isBlobOrArray(int idx)                  { return sqlda_is_blob_or_array(*this, idx); }
inline bool Sqlda::isNull(int idx)                         { return sqlda_is_null(*this, idx); }
inline void Sqlda::setNull(int idx)                        { sqlda_set_null(*this, idx); }
inline void Sqlda::setNotNull(int idx)                     { sqlda_set_not_null(*this, idx); }

inline bool  Sqlda::getBoolean(int idx)                    { return sqlda_get_boolean(*this, idx); }
inline short Sqlda::getShort(int idx)                      { return sqlda_get_short(*this, idx); }
inline int   Sqlda::getInt(int idx)                        { return sqlda_get_int(*this, idx); }
inline char* Sqlda::getText(int idx, int& len)             { return sqlda_get_text(*this, idx, len); }
inline char* Sqlda::getVarying(int idx, int& len)          { return sqlda_get_varying(*this, idx, len); }

inline void Sqlda::updateBoolean(int idx, bool v)          { sqlda_update_boolean(*this, idx, v); }
inline void Sqlda::updateShort(int idx, short v)           { sqlda_update_short(*this, idx, v); }
inline void Sqlda::updateInt(int idx, int v)               { sqlda_update_int(*this, idx, v); }
inline void Sqlda::updateText(int idx, const char* v)      { sqlda_update_text(*this, idx, v); }
inline void Sqlda::updateVarying(int idx, const char* v)   { sqlda_update_varying(*this, idx, v); }

inline void Sqlda::print()                                 { sqlda_print(*this); }

}; // end namespace IscDbcLibrary

#endif // !defined(_SQLDA_H_INCLUDED_)
