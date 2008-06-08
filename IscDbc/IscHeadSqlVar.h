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
 *  The Original Code was created by Vladimir Tsvigun for IBPhoenix.
 *
 *  Copyright (c) 2003 Vladimir Tsvigun
 *  All Rights Reserved.
 */

// IscHeadSqlVar.h: interface for the IscHeadSqlVar class.
// 
//////////////////////////////////////////////////////////////////////

#if !defined(_ISCHEADSQLVAR_H_)
#define _ISCHEADSQLVAR_H_

#include "Sqlda.h"

namespace IscDbcLibrary {

#define MAKEHEAD(a, b, c, d)	\
{								\
    sqlvar->sqltype = a + 1;	\
    sqlvar->sqlscale = b;		\
    sqlvar->sqlsubtype = c;		\
    sqlvar->sqllen = d;			\
}

class IscHeadSqlVar : public HeadSqlVar
{
	XSQLVAR		*sqlvar;
	char		*saveSqldata;
	short		*saveSqlind;
	bool		replaceForParamArray;
	short		sqlMultiple;

public:

	IscHeadSqlVar( CAttrSqlVar *attrVar )
	{
		sqlvar = attrVar->varOrg;
		saveSqldata = sqlvar->sqldata;
		saveSqlind = sqlvar->sqlind;
		replaceForParamArray = attrVar->replaceForParamArray;

		if ( !(sqlvar->sqllen % getCharsetSize( sqlvar->sqlsubtype )) )
			sqlMultiple = getCharsetSize( sqlvar->sqlsubtype );
		else
			sqlMultiple = 1;
	}

	void		setTypeText()		{ MAKEHEAD(SQL_TEXT, 0, 0, 0); }
	void		setTypeVarying()	{ MAKEHEAD(SQL_VARYING, 0, 0, 0); }
	void		setTypeBoolean()	{ MAKEHEAD(SQL_BOOLEAN, 0, 0, sizeof(TYPE_BOOLEAN)); }
	void		setTypeShort()		{ MAKEHEAD(SQL_SHORT, 0, 0, sizeof (short)); }
	void		setTypeLong()		{ MAKEHEAD(SQL_LONG, 0, 0, sizeof (long)); }
	void		setTypeFloat()		{ MAKEHEAD(SQL_FLOAT, 0, 0, sizeof (float)); }
	void		setTypeDouble()		{ MAKEHEAD(SQL_DOUBLE, 0, 0, sizeof (double)); }
	void		setType_D_Float()	{ MAKEHEAD(SQL_D_FLOAT, 0, 0, sizeof (double)); }
	void		setTypeTimestamp()	{ MAKEHEAD(SQL_TIMESTAMP, 0, 0, sizeof (ISC_TIMESTAMP)); }
	void		setTypeBlob()		{ MAKEHEAD(SQL_BLOB, 0, 0, sizeof (ISC_QUAD)); }
	void		setTypeArray()		{ MAKEHEAD(SQL_ARRAY, 0, 0, sizeof (ISC_QUAD)); }
	void		setTypeQuad()		{ MAKEHEAD(SQL_QUAD, 0, 0, sizeof (QUAD)); }
	void		setTypeTime()		{ MAKEHEAD(SQL_TYPE_TIME, 0, 0, sizeof (ISC_TIME)); }
	void		setTypeDate()		{ MAKEHEAD(SQL_TYPE_DATE, 0, 0, sizeof (ISC_DATE)); }
	void		setTypeInt64()		{ MAKEHEAD(SQL_INT64, 0, 0, sizeof (QUAD)); }

	void		setSqlType ( short type ) { sqlvar->sqltype = type; }
	void		setSqlScale ( short scale ) { sqlvar->sqlscale = scale; }
	void		setSqlSubType ( short subtype ) { sqlvar->sqlsubtype = subtype; }
	void		setSqlLen ( short len ) { sqlvar->sqllen = len; }
	short		getSqlMultiple () { return sqlMultiple; }

	char *		getSqlData() { return sqlvar->sqldata; }
	short *		getSqlInd() { return sqlvar->sqlind; }
	void		setSqlData( char *data ) { sqlvar->sqldata = data; }
	void		setSqlInd( short *ind ) { sqlvar->sqlind = ind; }

	bool		isReplaceForParamArray () { return replaceForParamArray; }

	void		release() { delete this; }
	void		restoreOrgPtrSqlData() { sqlvar->sqldata = saveSqldata;	sqlvar->sqlind = saveSqlind; }
};

}; // end namespace IscDbcLibrary

#endif // !defined(_ISCHEADSQLVAR_H_)
