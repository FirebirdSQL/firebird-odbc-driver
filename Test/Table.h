/*
 *	PROGRAM:		Virtual Data Manager
 *	MODULE:			Table.h
 *	DESCRIPTION:	Virtual Table class
 *
 * copyright (c) 1997 by James A. Starkey for IBPhoenix.
 */

#ifndef __TABLE_H
#define __TABLE_H

#include "Gen.h"
#include "LinkedList.h"
#include "Field.h"

class Index;
class Database;
class Peer;
class Database;
class CProject;
class NetfraDatabase;

struct FieldInfo {
	char	fieldName [128];
	char	typeName [128];
	long	length;
	long	dtype;
	long	nullable;
	long	precision;
	long	scale;
public:
	void print (const char *prefix);
    };

class Table : public Gen
	{
    public:
	    bool populated (NetfraDatabase *db);
	    void create (NetfraDatabase *db);
	    int copy (NetfraDatabase *db);
	    void findDependencies (LinkedList &list);
	    void findReferences();
	    virtual CString genSql();
	    void clearXRef();
	    void genDocumentation (CProject*, CString directory);
	    CString getURL();
	    void clearReferences();
	    Field* findIndexField (Field *field);
	    Field *findPrimaryKey (Field *field);
	    bool isUnique (Field *field);
	    LinkedList* getFields();
		LinkedList*	getIndexes ();

	Table ();
	//Table (Syntax*);
	Table (Database*, const char *qualifier, const char* owner, const char *name, bool view);
	~Table();

	Field			*addField (const char *name, Type type, int length,
							   boolean nullable, boolean primary_key,
							   const char *comment, Field *domain);
	void			addOrdered (LinkedList *orderedTables);
	boolean			addIndex (Index*);
	void			deleteChild (Index*);
	void			deleteChild (Field*);
    virtual Field	*findField (const char *name);
    //virtual Field	*findField (Syntax *syntax);
    Index			*findIndex (const char *name);

	static Table	*findTable (const char *name);
	static Table	*findTable (Syntax *syntax);
	static void		fini();

	virtual void	freeze();
	virtual CString	gen (boolean empty = TRUE);
	virtual CString	genUpgrade ();
	void			genUpgradeIndexes ();
	//void			getCurrent ();
	CString			getFullName ();
	virtual boolean	isDeleteable();
	boolean			isFirstLargeObject (Field*);
	virtual boolean	isFrozen();
	boolean			isKey (Field*);
	void			postLoad();
	void			resetOrdered();
	virtual void	thaw();

	CString		owner;
	CString		qualifier;
	CString		identifier;
	LinkedList	fields;
	LinkedList	indexes;
	LinkedList	constraints;
	int			entered;
	Table		*current;
	int			frozen;
	Database	*database;
	Index		*primaryKey;
	bool		isView;
	bool		done;

	static Hash	tables;
	};

#endif
