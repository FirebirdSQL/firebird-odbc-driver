// NetfraDatabase.cpp: implementation of the NetfraDatabase class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
//#include "MCET.h"
#include "NetfraDatabase.h"
#include "Connection.h"
//#include "ConnectDialog.h"
//#include "CreateDialog.h"
#include "SQLException.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

NetfraDatabase::NetfraDatabase()
{
	connection = NULL;
	savePassword = false;
}

NetfraDatabase::~NetfraDatabase()
{
	if (connection)
		connection->close();	
}

void NetfraDatabase::Serialize(CArchive & ar)
{
	if (ar.IsStoring())
		{
		ar << connectString;
		ar << account;
		ar << savePassword;
		if (savePassword)
			ar << password;
		ar << schema;
		}
	else
		{
		ar >> connectString;
		ar >> account;
		ar >> savePassword;
		if (savePassword)
			ar >> password;
		ar >> schema;
		attach();
		}
}

void NetfraDatabase::attach()
{
	if (connection)
		{
		connection->close();
		connection = NULL;
		}

	for (;; savePassword = false)
		{
		/***
		if (!savePassword)
			{
			CConnectDialog dialog;
			dialog.m_connect_string = connectString;
			dialog.m_account = account;
			dialog.m_schema = schema;
			if (dialog.DoModal() != IDOK)
				return;
			connectString = dialog.m_connect_string;
			account = dialog.m_account;
			schema = dialog.m_schema;
			password = dialog.m_password;
			savePassword = dialog.m_save_passwd;
			}
		***/
		try
			{
			connection = createConnection();
			connection->openDatabase (connectString, account, password);
			CString sql = "alter namespace push " + schema;
			executeUpdate (sql);
			return;
			}
		catch (SQLException& exception)
			{
			AfxMessageBox (exception.getText());
			}
		}
}

void NetfraDatabase::executeUpdate (const char * sql)
{
	Statement *statement = connection->createStatement();
	statement->executeUpdate (sql);
	statement->close();
}

void NetfraDatabase::findDatabase()
{
	/***
	CConnectDialog dialog;
	dialog.m_connect_string = connectString;
	dialog.m_account = account;
	dialog.m_schema = schema;

	if (dialog.DoModal() == IDOK)
		{
		if (connection)
			connection->close();
		connection = createConnection();
		connection->openDatabase (dialog.m_connect_string, dialog.m_account, dialog.m_password);
		connectString = dialog.m_connect_string;
		account = dialog.m_account;
		password = dialog.m_password;
		schema = dialog.m_schema;
		savePassword = dialog.m_save_passwd;
		}
	***/
	savePassword = false;
	attach();
}

void NetfraDatabase::createDatabase()
{
	/***
	CCreateDialog dialog;

	if (dialog.DoModal() == IDOK)
		{
		if (connection)
			connection->close();
		try
			{
			connection = createConnection();
			connection->createDatabase (dialog.m_host_name, dialog.m_database_name, dialog.m_file_name,
										dialog.m_account, dialog.m_password);
			connectString = dialog.m_database_name + "@" + dialog.m_host_name;
			account = dialog.m_account;
			password = dialog.m_password;
			schema = dialog.m_schema;
			savePassword = dialog.m_save_password;
			CString sql = "alter namespace push " + schema;
			executeUpdate (sql);
			}
		catch (SQLException *exception)
			{
			AfxMessageBox (exception->getText());
			delete exception;
			}
		}
	***/

}

void NetfraDatabase::commit()
{
	connection->commit();
}

void NetfraDatabase::rollback()
{
	connection->rollback();
}
