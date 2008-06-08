/*
 *	PROGRAM:		Schema Converter
 *	MODULE:			Constraint.h
 *	DESCRIPTION:	Virtual Field class
 *
 * copyright (c) 1997 by James A. Starkey for IBPhoenix.
 */

#ifndef __CONSTRAINT_H
#define __CONSTRAINT_H

#include "Gen.h"

class Table;
class CString;
class Syntax;
class LinkedList;
class Hash;

enum ConstraintType {
   //primaryKey,
   foreignKey,
   checkClauseConstraint,
   uniqueIndex,
   index
   };

class Constraint : public Gen
{
public:

	Constraint ();
	//Constraint (Table*, Syntax*);
	//Constraint (Field*, Table*, Syntax*);
	~Constraint ();

	void		addOrdered (LinkedList *orderedTables);
	CString		gen (boolean fieldLevel = FALSE);
	boolean		isKey (Field*);

	ConstraintType	type;
	CString			checkClause;
	Table			*foreignTable;
	LinkedList		keys, foreignKeys;
	Table			*table;
	CString			text;
	int				oracleValid;

protected:

};

#endif