/*
 *	PROGRAM:		Virtual Data Manager
 *	MODULE:			Index.cpp
 *	DESCRIPTION:	Index related definitions
 *
 * copyright (c) 1997 by James A. Starkey for IBPhoenix.
 */

#include "stdafx.h"
#include <stdio.h>
#include <string.h>
//#include "Syntax.h"
#include "LinkedList.h"
#include "Table.h"
#include "Index.h"
#include "Field.h"
//#include "CString.h"
#include "Hash.h"
//#include "SQLException.h"
#include "Odbc.h"

Hash	Index::indexes (101);


Index::Index ()
{
/**************************************
 *
 *		I n d e x
 *
 **************************************
 *
 * Functional description
 *		Initialize index object.
 *
 **************************************/

primaryKey = NULL;
fieldCount = 0;
}

Index::Index (Table *tbl, const char *nam, IndexType typ)
{
/**************************************
 *
 *		I n d e x
 *
 **************************************
 *
 * Functional description
 *		Initialize index object.
 *
 **************************************/

fieldCount = 0;
primaryKey = NULL;
table = tbl;
type = typ;
name = nam;
table->addIndex (this);
}

Index::~Index ()
{
/**************************************
 *
 *		~ I n d e x
 *
 **************************************
 *
 * Functional description
 *		Get rid of it.
 *
 **************************************/

}

void Index::addField (Field *field)
{
/**************************************
 *
 *		a d d F i e l d
 *
 **************************************
 *
 * Functional description
 *		Initialize index object.
 *
 **************************************/

++fieldCount;
keyField = field;
fields.append (field);
}

boolean Index::changed (Index *index)
{
/**************************************
 *
 *		c h a n g e d
 *
 **************************************
 *
 * Functional description
 *		See if index characteristics have changed.
 *
 **************************************/

if (type != index->type ||
    fieldCount != index->fieldCount)
	return TRUE;

LinkedList *pos2 = index->fields.getHead();

FOR_OBJECTS (Field*, field, &fields)
	Field *fld = (Field*) index->fields.getNext (&pos2);
	if (strcmp (field->getName(), fld->getName()))
		return TRUE;
END_FOR;

return FALSE;
}

void Index::drop (CDatabase *database)
{
/**************************************
 *
 *		d r o p
 *
 **************************************
 *
 * Functional description
 *		Drop index.
 *
 **************************************/
char	buffer [256];

sprintf (buffer, "drop index %s", name);
OdbcExecute (database, buffer);
}

void Index::fini ()
{
/**************************************
 *
 *		f i n i
 *
 **************************************
 *
 * Functional description
 *		Clean up.
 *
 **************************************/

//delete indexes;
}

CString Index::gen ()
{
/**************************************
 *
 *		g e n 
 *
 **************************************
 *
 * Functional description
 *		Drop index.
 *
 **************************************/
CString	string;

switch (type)
    {
	case UNIQUE_INDEX:
		string += "upgrade unique index ";
		break;

	case SECONDARY_INDEX:
		string += "upgrade index ";
		break;
	}

string += table->getIdentifier() + "_" + name;
string += " on ";
string += table->getName();
string += genFieldList (&fields);

return string;
}

boolean Index::isDuplicate (Index *index)
{
/**************************************
 *
 *		i s D u p l i c a t e
 *
 **************************************
 *
 * Functional description
 *		See if index characteristics have changed.
 *
 **************************************/

if (//type != index->type ||
    fieldCount != index->fieldCount)
	return FALSE;

LinkedList *pos2 = index->fields.getHead();

FOR_OBJECTS (Field*, field, &fields)
	Field *fld = (Field*) index->fields.getNext (&pos2);
	if (fld != field)
		return FALSE;
END_FOR;

return TRUE;
}

boolean Index::isKey (Field *target)
{
/**************************************
 *
 *		i s K e y
 *
 **************************************
 *
 * Functional description
 *		Is Field an index key?
 *
 **************************************/

FOR_OBJECTS (Field*, field, &fields)
	if (target == field)
		return TRUE;
	return FALSE;
END_FOR;

return FALSE;
}

void Index::postLoad()
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

indexes.insert (name, this);
}


LinkedList* Index::getFields()
{
	return &fields;
}

bool Index::isMember(Field * field)
{
/**************************************
 *
 *		i s M e m b e r
 *
 **************************************
 *
 * Functional description
 *		Is field part of index?
 *
 **************************************/

FOR_OBJECTS (Field*, fld, &fields)
	if (fld == field)
		return TRUE;
END_FOR;

return FALSE;
}

CString Index::genSql()
{
	CString string;

	if (type == UNIQUE_INDEX)
		string = "upgrade unique index ";
	else
		string = "upgrade index ";

	string += table->identifier + "_" + getIdentifier() + " on " + table->identifier + "(";
	const char *sep = "";

	FOR_OBJECTS (Field*, field, &fields)
		string += sep + field->getIdentifier();
		sep = ",";
	END_FOR;

	string += ")";

	return string;
}
