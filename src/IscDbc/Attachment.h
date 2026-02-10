/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *     Software distributed under the License is distributed on 
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *     express or implied.  See the License for the specific 
 *     language governing rights and limitations under the License.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 */

// Attachment.h: interface for the Attachment class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ATTACHMENT_H_)
#define _ATTACHMENT_H_

#include "Mutex.h"

namespace IscDbcLibrary {

using namespace classMutex;

class Properties;

/// @brief Manages a Firebird database attachment (connection to a Firebird server).
///
/// Wraps the Firebird OO API IAttachment handle and tracks server version,
/// character set, page size, and other connection-level metadata.
/// Reference-counted via addRef()/release().
class Attachment  
{
public:
	bool isAdmin();
	void checkAdmin();
	JString& getUserAccess();
	int getUserType();
	int getDatabaseDialect();
	inline int getUseSchemaIdentifier();
	inline int getUseLockTimeoutWaitTransactions();
	inline JString getIscStatusText ( Firebird::IStatus* status ) { return GDS->getIscStatusText( status ); }
	int release();
	void addRef();
	void loadClientLiblary( Properties *properties );
	bool isFirebirdVer2_0(){ return majorFb == 2; }
	/// Return the server major version (e.g. 5 for Firebird 5.0).
	int getMajorVersion() const { return majorFb; }
	/// Return the server minor version (e.g. 0 for Firebird 5.0).
	int getMinorVersion() const { return minorFb; }
	/// Check if the server is at least the given version (e.g. isVersionAtLeast(4, 0) for FB4+).
	bool isVersionAtLeast(int major, int minor = 0) const { return (majorFb > major) || (majorFb == major && minorFb >= minor); }
	void createDatabase(const char *dbName, Properties *properties);
	void openDatabase(const char * dbName, Properties * properties);
	Attachment();
	~Attachment();

	CFbDll		*GDS;
	Firebird::IAttachment* databaseHandle;
	Firebird::ITransaction* transactionHandle; // for two phase

	JString		dsn;
	JString		databaseName;
	JString		databaseServerName;
	JString		databaseNameFromServer;
	JString		userName;
	JString		userAccess;
	int			userType;
	JString		serverVersion;
	JString		databaseProductName;
	int			majorFb;
	int			minorFb;
	int			versionFb;
	int			charsetCode;
	int			pageSize;
	int			connectionTimeout;
	int			serverBaseLevel;	
	int			databaseDialect;
	int			useSchemaIdentifier;
	int			useLockTimeoutWaitTransactions;
	bool		quotedIdentifier;
	bool		sensitiveIdentifier;
	bool		autoQuotedIdentifier;
	int			databaseAccess;
	int			transactionIsolation;
	int			useCount;
	bool		admin;
	bool		isRoles;
	Mutex		mutex;
};

inline
int Attachment::getUseSchemaIdentifier()
{
	return useSchemaIdentifier;
}

inline
int Attachment::getUseLockTimeoutWaitTransactions()
{
	return useLockTimeoutWaitTransactions;
}

}; // end namespace IscDbcLibrary

#endif // !defined(_ATTACHMENT_H_)
