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

// IscColumnsMetaData.h: interface for the IscColumnsMetaData class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ISCCOLUMNSMETADATA_H_)
#define _ISCCOLUMNSMETADATA_H_

#include "IscMetaDataResultSet.h"
#include "IscBlob.h"
#include "IscArray.h"
#include "IscSqlType.h"

namespace IscDbcLibrary {

class IscColumnsResultSet : public IscMetaDataResultSet  
{
public:
	virtual bool nextFetch();
	void getColumns(const char * catalog, const char * schemaPattern, const char * tableNamePattern, const char * fieldNamePattern);
	void LegacyTypesConversion();
	IscColumnsResultSet(IscDatabaseMetaData *metaData);
	void initResultSet(IscStatement *stmt);
private:
	virtual void setCharLen (int charLenInd, int fldLenInd, IscSqlType &sqlType);
	virtual void checkQuotes (IscSqlType &sqlType, JString stringVal);
	virtual void adjustResults (IscSqlType &sqlType);	

	IscBlob blob;
	CAttrArray arrAttr;
	IscSqlType sqlType;

	static constexpr int COLUMN_DEFAULT_TARGET = 13;
	static constexpr int COLUMN_DEFAULT_SRC[]  = { 26, 20 };
	static constexpr char DEFAULT_SIGNATURE[]  = "DEFAULT";

	inline void setFieldDefault(bool removeQuotes = false)
	{
		sqlda->updateVarying(COLUMN_DEFAULT_TARGET, "NULL");

		for (auto src_fld : COLUMN_DEFAULT_SRC)
		{
			if (!sqlda->isNull(src_fld))
			{
				auto* var = sqlda->Var(src_fld);
				char buffer[1024];
				int lenRead;

				blob.directOpenBlob((char*)var->sqldata);
				blob.directFetchBlob(buffer, sizeof(buffer) - 1, lenRead);
				blob.directCloseBlob();

				const char* first = buffer + (strncasecmp(buffer, DEFAULT_SIGNATURE, lenRead) ? 0 : sizeof(DEFAULT_SIGNATURE) - 1);
				char* last = buffer + lenRead - 1;

				while (*first == ' ') ++first;
				while (last > first && *last == ' ') --last;

				if (removeQuotes && *first == '\'' && last > first)
				{
					++first;
					if (*last == '\'')
						--last;
				}

				*(last + 1) = '\0';

				sqlda->updateVarying(COLUMN_DEFAULT_TARGET, first);
				break;
			}
		}
	}
};

}; // end namespace IscDbcLibrary

#endif // !defined(_ISCCOLUMNSMETADATA_H_)
