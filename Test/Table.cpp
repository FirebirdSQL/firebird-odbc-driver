/*
 *	PROGRAM:		Subschema Upgrade Utility
 *	MODULE:			Table.cpp
 *	DESCRIPTION:	Virtual Table class
 *
 * copyright (c) 1997 by James A. Starkey for IBPhoenix.
 */

#include "stdafx.h"
#include <string.h>
#include <stdio.h>
#include "Odbc.h"
#include "Database.h"
#include "Table.h"
#include "Field.h"
#include "Constraint.h"
#include "Hash.h"
#include "Index.h"
#include "NetfraDatabase.h"
#include "SQLException.h"
#include "Connection.h"
#include "Print.h"

static const char htmlTable [] = "\
{columns}\n\
";

Hash	Table::tables (101);

Table::Table ()
{
/**************************************
 *
 *		T a b l e
 *
 **************************************
 *
 * Functional description
 *		Constructor.
 *
 **************************************/

entered = FALSE;
current = NULL;
frozen = 0;
primaryKey = NULL;
}

Table::Table (Database *db, const char *qual, const char* own, const char *nam, bool view)
{
/**************************************
 *
 *		T a b l e
 *
 **************************************
 *
 * Functional description
 *		Build table from database.
 *
 **************************************/

database = db;
name = nam;
identifier = getIdentifier();
owner = own;
qualifier = qual;
entered = FALSE;
current = NULL;
frozen = 0;
isView = view;
primaryKey = NULL;
}

Table::~Table ()
{
/**************************************
 *
 *		 ~ T a b l e
 *
 **************************************
 *
 * Functional description
 *		Get rid of it.
 *
 **************************************/

if (current)
    {
	delete current;
	current = NULL;
	}

FOR_OBJECTS (Field*, field, &fields)
	delete field;
END_FOR;

FOR_OBJECTS (Index*, index, &indexes)
    delete index;
END_FOR;

FOR_OBJECTS (Constraint*, constraint, &constraints)
    delete constraint;
END_FOR;
}

Field *Table::addField (const char *name, Type type, int length,
						boolean nullable, boolean primary_key,
						const char *comment, Field *domain)
{
/**************************************
 *
 *		a d d F i e l d
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
Field *field = new Field (this, name, type, length, nullable, 
						  primary_key, comment, domain);
fields.append (field);

return field;
}

boolean Table::addIndex (Index *index)
{
/**************************************
 *
 *		a d d I n d e x
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

FOR_OBJECTS (Index*, idx, &indexes)
    if (index->isDuplicate (idx))
		{
		printf ("Ignoring duplicate index %s on %s\n",
				(const char*) index->getName(), (const char*) name);
		return FALSE;
		}
END_FOR;

indexes.append (index);

return TRUE;
}

void Table::addOrdered (LinkedList *orderedTables)
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

if (entered)
    return;

entered = TRUE;

FOR_OBJECTS (Constraint*, constraint, &constraints)
    constraint->addOrdered (orderedTables);
END_FOR;

orderedTables->append (this);
}

void Table::deleteChild (Index *child)
{
/**************************************
 *
 *		d e l e t e C h i l d
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

indexes.deleteItem (child);
}

void Table::deleteChild (Field *child)
{
/**************************************
 *
 *		d e l e t e C h i l d
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

fields.deleteItem (child);
}

Field* Table::findField (const char* name)
{
/**************************************
 *
 *		f i n d F i e l d
 *
 **************************************
 *
 * Functional description
 *		Look up field.
 *
 **************************************/

if (fields.isEmpty())
    getFields();

FOR_OBJECTS (Field*, field, &fields)
	if (field->matches (name))
    //if (!strcmp (field->getName(), name))
		return field;
END_FOR;

return NULL;
}

Index* Table::findIndex (const char* name)
{
/**************************************
 *
 *		f i n d I n d e x
 *
 **************************************
 *
 * Functional description
 *		Look up field.
 *
 **************************************/

FOR_OBJECTS (Index*, index, &indexes)
    if (!strcmp (index->getName(), name))
		return index;
END_FOR;

return NULL;
}

Table* Table::findTable (const char* name)
{
/**************************************
 *
 *		f i n d T a b l e
 *
 **************************************
 *
 * Functional description
 *		Look up table.
 *
 **************************************/

return (Table*) tables.lookup (name);
}

void Table::fini ()
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

//delete tables;
}

void Table::freeze ()
{
/**************************************
 *
 *		f r e e z e
 *
 **************************************
 *
 * Functional description
 *		Shutdown.
 *
 **************************************/

++frozen;
}

CString Table::gen (boolean empty)
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

CString	string = "create table ";
string += name;
char *sep = " (\n    ";

FOR_OBJECTS (Field*, field, &fields)
    string += sep;
	string += field->gen (empty);
	sep = ",\n    ";
END_FOR;


FOR_OBJECTS (Constraint*, constraint, &constraints)
//	if (constraint->oracleValid)
		{
		string += sep;
		string += constraint->gen();
		sep = ",\n    ";
		}
END_FOR;

string += ");";

return string;
}

CString Table::genUpgrade ()
{
/**************************************
 *
 *		g e n U p g r a d e
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
CString	string;
boolean	change = FALSE, add = FALSE, mod = FALSE, drop = FALSE;

if (current)
	{
	FOR_OBJECTS (Field*, field, &fields)
		Field *prior = current->findField (field->getName());
		if (!prior)
			{
			change = add = TRUE;
			break;
			}
		else if (field->changed (prior))
			{
			change = mod = TRUE;
			break;
			}
	END_FOR;
	FOR_OBJECTS (Field*, field, &current->fields)
		if (!findField (field->getName()))
			{
			//change = TRUE;
			drop = TRUE;
			printf ("Field %s.%s dropped\n", name, (const char*) (field->getName()));
			}
	END_FOR;
	}
else
	change = TRUE;

if (!change)
    return string;

if (!current)
	{
	string = "create table ";
	string += name;
	char *sep = " (\n    ";
	FOR_OBJECTS (Field*, field, &fields)
		string += sep;
		string += field->gen();
		sep = ",\n    ";
	END_FOR;
	FOR_OBJECTS (Constraint*, constraint, &constraints)
		if (constraint->oracleValid)
			{
			string += sep;
			string += constraint->gen ();
			sep = ",\n    ";
			}
	END_FOR;
	string += ")";
	return string;
	}

boolean empty = OdbcTableEmpty ((CDatabase*) database, name);
string = "alter table ";
string += name;

if (add)
	{
	char *sep = " ADD (\n    ";
	FOR_OBJECTS (Field*, field, &fields)
		if (!current->findField (field->getName()))
			{
			string += sep;
			string += field->gen (empty);
			sep = ",\n    ";
			}
	END_FOR;
	string += ")\n";
	}

if (mod)
    {
	char *sep = " MOD (\n    ";
	FOR_OBJECTS (Field*, field, &fields)
		Field *prior = current->findField (field->getName());
		if (prior && field->changed (prior))
			{
			string += sep;
			string += field->gen (empty);
			sep = ",\n    ";
			}
	END_FOR;
	string += ")\n";
	}

/***
FOR_OBJECTS (Constraint*, constraint, constraints)
	if (constraint->oracleValid)
		{
		string += sep;
		string += constraint->genOracle();
		sep = ",\n    ";
		}
END_FOR;
***/

return string;
}

void Table::genUpgradeIndexes ()
{
/**************************************
 *
 *		g e n U p g r a d e I n d e x e s
 *
 **************************************
 *
 * Functional description
 *		Rebuild any necessary indexes.
 *
 **************************************/
int		hit = FALSE;

FOR_OBJECTS (Index*, index, &indexes)
	if (index->type == PRIMARY_KEY)
		continue;
	hit = TRUE;
	break;
END_FOR;

if (!hit)
    return;

FOR_OBJECTS (Index*, index, &indexes)
    if (index->type == PRIMARY_KEY)
		continue;
	if (current)
		{
		Index *prior = current->findIndex (index->getName());
		if (prior && !index->changed (prior))
			continue;
		if (prior)
			prior->drop ((CDatabase*) database);
		}
	CString sql = index->gen();
	OdbcExecute ((CDatabase*) database, sql);
END_FOR;
}

#ifdef UNDEF
void Table::getCurrent ()
{
/**************************************
 *
 *		g e t C u r r e n t
 *
 **************************************
 *
 * Functional description
 *		Pick up current definition from database.
 *
 **************************************/
char	tableName [128], owner [128], qualifier [128];
long	len;

HSTMT	statement;
statement = database->allocStatement();
int retcode = SQLTables(statement, NULL, SQL_NTS, 
								NULL, SQL_NTS, 
								(UCHAR*) (const char*) name, SQL_NTS, 
								(UCHAR*) "TABLE", SQL_NTS);
OdbcCheckCode (retcode, statement, "SQLTables");

for (;;)
    {
	retcode = SQLFetch (statement);
	if (retcode == SQL_NO_DATA_FOUND)
		break;
	if (!OdbcCheckCode (retcode, statement, "SQLFetch"))
		break;
	qualifier [0] = owner [0] = tableName [0] = 0;
	GET_STRING (statement, 1, qualifier, len); 
	GET_STRING (statement, 2, owner, len); 
	GET_STRING (statement, 3, tableName, len); 
	current = new Table (database, 
						(qualifier [0]) ? qualifier : NULL, 
						owner, name);
	}

database->freeStatement (statement);

if (current)
	current->getIndexes ();
}
#endif

CString Table::getFullName ()
{
/**************************************
 *
 *		g e t F u l l N a m e
 *
 **************************************
 *
 * Functional description
 *		Pick up full name, optionally qualified by owner.
 *
 **************************************/
CString	string;

if (strcmp (owner, ""))
	{
	string += owner;
	string += ".";
	}

string += name;

return string;
}

LinkedList *Table::getIndexes ()
{
/**************************************
 *
 *		T a b l e
 *
 **************************************
 *
 * Functional description
 *		Get existing indexes.
 *
 **************************************/
char	keyName [128], fieldName [128], currentKey [128];
long	sequence, len;
const char *own = (owner == "") ? NULL : (const char*) owner;
int		retcode;

if (!indexes.isEmpty())
    return &indexes;

getFields();

/* Pick up primary keys */

HSTMT statement = database->allocStatement();

//if (database->primaryKeySupport)
	{
	retcode = SQLPrimaryKeys (statement, (UCHAR*) (const char*) qualifier, SQL_NTS, 
									(UCHAR*) own, SQL_NTS, 
									(UCHAR*)(const char*) name, SQL_NTS);

	if (OdbcCheckCode (retcode, statement, "SQLPrimaryKeys"))
		{
		BIND_STRING (statement, 4, fieldName, len);
		BIND_LONG (statement, 5, sequence, len);
		BIND_STRING (statement, 6, keyName, len);

		currentKey [0] = 0;
		Index *index;

		for (;;)
			{
			retcode = SQLFetch (statement);
			if (retcode == SQL_NO_DATA_FOUND)
				break;
			if (!OdbcCheckCode (retcode, statement, "SQLFetch"))
				break;
			if (strcmp (currentKey, keyName))
				{
				primaryKey = index = new Index (this, keyName, PRIMARY_KEY);
				strcpy (currentKey, keyName);
				}
			Field *field = findField (fieldName);
			if (field)
				index->addField (field);
			else
				AfxMessageBox ("can't find field in index");
			}
		}

	/* Pick up foreign keys */

	SQLFreeStmt (statement, SQL_UNBIND);
	SQLFreeStmt (statement, SQL_CLOSE);
	retcode = SQLForeignKeys (statement, NULL, SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS,
									(UCHAR*)(const char*) qualifier, SQL_NTS, 
									(UCHAR*) own , SQL_NTS, 
									(UCHAR*)(const char*) name, SQL_NTS);

	if (OdbcCheckCode (retcode, statement, "SQLForeignKeys"))
		{
		currentKey [0] = 0;

		BIND_STRING (statement, 8, fieldName, len);
		BIND_LONG (statement, 9, sequence, len);
		BIND_STRING (statement, 12, keyName, len);

		Index *index;
		for (;;)
			{
			retcode = SQLFetch (statement);
			if (retcode == SQL_NO_DATA_FOUND)
				break;
			if (!OdbcCheckCode (retcode, statement, "SQLFetch"))
				break;
			if (strcmp (currentKey, keyName))
				{
				index = new Index (this, keyName, FOREIGN_KEY);
				strcpy (currentKey, keyName);
				}
			Field *field = findField (fieldName);
			if (field)
				index->addField (field);
			else
				AfxMessageBox ("can't find field in index");
			}
		}
	}

/* Pick up other indexes keys */

SQLFreeStmt (statement, SQL_UNBIND);
SQLFreeStmt (statement, SQL_CLOSE);

if (!isView)
	{
	retcode = SQLStatistics (statement, 
							 (UCHAR*)(const char*) qualifier, SQL_NTS, 
							 (UCHAR*)(const char*) own, SQL_NTS, 
							 (UCHAR*)(const char*) name, SQL_NTS,
							 SQL_INDEX_ALL, SQL_QUICK);

	OdbcCheckCode (retcode, statement, "SQLStatistics");
	currentKey [0] = 0;
	long indexType, indexNonUnique;
	Index *index = NULL;

	BIND_LONG (statement, 4, indexNonUnique, len);
	BIND_STRING (statement, 6, keyName, len);
	BIND_LONG (statement, 7, indexType, len);
	BIND_LONG (statement, 8, sequence, len);
	BIND_STRING (statement, 9, fieldName, len);

	for (;;)
		{
		retcode = SQLFetch (statement);
		if (retcode == SQL_NO_DATA_FOUND)
			break;
		if (!OdbcCheckCode (retcode, statement, "SQLFetch"))
			break;
		if (indexType == SQL_TABLE_STAT)
			continue;
		if (strcmp (currentKey, keyName))
			{
			strcpy (currentKey, keyName);
			if (findIndex (keyName))
				{
				index = NULL;
				continue;
				}
			IndexType indexType = (indexNonUnique) ? SECONDARY_INDEX : UNIQUE_INDEX;
			if (!indexNonUnique && !primaryKey && !strcmp (keyName, "PrimaryKey"))
				indexType = PRIMARY_KEY;
			index = new Index (this, keyName, indexType);
			if (indexType == PRIMARY_KEY)
				primaryKey = index;
			}
		if (index)
			{
			Field *field = findField (fieldName);
			if (field)
				index->addField (field);
			else
				AfxMessageBox ("can't find field in index");
			}
		}
	}

database->freeStatement (statement);

return &indexes;
}

boolean Table::isKey (Field *target)
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

FOR_OBJECTS (Index*, index, &indexes)
	if (index->isKey (target))
		return TRUE;
END_FOR;

FOR_OBJECTS (Constraint*, constraint, &constraints)
	if (constraint->isKey (target))
		return TRUE;
END_FOR;

return FALSE;
}

boolean Table::isDeleteable ()
{
/**************************************
 *
 *		i s D e l e t e a b l e
 *
 **************************************
 *
 * Functional description
 *		Can object be decently deleted?
 *
 **************************************/

return frozen == 0;
}

boolean Table::isFirstLargeObject (Field *target)
{
/**************************************
 *
 *		i s F i r s t L a r g e O b j e c t
 *
 **************************************
 *
 * Functional description
 *		Is field the first blob?  (Used to hack around the
 *		Oracle7 restriction of one blob per table).
 *
 **************************************/

FOR_OBJECTS (Field*, field, &fields)
    if (field == target)
		return TRUE;
	if (field->isLargeObject())
		return FALSE;
END_FOR;

return FALSE;
}

boolean Table::isFrozen ()
{
/**************************************
 *
 *		i s F r o z e n
 *
 **************************************
 *
 * Functional description
 *		Can object be decently deleted?
 *
 **************************************/

return frozen > 0;
}

void Table::postLoad()
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

tables.insert (name, this);

FOR_OBJECTS (Index*, index, &indexes)
    index->postLoad();
END_FOR;

FOR_OBJECTS (Field*, field, &fields)
	field->postLoad();
END_FOR;
}

void Table::resetOrdered()
{
/**************************************
 *
 *		r e s e t O r d e r e d
 *
 **************************************
 *
 * Functional description
 *		Reset flag used for building an ordered list of tables.
 *
 **************************************/

entered = FALSE;
}

void Table::thaw ()
{
/**************************************
 *
 *		t h a w
 *
 **************************************
 *
 * Functional description
 *		Shutdown.
 *
 **************************************/

if (frozen > 0)
	--frozen;
}


LinkedList* Table::getFields()
{
/**************************************
 *
 *		g e t F i e l d s
 *
 **************************************
 *
 * Functional description
 *		Shutdown.
 *
 **************************************/

if (!fields.isEmpty())
	return &fields;

printf ("Fields for %s:\n", (const char*) name);
HSTMT statement = database->allocStatement();
const char *own = (owner == "") ? NULL : (const char*) owner;

int retcode = SQLColumns(statement, (UCHAR*)(const char*) qualifier, SQL_NTS, 
								(UCHAR*) own, SQL_NTS, 
								(UCHAR*)(const char*) name, SQL_NTS, 
								NULL, SQL_NTS);

OdbcCheckCode (retcode, statement, "SQLColumns");
FieldInfo	info;
long		len;
BIND_STRING (statement, 4, info.fieldName, len);
BIND_LONG (statement, 5, info.dtype, len);
BIND_STRING (statement, 6, info.typeName, len); 
BIND_LONG (statement, 7, info.precision, len); 
BIND_LONG (statement, 8,info.length, len); 
BIND_LONG (statement, 9, info.scale, len); 
BIND_LONG (statement, 11, info.nullable, len); 

Print print (statement);
print.printHeaders();

for (;;)
    {
	retcode = SQLFetch (statement);
	if (retcode == SQL_NO_DATA_FOUND)
		break;
	if (!OdbcCheckCode (retcode, statement, "SQLFetch"))
		break;
	print.printLine();
	info.print ("    ");
	bool hit = false;
	FOR_OBJECTS (Field*, field, &fields)
		if (field->matches (info.fieldName))
			hit = true;
	END_FOR;
	if (!hit)
		fields.append (new Field (this, &info));
	}

print.skip();
database->freeStatement (statement);

return &fields;
}

bool Table::isUnique(Field * field)
{
/**************************************
 *
 *		i s U n i q u e
 *
 **************************************
 *
 * Functional description
 *		Is field unique with respect to table?
 *
 **************************************/

getIndexes();

FOR_OBJECTS (Index*, index, &indexes)
    if ((index->type == PRIMARY_KEY || index->type == UNIQUE_INDEX) &&
	    index->fieldCount == 1 && index->isKey (field))
		return true;
END_FOR;

return false;
}

Field *Table::findPrimaryKey (Field * field)
{
	getIndexes();

	FOR_OBJECTS (Index*, index, &indexes)
		if (index->type == FOREIGN_KEY)
			FOR_OBJECTS (Index*, foreignKey, &index->foreignKeys)
				if (field->getTable() == foreignKey->table)
					{
					LinkedList *fldPos=(&index->fields)->getHead();
					FOR_OBJECTS (Field*, key, &foreignKey->fields)
						Field *primaryKey = (Field*) (&index->fields)->getNext (&fldPos);
						if (key == field)
							return primaryKey;
					END_FOR;
					}
			END_FOR;
	END_FOR;

	if (database->primaryKeySupport)
		return NULL;

	FOR_OBJECTS (Index*, index, &indexes)
		if (index->type == PRIMARY_KEY || index->type == UNIQUE_INDEX)
			FOR_OBJECTS (Field*, key, &index->fields)
				if (field->keyMatch (key))
					return key;				
			END_FOR;
	END_FOR;

	return NULL;
}

Field* Table::findIndexField(Field * field)
{
	getIndexes();

	FOR_OBJECTS (Index*, index, &indexes)
		//if (index->type == PRIMARY_KEY || index->type == UNIQUE_INDEX)
			FOR_OBJECTS (Field*, key, &index->fields)
				if (field->keyMatch (key))
					return key;				
			END_FOR;
	END_FOR;

	return NULL;
}

void Table::clearReferences()
{
	FOR_OBJECTS (Field*, field, &fields)
		field->clearReferences();
	END_FOR;
}

CString Table::getURL()
{
	return (CString) "tbl_" + name + ".html";
}

/***
void Table::genDocumentation(CProject *project, CString directory)
{
	CString title = "Table ";
	title += name;
	CGenHtml page (project, directory, getURL(), title, title);
	CRString body = project->getTemplate ("htmlTable", htmlTable);
	CGenHtmlTable table (project, "Columns");
	getFields();

	FOR_OBJECTS (Field*, field, &fields)
		field->genDocumentation (&table);
	END_FOR;

	body.replace ("{columns}", (CString) table);
	page.setBody (body);
}
***/

void Table::clearXRef()
{
	FOR_OBJECTS (Field*, field, &fields)
		field->clearXRef();
	END_FOR;
}

CString Table::genSql()
{
	getFields();
	getIndexes();
	CString string = "upgrade table " + getIdentifier();
	const char *sep = "(\n    ";

	FOR_OBJECTS (Field*, field, &fields)
		string += sep + field->genSql();
		if (primaryKey && primaryKey->fieldCount == 1 && primaryKey->isMember (field))
			string += " primary key";
		else if (field->reference)
			string += " references " + field->reference->table->getIdentifier();
		sep = ",\n    ";
	END_FOR;

	string += ")";

	return string;
}

void Table::findReferences()
{
	FOR_OBJECTS (Field*, field, &fields)
		if (!primaryKey || primaryKey->keyField != field)
			field->findForeignReference (database);
	END_FOR;
}

void Table::findDependencies(LinkedList & list)
{
	if (done)
		return;

	if (list.isMember (this))
		{
		CString msg = "Dependency cycle:";
		FOR_OBJECTS (Table*, table, &list)
			msg += " " + table->name;
		END_FOR;
		AfxMessageBox (msg);
		return;
		}

	list.append (this);

	FOR_OBJECTS (Field*, field, &fields)
		if (field->reference)
			field->reference->table->findDependencies(list);
	END_FOR;
}

int Table::copy(NetfraDatabase * db)
{
	done = true;
	int numberRecords = 0;
	CString insert = "insert into " + getIdentifier() + " (";
	CString values;
	CString select = "select ";
	const char *sep = "";

	FOR_OBJECTS (Field*, field, &fields)
		insert += sep + field->getIdentifier();
		select += sep;
		select += "\"" + field->name + "\"";
		values += sep;
		values += "?";
		sep = ",";
	END_FOR;

	PreparedStatement *insertStatement = NULL;
	SQLHSTMT handle = database->allocStatement();

	try
		{
		insert += ") values (" + values + ")";
		insertStatement = db->connection->prepareStatement (insert);
		select += " from \"" + name + "\"";

		SQLRETURN retcode = SQLPrepare (handle, (SQLCHAR*)(const char*) select, SQL_NTS);
		OdbcCheckCode (retcode, handle, "SQLPrepare");
		retcode = SQLExecute (handle);
		OdbcCheckCode (retcode, handle, "SQLExecute");

		char buffer [10000];
		long value, len;
		double dbl;
		TIMESTAMP_STRUCT	odbcDate;
		struct tm time;
		long date;

		while (!retcode)
			{
			retcode = SQLFetch (handle);
			if (retcode == SQL_NO_DATA_FOUND)
				break;
			if (!OdbcCheckCode (retcode, handle, "SQLFetch"))
				break;
			int n = 1;
			FOR_OBJECTS (Field*, field, &fields)
				switch (field->type)
					{
					case Tiny:
					case Short:
					case Int:
						GET_LONG (handle, n, value, len);
						insertStatement->setInt (n, value);
						break;

					case Char:
					case Varchar:
					case TextBlob:
						GET_STRING (handle, n, buffer, len);
						if (len > field->length)
							AfxMessageBox ("field overflow!");
						else if (len <= 0)
							insertStatement->setNull (n, 0);
						else
							insertStatement->setString (n, buffer);
						break;

					case Date:
						retcode = SQLGetData (handle, n, SQL_C_TIMESTAMP, &odbcDate, sizeof (odbcDate), &len);
						OdbcCheckCode (retcode, handle, "SQLGetData");
						if (len <= 0)
							insertStatement->setNull (n, 0);
						else
							{
							memset (&time, 0, sizeof (time));
							time.tm_mday = odbcDate.day;
							time.tm_mon = odbcDate.month - 1;
							time.tm_year = odbcDate.year - 1900;
							time.tm_isdst = -1;
							date = mktime (&time);
							insertStatement->setInt (n, date);
							}
						break;

					case BinaryBlob:
						retcode = SQLGetData (handle, n, SQL_C_BINARY, buffer, sizeof (buffer), &len);
						insertStatement->setNull (n, 0);
						/***
						OdbcCheckCode (retcode, handle, "SQLGetData");
						if (len <= 0)
							insertStatement->setNull (n, 0);
						else
							insertStatement->setBytes (n, len, buffer);
						***/
						break;

					case Double:
						retcode = SQLGetData (handle, n, SQL_C_DOUBLE, &dbl, sizeof (dbl), &len);
						OdbcCheckCode (retcode, handle, "SQLGetData");
						if (len <= 0)
							insertStatement->setNull (n, 0);
						else
							insertStatement->setDouble (n, dbl);
						break;

					default:
						AfxMessageBox ("data type unsupported");
					}
				++n;
			END_FOR;
			insertStatement->executeUpdate();
			++numberRecords;
			}
		}
	catch (SQLException& exception)
		{
		AfxMessageBox (exception.getText());
		}

	if (insertStatement)
		insertStatement->close();

	database->freeStatement (handle);

	return numberRecords;
}

void Table::create(NetfraDatabase * db)
{
	CString sql = genSql();

	try
		{
		done = true;
		db->executeUpdate (sql);
		FOR_OBJECTS (Index*, index, &indexes)
			if (primaryKey && !index->isDuplicate (primaryKey))
				{
				sql = index->genSql();
				db->executeUpdate (sql);
				}
		END_FOR;
		FOR_OBJECTS (Field*, field, &fields)
			if (field->reference)
				{
				sql.Format ("upgrade index %s_%s on %s (%s)",
							(const char*) name,
							(const char*) field->name,
							(const char*) name,
							(const char*) field->name);
				db->executeUpdate (sql);
				}
		END_FOR;
		}
	catch (SQLException& exception)
		{
		CRString msg = exception.getText();
		AfxMessageBox (msg.getCRLFstring());
		}
}

bool Table::populated(NetfraDatabase * db)
{
	bool result = false;
	PreparedStatement *statement = NULL;

	try
		{
		CString string = "select * from ";
		string += getIdentifier();
		statement = db->connection->prepareStatement (string);
		ResultSet *resultSet = statement->executeQuery();
		if (resultSet->next())
			result = true;
		resultSet->close();
		}
	catch (SQLException& exception)
		{
		CRString msg = exception.getText();
		AfxMessageBox (msg.getCRLFstring());
		}

	if (statement)
		statement->close();

	return result;
}

void FieldInfo::print(const char * prefix)
{
	printf ("%s%s type %s (%d), length %d, scale %d, nullable %d\n",
			prefix, fieldName, typeName, dtype, precision, length, nullable);
}
