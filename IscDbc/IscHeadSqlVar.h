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

#define MAKEHEAD(a, b)	\
{																	\
	sqlvar->sqltype = a;											\
	sqlvar->sqllen = b;												\
	/*scale is always set separately*/								\
	sqlvar->sqlscale = 0;											\
	/*subtype should not be reset to 0 for blobs*/					\
	sqlvar->sqlsubtype = (a == SQL_BLOB || a == SQL_ARRAY) ? sqlvar->sqlsubtype : 0;	\
}

class IscHeadSqlVar : public HeadSqlVar
{
	CAttrSqlVar *sqlvar;
	char		*saveSqldata;
	short		*saveSqlind;
	bool		replaceForParamArray;
	short		sqlMultiple;

public:

	IscHeadSqlVar( CAttrSqlVar *attrVar )
	{
		sqlvar = attrVar;
		saveSqldata = sqlvar->sqldata;
		saveSqlind = sqlvar->sqlind;
		replaceForParamArray = attrVar->replaceForParamArray;

		if ( !(sqlvar->sqllen % getCharsetSize( sqlvar->sqlsubtype )) )
			sqlMultiple = getCharsetSize( sqlvar->sqlsubtype );
		else
			sqlMultiple = 1;
	}

	inline void	setTypeText()
	{ 
		if (sqlvar->sqltype != SQL_TEXT)
		{
			if (sqlvar->sqltype != SQL_VARYING) sqlvar->sqlsubtype = 0;
			sqlvar->sqltype = SQL_TEXT;
			sqlvar->sqlscale = 0;
		}
	}
	inline void	setTypeVarying()
	{
		if (sqlvar->sqltype != SQL_VARYING)
		{
			if (sqlvar->sqltype != SQL_TEXT) sqlvar->sqlsubtype = 0;
			sqlvar->sqltype = SQL_VARYING;
			sqlvar->sqlscale = 0;
		}
	}

	inline void	setTypeBoolean()	{ MAKEHEAD(SQL_BOOLEAN,		sizeof(TYPE_BOOLEAN));	}
	inline void	setTypeShort()		{ MAKEHEAD(SQL_SHORT,		sizeof(short));			}
	inline void	setTypeLong()		{ MAKEHEAD(SQL_LONG,		sizeof(int));			}
	inline void	setTypeFloat()		{ MAKEHEAD(SQL_FLOAT,		sizeof(float));			}
	inline void	setTypeDouble()		{ MAKEHEAD(SQL_DOUBLE,		sizeof(double));		}
	inline void	setType_D_Float()	{ MAKEHEAD(SQL_D_FLOAT,		sizeof(double));		}
	inline void	setTypeTimestamp()	{ MAKEHEAD(SQL_TIMESTAMP,	sizeof(ISC_TIMESTAMP));	}
	inline void	setTypeBlob()		{ MAKEHEAD(SQL_BLOB,		sizeof(ISC_QUAD));		}
	inline void	setTypeArray()		{ MAKEHEAD(SQL_ARRAY,		sizeof(ISC_QUAD));		}
	inline void	setTypeQuad()		{ MAKEHEAD(SQL_QUAD,		sizeof(QUAD));			}
	inline void	setTypeTime()		{ MAKEHEAD(SQL_TYPE_TIME,	sizeof(ISC_TIME));		}
	inline void	setTypeDate()		{ MAKEHEAD(SQL_TYPE_DATE,	sizeof(ISC_DATE));		}
	inline void	setTypeInt64()		{ MAKEHEAD(SQL_INT64,		sizeof(QUAD));			}

	inline void	setSqlScale ( short scale ) { sqlvar->sqlscale = scale; }
	inline void	setSqlLen(short len) { sqlvar->sqllen = len; }
	inline void	setSqlData ( char* data ) { sqlvar->sqldata = data; }

	inline short	getSqlMultiple () { return sqlMultiple; }
	inline char *	getSqlData() { return sqlvar->sqldata; }
	inline short *	getSqlInd() { return sqlvar->sqlind; }

	// not used
	//void		setSqlInd( short *ind ) { sqlvar->sqlind = ind; }
	//void		setSqlType ( short type ) { sqlvar->sqltype = type; }
	//void		setSqlSubType ( short subtype ) { sqlvar->sqlsubtype = subtype; }

	inline bool	isReplaceForParamArray () { return replaceForParamArray; }
	inline void	release() { delete this; }
	inline void	restoreOrgPtrSqlData() { sqlvar->sqldata = saveSqldata;	sqlvar->sqlind = saveSqlind; }
};

}; // end namespace IscDbcLibrary

#endif // !defined(_ISCHEADSQLVAR_H_)
