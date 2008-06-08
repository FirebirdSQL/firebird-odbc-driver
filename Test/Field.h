/*
 *	PROGRAM:		Virtual Data Manager
 *	MODULE:			Field.h
 *	DESCRIPTION:	Virtual Field class
 *
 * copyright (c) 1997 by James A. Starkey for IBPhoenix.
 */

#ifndef __FIELD_H
#define __FIELD_H

#include "Gen.h"
#include "Type.h"
#include "LinkedList.h"

class Database;
class Table;
class CString;
class Syntax;
class Constraint;
struct FieldInfo;
class CGenHtmlTable;
class CProperty;
class Index;

class Field : public Gen, public CType {
    public:
	    Index* findForeignReference (Database *database);
	    virtual CString genSql();
	    void addReference (CProperty* property);
	    void clearXRef();
	    void genDocumentation (CGenHtmlTable *table);
	    void deleteReference();
	    void addReference();
	    void clearReferences();
		Field ();
		Field (Table *table, FieldInfo *info);
		Field (Table *table, const char *name, Type type, int length,
			   boolean nullable, boolean primary_key,
			   const char *comment, Field *domain);
		~Field();

	    bool				keyMatch (Field *field);
	    bool				matches (const char *string);

		boolean				changed (Field *field);
		virtual void		freeze();
		virtual void		thaw();
	    virtual CString		genSqlString();
		virtual CString		gen (boolean empty = TRUE);
		Field				*getDomain();
		Table				*getTable();
	    Database*			getDatabase();
		Type			getType();
		int					getLength();
		int					getScale();
		int					getPrecision();
	    //VARTYPE				getVariantType();
		virtual boolean		isDeleteable();
		virtual boolean		isFrozen();
		boolean				isLargeObject();
		boolean				isKey();
		boolean				isNullable();
		boolean				isPrimaryKey();
	    bool				isUnique();
		boolean				modified (const char *newName, Type newType,
									  int newLength, boolean nullable,
									  boolean primaryKey, const char *comment,
									  Field *newDomain);
		void				postLoad();

		static Field		*findDomain (const char *name);
		static void			fini();

		CString		defaultValue;
		int			primaryKey;
		CString		typeName;
		int			referenceCount;
		Index		*reference;

		//static Type		typeFromOdbcType (int type, const char *typeName);

	protected:
		//DataType	type;
		//int			length, precision, scale;
		Table		*table;
		boolean		notNull;
		Field		*domain;
		Constraint	*foreignKey;
		int			frozen;
		LinkedList	references;

	static Hash	domains;
	};



#endif
