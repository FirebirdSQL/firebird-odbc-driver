// NetfraDatabase.h: interface for the NetfraDatabase class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NETFRADATABASE_H__CBD41C52_EAEA_11D3_98D6_0000C01D2301__INCLUDED_)
#define AFX_NETFRADATABASE_H__CBD41C52_EAEA_11D3_98D6_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class Connection;

class NetfraDatabase  
{
public:
	void rollback();
	void commit();
	void createDatabase();
	void findDatabase();
	void executeUpdate (const char *sql);
	void attach();
	void Serialize(CArchive& ar);
	NetfraDatabase();
	virtual ~NetfraDatabase();

	Connection *connection;
	CString		connectString;
	CString		password;
	CString		schema;
	CString		account;
	int			savePassword;
};

#endif // !defined(AFX_NETFRADATABASE_H__CBD41C52_EAEA_11D3_98D6_0000C01D2301__INCLUDED_)
