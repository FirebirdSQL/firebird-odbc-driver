// PDatabase.cpp: implementation of the Database class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
//#include "MCET.h"
#include "Database.h"
#include "Table.h"
//#include "StoredProcedure.h"
#include "Odbc.h"
//#include "Project.h"
//include "GenHtml.h"
//#include "GenHtmlList.h"
#include "RString.h"
#include "Index.h"
#include "NetfraDatabase.h"
#include "Print.h"

#define UPCASE(c)		((c >= 'a' && c <= 'z') ? c - 'a' + 'A' : c)
#define SQL_ANY			(UCHAR*) "%"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

static const char htmlDatabase [] = "\
{tables}\n\
";

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Database::Database()
{
	statements = 0;
	database = NULL;
}

Database::~Database()
{
	FOR_OBJECTS (Table*, object, &tables)
		delete object;
	END_FOR;

	/***
	FOR_OBJECTS (StoredProcedure*, object, &procedures)
		delete object;
	END_FOR;
	***/

	if (database)
		delete database;
}

LinkedList* Database::getTables()
{
	if (!tables.isEmpty())
		return &tables;

	HSTMT	statement;
	statement = allocStatement();
	int retcode = SQLTables (statement, (UCHAR*) SQL_ALL_CATALOGS, SQL_NTS,	// catalog name
									(UCHAR*) SQL_ALL_SCHEMAS, SQL_NTS,  // schema name
									SQL_ANY, SQL_NTS,  // table name
									(UCHAR*) "TABLE,VIEW", SQL_NTS);
	OdbcCheckCode (retcode, statement, "SQLTables");
	char qualifier [128], owner [128], name [128], type [32];
	long len;
	BIND_STRING (statement, 1, qualifier, len);
	BIND_STRING (statement, 2, owner, len);
	BIND_STRING (statement, 3, name, len);
	BIND_STRING (statement, 4, type, len);
	Print print (statement);
	print.printHeaders();

	for (;;)
		{
		qualifier [0] = 0;
		owner [0] = 0;
		retcode = SQLFetch (statement);
		if (retcode == SQL_NO_DATA_FOUND)
			break;
		if (!OdbcCheckCode (retcode, statement, "SQLFetch"))
			break;
		print.printLine();
		Table *table = new Table (this, qualifier, owner, name, !strcmp (type, "VIEW"));
		tables.append (table);
		}

	freeStatement (statement);
	memset (primaryKeys, 0, sizeof (primaryKeys));

	FOR_OBJECTS (Table*, table, &tables)
		table->getFields();
		table->getIndexes();
		Index *primaryKey = table->primaryKey;
		if (primaryKey && primaryKey->fieldCount == 1)
			{
			Field *field = primaryKey->keyField;
			int slot = field->hash (PK_HASH_SIZE);
			for (Index *index = primaryKeys [slot]; index; index = index->collision)
				if (index->keyField->name == field->name)
					{
					primaryKey->duplicate = index->duplicate;
					index->duplicate = primaryKey;
					break;
					}
			if (!index)
				{
				primaryKey->duplicate = NULL;
				primaryKey->collision = primaryKeys [slot];
				primaryKeys [slot] = primaryKey;
				}
			}
	END_FOR;

	FOR_OBJECTS (Table*, table, &tables)
		table->findReferences();
	END_FOR;

	return &tables;
}

LinkedList* Database::getProcedures()
{
	if (!procedures.isEmpty())
		return &procedures;

	HSTMT	statement;
	statement = allocStatement();
	int retcode = SQLProcedures (statement, NULL, SQL_NTS,	// catalog name
									NULL, SQL_NTS,  // schema name
									NULL, SQL_NTS);  // table name
	OdbcCheckCode (retcode, statement, "SQLColumns");
	char qualifier [128], owner [128], name [128];
	long len;
	BIND_STRING (statement, 1, qualifier, len);
	BIND_STRING (statement, 2, owner, len);
	BIND_STRING (statement, 3, name, len);

	for (;;)
		{
		retcode = SQLFetch (statement);
		if (retcode == SQL_NO_DATA_FOUND)
			break;
		if (!OdbcCheckCode (retcode, statement, "SQLFetch"))
			break;
		//procedures.append (new StoredProcedure (this, qualifier, owner, name));
		}

	freeStatement (statement);

	return &procedures;
}

BOOL Database::OpenEx(LPCTSTR DBconnectString, DWORD dwOptions)
{
	BOOL	ret;
	CString temp;
	DWORD	options = dwOptions;

	if (!database)
		database = new CDatabase;

	if (DBconnectString)
		{
		temp = rewriteConnectString (DBconnectString);
		options = CDatabase::noOdbcDialog;
		}
	
	try 
		{
		ret = database->OpenEx (DBconnectString, options);
		}
	catch (CDBException *)
		{
		try
			{
			ret = database->OpenEx (temp, dwOptions);
			}
		catch (CDBException *exception)
			{
			CString text = exception->m_strStateNativeOrigin;
			AfxMessageBox (text);
			return false;
			}
		}

	if (!ret)
		return ret;

	if (name == "")
		{
		temp = database->GetDatabaseName();
		char c;
		const char *p, *start;

		for (p = start = temp; c = *p++;)
			if (c == '\\' || c == ':' || c == ' ' || c == '.')
				start = p;

		name = start;
		}

	connectString = database->GetConnect();
	primaryKeySupport = false;
		
	SWORD	length;
	union {
	    ULONG	lValue;
		short	sValue;
		char	string [256];
		} info;

	ret = SQLGetInfo (database->m_hdbc, SQL_ODBC_API_CONFORMANCE, &info, sizeof (info.string), &length);

	if (!ret && info.sValue >= 2)
		primaryKeySupport = true;

	//countObjects();

	return true;
}

void* Database::allocStatement()
{
	HSTMT	statement;

	if (statements++)
		AfxMessageBox ("Multiple statements");

	//int retcode = SQLAllocStmt (database->m_hdbc, &statement);
	statement = (HSTMT) 123;
	int retcode = SQLAllocHandle (SQL_HANDLE_STMT, database->m_hdbc, &statement);

	if (retcode)
		{
		UCHAR	sqlState [128], text [SQL_MAX_MESSAGE_LENGTH];
		SDWORD	nativeCode;
		SWORD	textLength;
		int n = SQLError (SQL_NULL_HENV, database->m_hdbc, SQL_NULL_HSTMT, 
						  sqlState, &nativeCode, 
						  text, sizeof (text) -1, &textLength);
		printf ("SQLAllocHandle failed: %s\n", text);
		return NULL;
		}

	//retcode = SQLSetStmtAttr (statement, SQL_ATTR_CONCURRENCY, (SQLPOINTER) SQL_CONCUR_READ_ONLY, NULL);

	return statement;
}

void Database::freeStatement(void* statement)
{
	//SQLFreeStmt (statement, SQL_DROP);

	SQLFreeHandle (SQL_HANDLE_STMT, statement);
	--statements;

}

Table* Database::findTable(const char * name)
{
	if (tables.isEmpty())
		getTables();

	FOR_OBJECTS (Table*, table, &tables)
		if (!stricmp (table->getName(), name))
		//if (table->getName() == name)
			return table;
	END_FOR;

	return NULL;
}

CString Database::rewriteConnectString(const char * orgString)
{
	CString string;
	CString dsn;
	for (const char *p = orgString; *p;)
		{
		CString keyword;
		char c;
		while ((c = *p++) && c != '=' && c != ';')
			keyword += c;
		if (!c)
			break;
		CString option = keyword;
		if (c == '=')
			{
			option += c;
			while ((c = *p++) && c != ';')
				option += c;
			if (!c)
				break;
			}
		option += ';';
		keyword.MakeUpper();
		if (keyword == "DSN")
			dsn = option;
		else 
			string += option;
		}
	string += dsn;

	return string;
}

Database::operator CDatabase *()
{
	return database;
}

CString Database::GetConnect()
{
	return database->GetConnect();
}

CString Database::getConnectOption(const char * option)
{
	CString	string;
	
	for (const char *p = connectString; *p;)
		{
		for (const char *q = p, *o = option; *o && UPCASE (*q) == UPCASE (*o); ++q, ++o)
			;
		if (!*o)
			{
			if (*q++ == '=')
				while (*q && *q != ';')
					string += *q++;
			return string;				
			}
		while (*p && *p++ != ';')
			;
		}

	return string;
}

void Database::countObjects()
{
	int numberTables = 0, numberFields = 0;
	getTables();

	FOR_OBJECTS (Table*, table, &tables)
		++numberTables;
		LinkedList *fields = table->getFields();
		FOR_OBJECTS (Field*, field, fields)
			++numberFields;
		END_FOR;
	END_FOR;

	printf ("%d, %d\n", numberTables, numberFields);
}

void Database::clearReferences()
{
	FOR_OBJECTS (Table*, table, &tables)
		table->clearReferences();
	END_FOR;
}

void Database::setName(CString & newName)
{
	name = newName;
}

CString Database::getURL()
{
	return (CString) "db_" + name + ".html";
}

/***
void Database::genDocumentation(CProject * project, CString directory)
{
	CString title = "Database ";
	title += name;
	CGenHtml page (project, directory, getURL(), title, title);
	CRString body = project->getTemplate ("htmlDatabase", htmlDatabase);
	CGenHtmlList list ("Tables");
	getTables();

	FOR_OBJECTS (Table*, table, &tables)
		list.addElement (page.makeLink (table->getName(), table->getURL()));
		table->genDocumentation (project, directory);
	END_FOR;

	body.replace ("{tables}", (CString) list);
	page.setBody (body);
}
***/

void Database::clearXRef()
{
	FOR_OBJECTS (Table*, table, &tables)
		table->clearXRef();
	END_FOR;
}

Index* Database::findPrimaryKey(Field * field)
{
	for (Index *index = primaryKeys [field->hash (PK_HASH_SIZE)]; index; index = index->collision)
		if (index->keyField->name == field->name)
			return index;

	return NULL;
}

void Database::clear()
{
	FOR_OBJECTS (Table*, table, &tables)
		table->done = false;
	END_FOR;
}

void Database::createAll(NetfraDatabase * db)
{
	clear();
	FILE *log = fopen ("mcet.log", "w");

	FOR_OBJECTS (Table*, table, &tables)
		if (!table->done)
			{
			LinkedList list;
			table->findDependencies (list);
			FOR_OBJECTS_BACKWARD (Table*, tbl, &list)
				if (!tbl->isView && !tbl->fields.isEmpty())
					tbl->create (db);
			END_FOR;
			}
	END_FOR;

	if (log)
		fclose (log);
}

void Database::copyAll(NetfraDatabase * db)
{
	clear();
	FILE *log = fopen ("mcet.log", "w");

	FOR_OBJECTS (Table*, table, &tables)
		if (!table->done)
			{
			LinkedList list;
			table->findDependencies (list);
			FOR_OBJECTS_BACKWARD (Table*, tbl, &list)
				if (!tbl->isView && !tbl->fields.isEmpty())
					{
					if (tbl->populated (db))
						tbl->done = true;
					else
						{
						int count = tbl->copy (db);
						db->commit();
						if (log)
							{
							fprintf (log, "%s - %d records\n", (const char*) tbl->name, count);
							fflush (log);
							}
						}
					}
			END_FOR;
			}
	END_FOR;

	if (log)
		fclose (log);
}

void Database::setTrace(const char *logFile)
{
	int ret = SQLSetConnectAttr (NULL, SQL_ATTR_TRACE, (SQLPOINTER) SQL_OPT_TRACE_ON, NULL);
	ret = SQLSetConnectAttr (NULL, SQL_ATTR_TRACEFILE, (SQLPOINTER) logFile, SQL_NTS);
}
