// PDatabase.h: interface for the CPDatabase class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PDATABASE_H__7427858B_AAF8_11D1_AB1B_0000C01D2301__INCLUDED_)
#define AFX_PDATABASE_H__7427858B_AAF8_11D1_AB1B_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <afxdb.h>
#include "LinkedList.h"	// Added by ClassView

#define PK_HASH_SIZE		101

class Table;
class CProject;
class Index;
class Field;
class NetfraDatabase;

class Database
{
public:
	void setTrace (const char *logFile);
	void copyAll (NetfraDatabase *db);
	void createAll (NetfraDatabase *db);
	void clear();
	Index* findPrimaryKey (Field *field);
	void clearXRef();
	void genDocumentation (CProject *project, CString directory);
	CString getURL();
	void*		allocStatement();
	void		clearReferences();
	void		countObjects();
	Table*		findTable (const char *name);
	void		freeStatement (void* statement);
	CString		getConnectOption (const char *option);
	LinkedList* getTables();
	LinkedList* getProcedures();
	CString		GetConnect();
	operator	CDatabase*();
	CString		rewriteConnectString (const char*string);
	virtual BOOL OpenEx (LPCTSTR connectString, DWORD dwOptions=0);
	void		setName (CString& newName);

	Database();
	virtual ~Database();
	CString	name;

	CString	connectString;
	bool	primaryKeySupport;

protected:
	int			statements;
	LinkedList	tables;
	LinkedList	procedures;
	CDatabase	*database;

	Index		*primaryKeys [PK_HASH_SIZE];
};

#endif // !defined(AFX_PDATABASE_H__7427858B_AAF8_11D1_AB1B_0000C01D2301__INCLUDED_)
