/*
 *	PROGRAM:		Subschema Upgrade Utility
 *	MODULE:			Field.cpp
 *	DESCRIPTION:	Virtual Field class
 *
 * copyright (c) 1997 by James A. Starkey for IBPhoenix.
 */

#include "stdafx.h"
#include <stdio.h>
#include <string.h>
#include "Odbc.h"
#include "Table.h"
#include "Field.h"
#include "Hash.h"
#include "Constraint.h"
#include "Database.h"
#include "Index.h"
//#include "GenHtmlTable.h"
//#include "Class.h"
//#include "Property.h"

#define UPCASE(c)		((c >= 'a' && c <= 'z') ? c - 'a' + 'A' : c)

Hash	Field::domains (101);

Field::Field ()
{
/**************************************
 *
 *		F i e l d
 *
 **************************************
 *
 * Functional description
 *		Constructor.
 *
 **************************************/
table = NULL;
notNull = FALSE;
scale = length = precision = 0;
domain = NULL;
primaryKey = FALSE;
foreignKey = NULL;
frozen = 0;
referenceCount = 0;
reference = NULL;
}

Field::Field (Table *tbl, const char *nam, Type typ, int len,
			   boolean nullable, boolean pk, 
			   const char *cmnt, Field *dom)
{
/**************************************
 *
 *		F i e l d
 *
 **************************************
 *
 * Functional description
 *		Constructor.
 *
 **************************************/

setType (typ, len);
foreignKey = NULL;
table = tbl;
name = nam;
notNull = !nullable;
primaryKey = pk;
comment = cmnt;
domain = dom;
frozen = 0;
referenceCount = 0;
reference = NULL;

if (domain)
	setType (domain);
}

Field::Field (Table *tbl, FieldInfo *info)
{
/**************************************
 *
 *		F i e l d
 *
 **************************************
 *
 * Functional description
 *		Constructor.
 *
 **************************************/

setType (info->dtype, info->precision, info->typeName);
table = tbl;
notNull = FALSE;
scale = info->scale;
length = info->length;
domain = NULL;
primaryKey = FALSE;
foreignKey = NULL;
frozen = 0;
referenceCount = 0;
reference = NULL;

for (char *p = info->fieldName; *p; p++)
    *p = UPCASE (*p);

name = info->fieldName;
Field::typeName = info->typeName;

//type = typeFromOdbcType (info->dtype, info->typeName);

}

Field::~Field ()
{
/**************************************
 *
 *		~ F i e l d
 *
 **************************************
 *
 * Functional description
 *		Constructor.
 *
 **************************************/

if (foreignKey)
    {
	delete foreignKey;
	foreignKey = NULL;
	}
}

boolean Field::changed (Field *field)
{
/**************************************
 *
 *		c h a n g e d
 *
 **************************************
 *
 * Functional description
 *		Determine whether field has changed characteristics.
 *
 **************************************/

if (type != field->type ||
    length != field->length ||
	scale != field->scale ||
	precision != field->precision ||
	notNull != field->notNull)
	return FALSE;

return TRUE;
}

Field *Field::findDomain (const char *name)
{
/**************************************
 *
 *		f i n d D o m a i n 
 *
 **************************************
 *
 * Functional description
 *		Find domain if defined; otherwise return NULL.
 *
 **************************************/

return (Field*) domains.lookup (name);
}

void Field::fini ()
{
/**************************************
 *
 *		f i n i
 *
 **************************************
 *
 * Functional description
 *		Shutdown.
 *
 **************************************/

//domains->deleteAll();
//delete domains;
}

void Field::freeze ()
{
/**************************************
 *
 *		f r e e z e
 *
 **************************************
 *
 * Functional description
 *		Freeze field from arbitrary update.
 *
 **************************************/

++frozen;
}

CString Field::gen (boolean empty)
{
/**************************************
 *
 *		g e n 
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
CString	string = name;

if (!strcmp (name, "START"))
	string += "X";

string += " ";
CString typeString;

if (domain)
    {
	type = domain->type;
	length = domain->length;
	precision = domain->precision;
	scale = domain->scale;
	}

if (genType == GenGeneric && domain)
	typeString = domain->getName();
else
	typeString = genSqlString();

if (!strcmp (defaultValue, "SYSTEM_USER"))
	typeString = "varchar (40)";

string += typeString;

if (!defaultValue.IsEmpty())
	{
	string += " DEFAULT (";
	if (!strcmp (defaultValue, "SYSTEM_USER"))
		string += "USER";
	else
		string += defaultValue;
	string += ")";
	}

if (primaryKey)
    string += " PRIMARY KEY";

if (notNull && empty)
    string += " NOT NULL";

if (foreignKey)
	{
	string += " ";
    string += foreignKey->gen (TRUE);
	}

return string;
}

Field *Field::getDomain ()
{
/**************************************
 *
 *		g e t D o m a i n
 *
 **************************************
 *
 * Functional description
 *		Get length from field or domain.
 *
 **************************************/

return domain;
}

int Field::getLength ()
{
/**************************************
 *
 *		g e t L e n g t h
 *
 **************************************
 *
 * Functional description
 *		Get length from field or domain.
 *
 **************************************/

return (domain) ? domain->getLength() : length;
}

int Field::getPrecision ()
{
/**************************************
 *
 *		g e t P r e c i s i o n
 *
 **************************************
 *
 * Functional description
 *		Get scale from field or domain.
 *
 **************************************/

return (domain) ? domain->getPrecision() : precision;
}

int Field::getScale ()
{
/**************************************
 *
 *		g e t S c a l e
 *
 **************************************
 *
 * Functional description
 *		Get scale from field or domain.
 *
 **************************************/

return (domain) ? domain->getScale() : scale;
}

Table *Field::getTable ()
{
/**************************************
 *
 *		g e t T a b l e
 *
 **************************************
 *
 * Functional description
 *		Return table if field, NULL if domain.
 *
 **************************************/

return table;
}

Type Field::getType ()
{
/**************************************
 *
 *		g e t T y p e
 *
 **************************************
 *
 * Functional description
 *		Get type from field or domain.
 *
 **************************************/

return (domain) ? domain->getType() : type;
}

boolean Field::isDeleteable ()
{
/**************************************
 *
 *		i s D e l e t e a b l e
 *
 **************************************
 *
 * Functional description
 *		Is field an indexed key?
 *
 **************************************/

return frozen == 0;
}

boolean Field::isFrozen ()
{
/**************************************
 *
 *		i s F r o z e n
 *
 **************************************
 *
 * Functional description
 *		Is field an indexed key?
 *
 **************************************/

return frozen > 0;
}

boolean Field::isKey ()
{
/**************************************
 *
 *		i s K e y
 *
 **************************************
 *
 * Functional description
 *		Is field an indexed key?
 *
 **************************************/

if (primaryKey || foreignKey)
	return TRUE;

if (table)
	return table->isKey (this);

return FALSE;
}

boolean Field::isNullable ()
{
/**************************************
 *
 *		i s N u l l a b l e
 *
 **************************************
 *
 * Functional description
 *		Is field nullable?
 *
 **************************************/

return !notNull;
}

boolean Field::isLargeObject()
{
/**************************************
 *
 *		i s L a r g e O b j e c t
 *
 **************************************
 *
 * Functional description
 *		Is this thing a blob/clob/long/image/text?
 *
 **************************************/

return (type == TextBlob || type == BinaryBlob);
}

boolean Field::isPrimaryKey ()
{
/**************************************
 *
 *		i s P r i m a r y K e y
 *
 **************************************
 *
 * Functional description
 *		Is field nullable?
 *
 **************************************/

return primaryKey;
}

boolean Field::modified (const char *newName, Type newType,
						 int newLength, boolean nullable,
						 boolean newPrimaryKey, const char *newComment,
						 Field *newDomain)
{
/**************************************
 *
 *		m o d i f i e d
 *
 **************************************
 *
 * Functional description
 *		Update field attributes (maybe), indicate whether anything
 *		changed.
 *
 **************************************/
int		changed = FALSE;

if (strcmp (name, newName))
	{
	name = newName;
	changed = TRUE;
	}

if (type != newType)
    {
	type = newType;
	changed = TRUE;
	}

switch (type)
    {
	case Char:
	case Varchar:
		if (length != newLength)
			{
			length = newLength;
			changed = TRUE;
			}
		break;

	default:
		if (isInteger())
			{
			if (newLength != precision)
				{
				precision = newLength;
				changed = TRUE;
				}
			}
	}

if (newDomain != domain)
	{
	domain = newDomain;
	changed = TRUE;
	}

if (domain)
	{
	type = domain->type;
	length = domain->length;
	precision = domain->precision;
	scale = domain->scale;
	}

if (nullable == notNull)
	{
	notNull = !nullable;
	changed = TRUE;
	}

if (primaryKey != newPrimaryKey)
    {
	primaryKey = newPrimaryKey;
	changed = TRUE;
	}

if (strcmp (comment, newComment))
	{
	comment = newComment;
	changed = TRUE;
	}

return changed;
}

void Field::postLoad()
{
/**************************************
 *
 *		p o s t L o a d
 *
 **************************************
 *
 * Functional description
 *		Finish load process.
 *
 **************************************/

if (!table)
    domains.insert (name, this);

//comment = "";
}


void Field::thaw()
{
/**************************************
 *
 *		t h a w
 *
 **************************************
 *
 * Functional description
 *		Finish load process.
 *
 **************************************/

if (frozen > 0)
   --frozen;
}


CString Field::genSqlString()
{
/**************************************
 *
 *		g e n S q l S t r i n g
 *
 **************************************
 *
 * Functional description
 *		Finish load process.
 *
 **************************************/
CString	typeString;

switch (type)
	{
	case Char:
		typeString.Format ("char (%d)", length);
		break;

	case Varchar:
		typeString.Format ("varchar (%d)", length);
		break;

	case Tiny:
		typeString = "tinyInt";
		break;

	case Short:
		typeString = "smallInt";
		break;

	case Int:
		typeString = "int";
		break;

	case Long:
		//typeString.Format ("number (%d)", precision);
		break;

	case TextBlob:
		switch (genType)
			{
			case GenOracle7:
				if (table->isFirstLargeObject (this))
					typeString = "long";
				else
					typeString = "varchar (2000)";
				break;

			case GenSqlServer:
				typeString = "text";
				break;

			default:
			case GenOracle8:
				typeString = "clob"; //"long";
				break;

			};
		break;

	case BinaryBlob:
		switch (genType)
			{
			case GenOracle7:
				typeString = "long";
				break;

			case GenSqlServer:
				typeString = "image";
				break;

			default:
			case GenOracle8:
				typeString = "blob";
				break;

			};
		break;

	case Date:
		typeString = "date";
		break;

	case Float:
		typeString = "float";
		break;

	case Double:
		typeString = "double";
		break;

	default:
		typeString = "*** genSqlString not done ***";
		break;
	}

return typeString;
}

Database* Field::getDatabase()
{
	return table->database;
}

bool Field::matches(const char * string)
{
/**************************************
 *
 *		m a t c h e s
 *
 **************************************
 *
 * Functional description
 *		Case insensitive comparison.
 *
 **************************************/
const char *p = name;
const char *q = string;

for (; *p; ++p, ++q)
	if (UPCASE (*p) != UPCASE (*q))
		return FALSE;

return *q == 0;
}


bool Field::isUnique()
{
/**************************************
 *
 *		i s U n i q u e
 *
 **************************************
 *
 * Functional description
 *		Is field known to be unique in table?
 *
 **************************************/

return table->isUnique (this);
}

bool Field::keyMatch(Field * field)
{
/**************************************
 *
 *		k e y M a t c h
 *
 **************************************
 *
 * Functional description
 *		If a field in a another relation a reason join term?
 *
 **************************************/

if (name == field->name && type == field->type)
	return true;

return false;
}


void Field::clearReferences()
{
referenceCount = 0;
}

void Field::addReference()
{
++referenceCount;
}

void Field::deleteReference()
{
--referenceCount;
}

/***
void Field::genDocumentation(CGenHtmlTable * table)
{
	if (table->first)
		table->addRow ("Column Name", "Data Type", "Component", "Property");

	table->row();
	table->addElement (name);
	table->addElement (genSqlString());
	int count = 0;

	FOR_OBJECTS (CProperty*, property, &references)
		if (count++)
			{
			table->endRow();
			table->addElement ("");
			table->addElement ("");
			}
		CClass *pClass = property->pClass;
		table->addLink (pClass->name, pClass->getURL());
		table->addElement (property->name);
	END_FOR;

	if (!count)
		table->addSpanningElement (2, "unmapped");

	table->endRow();
}
***/

void Field::clearXRef()
{
	references.clear();
}

void Field::addReference(CProperty * property)
{
	references.append (property);
}


CString Field::genSql()
{
	CString string = getIdentifier() + " " + genSqlString();

	return string;
}

Index* Field::findForeignReference(Database * database)
{
	int count = 0;

	for (Index *index = database->findPrimaryKey (this); index; index = index->duplicate)
		if (index->table != table)
			{
			++count;
			reference = index;
			}

	if (count != 1)
		reference = NULL;

	return reference;
}
