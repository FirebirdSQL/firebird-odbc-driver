/*
 *	PROGRAM:		Virtual Data Manager
 *	MODULE:			Field.cpp
 *	DESCRIPTION:	Virtual Field class
 *
 * copyright (c) 1997 by James A. Starkey for IBPhoenix.
 */

#include "stdafx.h"
#include <stdio.h>
#include <string.h>
#include "Table.h"
#include "Field.h"
//#include "Syntax.h"
#include "Constraint.h"
#include "Hash.h"
//#include "SQLException.h"


Constraint::Constraint ()
{
/**************************************
 *
 *		C o n s t r a i n t
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

table = foreignTable = NULL;
}

Constraint::~Constraint ()
{
/**************************************
 *
 *		~ C o n s t r a i n t
 *
 **************************************
 *
 * Functional description
 *		Get rid of it.
 *
 **************************************/

}

void Constraint::addOrdered (LinkedList *orderedTables)
{
/**************************************
 *
 *		a d d O r d e r e d
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

if (type != foreignKey)
    return;

foreignTable->addOrdered (orderedTables);
}

CString Constraint::gen (boolean fieldLevel)
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
CString string;

if (strcmp (name, ""))
	{
	string = "constraint ";
	string += name;
	}

switch (type)
    {
	case foreignKey:
		if (!fieldLevel)
			{
			string += " FOREIGN KEY ";
			string += genFieldList (&keys);
			}
		string += " REFERENCES ";
		string += foreignTable->getName();
		string += genFieldList (&foreignKeys);
		break;

	/***
	case primaryKey:
		string += " PRIMARY KEY";
		if (!fieldLevel)
			string += genFieldList (&keys);
		break;
	***/

	case uniqueIndex:
		string += " UNIQUE ";
		if (!fieldLevel)
			string += genFieldList (&keys);
		break;

	case checkClauseConstraint:
		string += " CHECK (";
		string += checkClause;
		string += ")";
		break;

	default:
		printf ("Constraint::gen -- not done\n");
	}

return string;
}

boolean Constraint::isKey (Field *target)
{
/**************************************
 *
 *		i s K e y
 *
 **************************************
 *
 * Functional description
 *		Is field a de facto key?
 *
 **************************************/

switch (type)
    {
	//case primaryKey:
	case uniqueIndex:
	case foreignKey:
		FOR_OBJECTS (Field*, field, &keys)
			if (target == field)
				return TRUE;
		END_FOR;
		break;
	}

return FALSE;
}

