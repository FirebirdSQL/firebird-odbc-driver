/*
 *	PROGRAM:		Virtual Data Manager
 *	MODULE:			Index.h
 *	DESCRIPTION:	Index related definitions
 *
 * copyright (c) 1997 by James A. Starkey for IBPhoenix.
 */

#ifndef __INDEX_H
#define __INDEX_H

#include "Gen.h"
#include "LinkedList.h"

class Table;
class Table;
class Field;
class CDatabase;
class Hash;

typedef enum {
    PRIMARY_KEY,
	UNIQUE_INDEX,
	FOREIGN_KEY,
	SECONDARY_INDEX,
	} IndexType;

class Index : public Gen {
    public:
	    CString genSql();
	    bool isMember (Field *field);
	    virtual LinkedList* getFields();

	Index ();
	Index (Table *table, const char *name, IndexType type);
	//Index (Syntax *syntax);
	~Index();

	void		addField (Field*);
	boolean		changed (Index *index);
	void		drop (CDatabase*);
	static void	fini();
	CString		gen();
	boolean		isDuplicate (Index*);
	boolean		isKey (Field*);
	void		postLoad();

	IndexType	type;
	Table		*table;

	LinkedList	fields, foreignKeys;
	Field		*keyField;
	Index		*primaryKey;
	int			fieldCount;

	Index		*collision;
	Index		*duplicate;

	static Hash	indexes;
	};

#endif

