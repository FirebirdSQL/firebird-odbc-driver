// Type.cpp: implementation of the CType class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <ole2.h>
#include <string.h>
#include "Odbc.h"
//#include "MCET.h"
#include "Type.h"
#include "RString.h"

#ifndef NOT_YET_SUPPORTED
#define NOT_YET_SUPPORTED(msg)	OutputDebugString (msg)
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CType::CType()
{
	length = scale = precision = indirectCount = 0;
}

CType::~CType()
{

}

bool CType::isLargeObject()
{
	return type == TextBlob || type == BinaryBlob;
}


VARTYPE CType::getVariantType()
{
	switch (type)
		{
		case Char:			return VT_I4;
		case Varchar:		return VT_BSTR;
		case Tiny:			return VT_I1;
		case Short:			return VT_I2;
		case Int:			return VT_I4;
		case Long:			return VT_I8;
		case Float:			return VT_R4;
		case Double:		return VT_R8;
		case Date:			return VT_DATE;
		case TextBlob:
		case BinaryBlob:	return VT_BLOB;
		}

	return VT_I4;	
}

void CType::setType(CType * t)
{
	type = t->type;
	length = t->length;
	scale = t->scale;
	precision = t->precision;
}

void CType::setType(Type typ, int len)
{
	type = typ;
	length = len;

	if (isInteger())
		precision = length;
}

bool CType::isInteger()
{
	switch (type)
		{
		case Tiny:
		case Short:
		case Int:
		case Long:
			return true;
		}

	return false;
}

int CType::getPrecision()
{
	if (precision)
		return precision;

	switch (type)
		{
		case Tiny:		return 2;
		case Short:		return 5;
		case Int:		return 9;
		case Long:		return 17;
		}

	return 0;
}

void CType::setType(long odbcType, long prec, const char * typeName)
{
	precision = prec;

	switch (odbcType)
		{
		case SQL_VARCHAR:	type = Varchar;	break;
		case SQL_CHAR:		type = Char;	break;
		case SQL_FLOAT:		type = Float;	break;
		case SQL_DOUBLE:	type = Double;	break;
		case SQL_REAL:		type = Float;	break;

		case SQL_TYPE_TIMESTAMP:
		case SQL_TIMESTAMP:	
			type = Date;	
			break;

		case SQL_NUMERIC:
		case SQL_DECIMAL:
			if (precision == 0)
				type = Int;
			else if (precision <= 2)
				type = Tiny;
			else if (precision <= 5)
				type = Short;
			else
				type = Int;
			/***
			else if (precision <= 9)
				type = Int;
			else
				type = Long;
			***/
			break;

		case SQL_BINARY:
		case SQL_INTEGER:	type = Int;		break;
		case SQL_SMALLINT:	type = Short;	break;

		case SQL_LONGVARBINARY:	type = BinaryBlob; break;
		case SQL_LONGVARCHAR:	type = TextBlob; break;



		default:
			if (!strcmp (typeName, "LONG"))
				type = BinaryBlob;
			else if (!strcmp (typeName, "LONGTEXT"))
				type = TextBlob;
			else if (!strcmp (typeName, "BIT"))
				type = Int;
			else
				{
				CString msg;
				msg.Format ("Field::typeFromOdbcType -- don't understand type %d (%s)\n", 
						odbcType, typeName);
				AfxMessageBox (msg);
				type = Int;
				}
		}
}

int CType::getBoundary(CharWidth charWidth)
{
	switch (type)
		{
		case Char:
		case Varchar:
			return (charWidth == oneByte) ? 1 : 2;

		case Tiny:			return sizeof (char);
		case Short:			return sizeof (short);
		case Int:			return sizeof (long);
		case Long:			return 8;
		case Float:			return sizeof (float);
		case Double:		return sizeof (double);
		case Date:			return sizeof (DATE);

		case TextBlob:
		case BinaryBlob:	return sizeof (IUnknown*);
		}

	return sizeof (int);	
}

int CType::getCLength(CharWidth charWidth)
{
	int len = getLength();

	switch (type)
		{
		case Varchar:
		case Char:
			if (charWidth == oneByte)
				return length + 1;
			return 2 * (length + 1);
		
		}

	return len;	
}

CString CType::getCDeclaration(const char * name, CharWidth charWidth)
{
	CRString cType;

	switch (type)
		{
		case Char:
		case Varchar:
			cType.Format ((charWidth == oneByte) ? "char\t<name>" : "WCHAR\t<name> [%d]", length + 1);
			break;

		case Tiny:
			cType = "char\t<name>";
			break;

		case Short:
			cType = "short\t<name>";
			break;

		case Int:
			cType = "LONG\t<name>";
			break;

		case Long:
			cType = "__int64\t<name>";
			break;

		case Date:
			cType = "DATE\t<name>";
			break;

		case Double:
			cType = "double\t<name>";
			break;

		case Float:
			cType = "float\t<name>";
			break;

		case TextBlob:
			cType = "IUnknown*\t<name>";
			break;

		default:
			NOT_YET_SUPPORTED ("CType::getCDeclaration\n");
		}

	cType.replace ("<name>", name);

	return cType;
}

CString CType::getOleDbBindType()
{
	switch (type)
		{
		case Char:
		case Varchar:
			return "DBTYPE_WSTR";

		case Tiny:
			return "DBTYPE_I1";

		case Short:
			return "DBTYPE_I2";

		case Int:
			return "DBTYPE_I4";

		case Long:
			return "DBTYPE_I8";

		case Date:
			return "DBTYPE_DATE";

		case Double:
			return "DBTYPE_R8";

		case Float:
			return "DBTYPE_R4";

		case TextBlob:
			return "DBTYPE_IUNKNOWN";

		default:
			NOT_YET_SUPPORTED ("CType::getOleDbBindType");
		}

	return "*** unknown type ***";
}

CString CType::getJavaType()
{
	switch (type)
		{
		case Varchar:	return "String";
		case Float	:	return "Float";
		case Double:	return "Double";
		case Date:		return "Date";
		case Tiny:		return "byte";
		case Short:		return "short";
		case Int:		return "int";
		case Long:		return "long";
		case TextBlob:	return "UnicodeStream";
		case BinaryBlob:	return "BinaryStream";

		default:
			NOT_YET_SUPPORTED ("CType::getjavaType");
		}

	return "*** unknown type ***";
}

int CType::getLength()
{
	switch (type)
		{
		case Varchar:
		case Char:			return length;
		case Tiny:			return sizeof (char);
		case Short:			return sizeof (short);
		case Int:			return sizeof (long);
		case Long:			return 8;
		case Float:			return sizeof (float);
		case Double:		return sizeof (double);
		case Date:			return sizeof (DATE);
		case TextBlob:
		case BinaryBlob:	return sizeof (IUnknown*);
		}

	return sizeof (int);	

}

bool CType::isCharacter()
{
	return type == Char || type == Varchar;
}

CString CType::getJdbcType()
{
	switch (type)
		{
		case Varchar:	return "String";
		case Float	:	return "Float";
		case Double:	return "Double";
		case Date:		return "Date";
		case Tiny:		return "Byte";
		case Short:		return "Short";
		case Int:		return "Int";
		case Long:		return "Long";
		case TextBlob:	return "UnicodeStream";
		case BinaryBlob:	return "BinaryStream";

		default:
			NOT_YET_SUPPORTED ("CType::getjavaType");
		}

	return "*** unknown type ***";
}
