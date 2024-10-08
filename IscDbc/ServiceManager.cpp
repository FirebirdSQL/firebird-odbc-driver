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
 *  The Original Code was created by Vladimir Tsvigun for IBPhoenix.
 *
 *  Copyright (c) 2005 Vladimir Tsvigun
 *  All Rights Reserved.
 */

// ServiceManager.cpp: implementation of the ServiceManager class.
//
//////////////////////////////////////////////////////////////////////

#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "IscDbc.h"
#include "IscConnection.h"
#include "SQLError.h"
#include "Parameters.h"
#include "../SetupAttributes.h"
#include "ServiceManager.h"

using namespace Firebird;

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern "C" ServiceManager* createServices()
{
	return new CServiceManager;
}

CServiceManager::CServiceManager()
{
	useCount = 1;
	GDS = NULL;
	properties = NULL;
	svcHandle = NULL;
}

CServiceManager::~CServiceManager()
{
	if( GDS && svcHandle ) {
		ThrowStatusWrapper status( GDS->_status );
		try {
			svcHandle->detach( &status );
		} catch( ... ) {
			svcHandle->release();
		}
		svcHandle = nullptr;
	}
	if( GDS )
		delete GDS;
}

Properties* CServiceManager::allocProperties()
{
	return new Parameters;
}

bool CServiceManager::attachServiceManager()
{
	char svcName[RESPONSE_BUFFER/12];
	const char *param;
	bool isServer = false;

	IUtil* utl = GDS->_master->getUtilInterface();
	ThrowStatusWrapper status( GDS->_status );
	IXpbBuilder* spb = nullptr;
	try
	{
		spb = utl->getXpbBuilder(&status, IXpbBuilder::SPB_ATTACH, NULL, 0);

		param = properties->findValue( SETUP_USER, NULL );
		spb->insertString(&status, isc_spb_user_name, param);

		param = properties->findValue( SETUP_PASSWORD, NULL );
		spb->insertString(&status, isc_spb_password, param);

		param = properties->findValue( "serverName", NULL );

		if ( param && *param )
		{
			//    TCP: ServerName + ":service_mgr"
			//    SPX: ServerName + "@service_mgr"
			//    Pipe: "\\\\" + ServerName + "\\service_mgr"
			//    Local: "service_mgr"
			sprintf( svcName, "%s:service_mgr", param );
			isServer = true;
		}
		else
			strcpy( svcName, "service_mgr" );
		
		svcHandle = GDS->_prov->attachServiceManager( &status, svcName, spb->getBufferLength(&status), spb->getBuffer(&status) );
		spb->dispose();
		spb = nullptr;
	}
	catch( const FbException& error )
	{
		if( spb ) spb->dispose();
		const ISC_STATUS * statusVector = error.getStatus()->getErrors();
		throw SQLEXCEPTION ( GDS->getSqlCode( statusVector ), statusVector [1], getIscStatusText( error.getStatus() ) );
	}
	return isServer;
}

void CServiceManager::startBackupDatabase( Properties *prop, ULONG options )
{
	const char *param;
	ULONG tempVal;
	properties = prop;	

	if ( !GDS )
		loadShareLibrary();

	bool isServer = attachServiceManager();

	IUtil* utl = GDS->_master->getUtilInterface();
	ThrowStatusWrapper status( GDS->_status );
	IXpbBuilder* spb = nullptr;
	try
	{
		spb = utl->getXpbBuilder(&status, IXpbBuilder::SPB_START, NULL, 0);

		spb->insertTag( &status, isc_action_svc_backup );

		param = properties->findValue( SETUP_DBNAME, NULL );
		if ( isServer )
			while ( *param++ != ':' );
		spb->insertString( &status, isc_spb_dbname, param );

		param = properties->findValue( "backupFile", NULL );
		spb->insertString( &status, isc_spb_bkp_file, param );

		if ( options )
		{
			spb->insertInt( &status, isc_spb_options, options );
		}

		spb->insertTag( &status, isc_spb_verbose );

		param = properties->findValue( "blockingFactor", "0" );
		tempVal = atol( param );
		if ( tempVal )
		{
			spb->insertInt( &status, isc_spb_bkp_factor, tempVal );
		}

		svcHandle->start( &status, spb->getBufferLength(&status), spb->getBuffer(&status) );
		spb->dispose();
		spb = nullptr;
	}
	catch( const FbException& error )
	{
		if( spb ) spb->dispose();
		const ISC_STATUS * statusVector = error.getStatus()->getErrors();
		throw SQLEXCEPTION ( GDS->getSqlCode( statusVector ), statusVector [1], getIscStatusText( error.getStatus() ) );
	}
}

void CServiceManager::startRestoreDatabase( Properties *prop, ULONG options )
{
	const char *param;
	ULONG sizeVal;

	properties = prop;

	if ( !GDS )
		loadShareLibrary();

	bool isServer = attachServiceManager();

	IUtil* utl = GDS->_master->getUtilInterface();
	ThrowStatusWrapper status( GDS->_status );
	IXpbBuilder* spb = nullptr;
	try
	{
		spb = utl->getXpbBuilder(&status, IXpbBuilder::SPB_START, NULL, 0);

		spb->insertTag( &status, isc_action_svc_restore );

		if ( !(options & isc_spb_res_replace) )
			options |= isc_spb_res_create;

		if ( options )
		{
			spb->insertInt( &status, isc_spb_options, options );
		}

		spb->insertTag( &status, isc_spb_verbose );

		param = properties->findValue( SETUP_PAGE_SIZE, "0" );
		sizeVal = atol( param );
		if ( sizeVal )
		{
			spb->insertInt( &status, isc_spb_res_page_size, sizeVal );
		}

		param = properties->findValue( "buffersSize", "0" );
		sizeVal = atol( param );
		if ( sizeVal )
		{
			spb->insertInt( &status, isc_spb_res_buffers, sizeVal );
		}

		param = properties->findValue( "backupFile", NULL );
		spb->insertString( &status, isc_spb_bkp_file, param );

		param = properties->findValue( SETUP_DBNAME, NULL );
		if ( isServer )
			while ( *param++ != ':' );
		spb->insertString( &status, isc_spb_dbname, param );
		
		svcHandle->start( &status, spb->getBufferLength(&status), spb->getBuffer(&status) );
		spb->dispose();
		spb = nullptr;
	}
	catch( const FbException& error )
	{
		if( spb ) spb->dispose();
		const ISC_STATUS * statusVector = error.getStatus()->getErrors();
		throw SQLEXCEPTION ( GDS->getSqlCode( statusVector ), statusVector [1], getIscStatusText( error.getStatus() ) );
	}
}

void CServiceManager::exitRestoreDatabase()
{
	IAttachment* databaseHandle = NULL;
	const char *param;

	if ( !GDS )
		loadShareLibrary();

	param = properties->findValue( "noReadOnly", NULL );
	if ( param && *param == 'N')
 	{
		IUtil* utl = GDS->_master->getUtilInterface();
		IXpbBuilder* dpb = nullptr;
		ThrowStatusWrapper status( GDS->_status );

		try
		{
			dpb = utl->getXpbBuilder( &status, IXpbBuilder::DPB, NULL, 0 );

			param = properties->findValue( SETUP_USER, NULL );
			dpb->insertString( &status, isc_dpb_user_name, param );

			param = properties->findValue( SETUP_PASSWORD, NULL );
			dpb->insertString( &status, isc_dpb_password, param );

			char ch = 1;
			dpb->insertBytes( &status, isc_dpb_set_db_readonly, &ch, 1 );

			param = properties->findValue( SETUP_DBNAME, NULL );
						
			databaseHandle = GDS->_prov->attachDatabase( &status, param, dpb->getBufferLength(&status), dpb->getBuffer(&status) );
			databaseHandle->detach( &status );
			dpb->dispose();
			dpb = nullptr;
		}
		catch( const FbException& error )
		{
			if( dpb ) dpb->dispose();

			if( databaseHandle ) try {
				databaseHandle->detach( &status );
			} catch( ... ) {
				databaseHandle->release();
			}

			const ISC_STATUS * statusVector = error.getStatus()->getErrors();
			throw SQLEXCEPTION ( GDS->getSqlCode( statusVector ), statusVector [1], getIscStatusText( error.getStatus() ) );
		}
	}
}

void CServiceManager::startStaticticsDatabase( Properties *prop, ULONG options )
{
	const char *param;
	properties = prop;

	if ( !GDS )
		loadShareLibrary();

	bool isServer = attachServiceManager();

	IUtil* utl = GDS->_master->getUtilInterface();
	ThrowStatusWrapper status( GDS->_status );
	IXpbBuilder* spb = nullptr;
	try
	{
		spb = utl->getXpbBuilder(&status, IXpbBuilder::SPB_START, NULL, 0);

		spb->insertTag( &status, isc_action_svc_db_stats );

		param = properties->findValue( SETUP_DBNAME, NULL );
		if ( isServer )
			while ( *param++ != ':' );
		spb->insertString( &status, isc_spb_dbname, param );

		if ( options )
		{
			spb->insertInt( &status, isc_spb_options, options );
		}

		svcHandle->start( &status, spb->getBufferLength(&status), spb->getBuffer(&status) );
		spb->dispose();
		spb = nullptr;
	}
	catch( const FbException& error )
	{
		if( spb ) spb->dispose();
		const ISC_STATUS * statusVector = error.getStatus()->getErrors();
		throw SQLEXCEPTION ( GDS->getSqlCode( statusVector ), statusVector [1], getIscStatusText( error.getStatus() ) );
	}
}

void CServiceManager::startShowDatabaseLog( Properties *prop )
{
	properties = prop;

	if ( !GDS )
		loadShareLibrary();
	
	bool isServer = attachServiceManager();

	IUtil* utl = GDS->_master->getUtilInterface();
	ThrowStatusWrapper status( GDS->_status );
	IXpbBuilder* spb = nullptr;
	try
	{
		spb = utl->getXpbBuilder(&status, IXpbBuilder::SPB_START, NULL, 0);
		spb->insertTag( &status, isc_action_svc_get_ib_log );		
		
		svcHandle->start( &status, spb->getBufferLength(&status), spb->getBuffer(&status) );
		spb->dispose();
		spb = nullptr;
	}
	catch( const FbException& error )
	{
		if( spb ) spb->dispose();
		const ISC_STATUS * statusVector = error.getStatus()->getErrors();
		throw SQLEXCEPTION ( GDS->getSqlCode( statusVector ), statusVector [1], getIscStatusText( error.getStatus() ) );
	}
}

void CServiceManager::startRepairDatabase( Properties *prop, ULONG options, ULONG optionsValidate )
{
	const char *param;
	properties = prop;

	if ( !GDS )
		loadShareLibrary();

	bool isServer = attachServiceManager();

	IUtil* utl = GDS->_master->getUtilInterface();
	ThrowStatusWrapper status( GDS->_status );
	IXpbBuilder* spb = nullptr;
	try
	{
		spb = utl->getXpbBuilder(&status, IXpbBuilder::SPB_START, NULL, 0);
		
		spb->insertTag( &status, isc_action_svc_repair );

		param = properties->findValue( SETUP_DBNAME, NULL );
		if ( isServer )
			while ( *param++ != ':' );
		spb->insertString( &status, isc_spb_dbname, param );

		if ( options )
		{
			spb->insertInt( &status, isc_spb_options, options );
		}

		if ( optionsValidate )
		{
			spb->insertInt( &status, isc_spb_options, optionsValidate );
		}
	
		svcHandle->start( &status, spb->getBufferLength(&status), spb->getBuffer(&status) );
		spb->dispose();
		spb = nullptr;
	}
	catch( const FbException& error )
	{
		if( spb ) spb->dispose();
		const ISC_STATUS * statusVector = error.getStatus()->getErrors();
		throw SQLEXCEPTION ( GDS->getSqlCode( statusVector ), statusVector [1], getIscStatusText( error.getStatus() ) );
	}
}

void CServiceManager::startUsersQuery( Properties *prop )
{
	const char *param;
	const char *paramUser;
	ULONG tempVal;
	properties = prop;

	if ( !GDS )
		loadShareLibrary();

	bool isServer = attachServiceManager();

	IUtil* utl = GDS->_master->getUtilInterface();
	ThrowStatusWrapper status( GDS->_status );
	IXpbBuilder* spb = nullptr;
	try
	{
		spb = utl->getXpbBuilder(&status, IXpbBuilder::SPB_START, NULL, 0);

		do
		{
			paramUser = properties->findValue( "userName", NULL );

			param = properties->findValue( "displayUser", NULL );
			if ( param && *param )
			{
				spb->insertTag( &status, isc_action_svc_display_user );
				if ( paramUser && *paramUser )
				{
					spb->insertString( &status, isc_spb_sec_username, paramUser );
				}
				break;
			}

			param = properties->findValue( "deleteUser", NULL );
			if ( param && *param )
			{
				spb->insertTag( &status, isc_action_svc_delete_user );
				if ( paramUser && *paramUser )
				{
					spb->insertString( &status, isc_spb_sec_username, paramUser );
				}
				break;
			}

			param = properties->findValue( "addUser", NULL );
			if ( param && *param )
			{
				spb->insertTag( &status, isc_action_svc_add_user );
			}
			else
			{
				param = properties->findValue( "modifyUser", NULL );
				if ( param && *param )
				{
					spb->insertTag( &status, isc_action_svc_modify_user );
				}
			}

			if ( paramUser && *paramUser )
			{
				spb->insertString( &status, isc_spb_sec_username, paramUser );
			}

			param = properties->findValue( "userPassword", NULL );
			if ( param && *param )
			{
				spb->insertString( &status, isc_spb_sec_password, param );
			}

			param = properties->findValue( "firstName", NULL );
			if ( param && *param )
			{
				spb->insertString( &status, isc_spb_sec_firstname, param );
			}
			param = properties->findValue( "middleName", NULL );
			if ( param && *param )
			{
				spb->insertString( &status, isc_spb_sec_middlename, param );
			}
			param = properties->findValue( "lastName", NULL );
			if ( param && *param )
			{
				spb->insertString( &status, isc_spb_sec_lastname, param );
			}

			param = properties->findValue( "groupId", NULL );
			if ( param && *param )
			{
				tempVal = atol( param );
				spb->insertInt( &status, isc_spb_sec_groupid, tempVal );
			}
			param = properties->findValue( "userId", NULL );
			if ( param && *param )
			{
				tempVal = atol( param );
				spb->insertInt( &status, isc_spb_sec_userid, tempVal );
			}

		} while ( false );

		svcHandle->start( &status, spb->getBufferLength(&status), spb->getBuffer(&status) );
		spb->dispose();
		spb = nullptr;
	}
	catch( const FbException& error )
	{
		if( spb ) spb->dispose();
		const ISC_STATUS * statusVector = error.getStatus()->getErrors();
		throw SQLEXCEPTION ( GDS->getSqlCode( statusVector ), statusVector [1], getIscStatusText( error.getStatus() ) );
	}
}

bool CServiceManager::nextQuery( char *outBuffer, int lengthOut, int &lengthRealOut, int &countError )
{
	char sendBuffer[] = { isc_info_svc_line };
	char respBuffer[RESPONSE_BUFFER];
	char *resp = respBuffer;
	int length = lengthOut;
	int offset;
	bool nextQuery = false;
	CheckStatusWrapper status( GDS->_status );

	do
	{
		svcHandle->query( &status, 0, NULL, 
		                  sizeof( sendBuffer ), (const unsigned char*)sendBuffer,
						  RESPONSE_BUFFER, (unsigned char*)respBuffer );

		char *p = respBuffer;

		offset = 0;
		nextQuery = *p == isc_info_svc_line;

		if ( status.getState() & IStatus::STATE_ERRORS )
		{
			const ISC_STATUS * statusVector = status.getErrors();
			if ( !nextQuery )
				throw SQLEXCEPTION( GDS->getSqlCode( statusVector ), statusVector[1], getIscStatusText( &status ) );
			++countError;
		}

		ISC_USHORT len = (ISC_USHORT)GDS->_vax_integer( ++p, sizeof ( ISC_USHORT ) );
		p += sizeof ( ISC_USHORT );
		if ( !len )
		{
			if ( *p ==  isc_info_data_not_ready )
			{
				length = snprintf( &outBuffer[offset], lengthOut, "\n%s", "no data available at this moment" );
				lengthOut -= length;
				offset += length;
				break;
			}
			else
			{
				if ( *p != isc_info_end )
				{
					length = snprintf( &outBuffer[offset], lengthOut, "\nFormat error ... <%d>", *p );
					lengthOut -= length;
					offset += length;
					++countError;
				}
				nextQuery = false;
				break;
			}
		}
		
		length = snprintf( &outBuffer[offset], lengthOut, "%.*s", len, p );
		lengthOut -= length;
		offset += length;

		p += len;

		if ( *p != isc_info_truncated && *p != isc_info_end )
		{
			length = snprintf( &outBuffer[offset], lengthOut, "\nFormat error ... encountered <%d>", *p );
			lengthOut -= length;
			offset += length;
			++countError;
		}

	} while ( false );
	
	lengthRealOut = offset;
	return nextQuery;
}

bool CServiceManager::nextQueryLimboTransactionInfo( char *outBuffer, int lengthOut, int &lengthRealOut )
{
	char sendBuffer[] = { isc_info_svc_limbo_trans };
	char respBuffer[RESPONSE_BUFFER];
	char *resp = respBuffer;
	int length = lengthOut;
	int offset;
	bool nextQuery = false;
	CheckStatusWrapper status( GDS->_status );

	do
	{
		svcHandle->query( &status, 0, NULL, 
		                  sizeof( sendBuffer ), (const unsigned char*)sendBuffer,
						  RESPONSE_BUFFER, (unsigned char*)respBuffer );

		if ( status.getState() & IStatus::STATE_ERRORS )
		{
			const ISC_STATUS * statusVector = status.getErrors();
			if ( !nextQuery )
				throw SQLEXCEPTION( GDS->getSqlCode( statusVector ), statusVector[1], getIscStatusText( &status ) );
		}

		char *p = respBuffer;

		offset = 0;
		nextQuery = *p == isc_info_svc_limbo_trans;

		ISC_USHORT len = (ISC_USHORT)GDS->_vax_integer( ++p, sizeof ( ISC_USHORT ) );
		p += sizeof ( ISC_USHORT );
		if ( !len )
		{
			if ( *p ==  isc_info_data_not_ready )
			{
				length = snprintf( &outBuffer[offset], lengthOut, "\n%s", "no data available at this moment" );
				lengthOut -= length;
				offset += length;
				break;
			}
			else
			{
				if ( *p != isc_info_end )
				{
					length = snprintf( &outBuffer[offset], lengthOut, "\nFormat error ... <%d>", *p );
					lengthOut -= length;
					offset += length;
				}
				nextQuery = false;
				break;
			}
		}
		
		length = snprintf( &outBuffer[offset], lengthOut, "%.*s", len, p );
		lengthOut -= length;
		offset += length;

		p += len;

		if ( *p != isc_info_truncated && *p != isc_info_end )
		{
			length = snprintf( &outBuffer[offset], lengthOut, "\nFormat error ... encountered <%d>", *p );
			lengthOut -= length;
			offset += length;
		}

	} while ( false );
	
	lengthRealOut = offset;
	return nextQuery;
}

bool CServiceManager::nextQueryUserInfo( char *outBuffer, int lengthOut, int &lengthRealOut )
{
	char sendBuffer[] = { isc_info_svc_get_users };
	char respBuffer[RESPONSE_BUFFER];
	char *resp = respBuffer;
	char *out = outBuffer;
	UserInfo *info = NULL;
	int length = lengthOut;
	int offset;
	bool nextQuery = false;
	CheckStatusWrapper status( GDS->_status );

	do
	{
		svcHandle->query( &status, 0, NULL, 
		                  sizeof( sendBuffer ), (const unsigned char*)sendBuffer,
						  RESPONSE_BUFFER, (unsigned char*)respBuffer );

		if ( status.getState() & IStatus::STATE_ERRORS )
		{
			const ISC_STATUS * statusVector = status.getErrors();
			if ( !nextQuery )
				throw SQLEXCEPTION( GDS->getSqlCode( statusVector ), statusVector[1], getIscStatusText( &status ) );
		}

		char *p = respBuffer;

		offset = 0;
		nextQuery = *p == isc_info_svc_get_users;

		ISC_USHORT len = (ISC_USHORT)GDS->_vax_integer( ++p, sizeof ( ISC_USHORT ) );
		p += sizeof ( ISC_USHORT );
		if ( !len )
		{
			if ( *p ==  isc_info_data_not_ready )
			{
				length = snprintf( &outBuffer[offset], lengthOut, "\n%s", "no data available at this moment" );
				lengthOut -= length;
				offset += length;
				break;
			}
			else
			{
				if ( *p != isc_info_end )
				{
					length = snprintf( &outBuffer[offset], lengthOut, "\nFormat error ... <%d>", *p );
					lengthOut -= length;
					offset += length;
				}
				nextQuery = false;
				break;
			}
		}
		
        while ( *p != isc_info_end )
        {
			switch ( *p++ )
			{
			case isc_spb_sec_username:
				if ( !info )
				{
					info = (UserInfo*)out;
					memset( info, 0, sizeof ( UserInfo ) );
				}
				else
				{
					info->next = (UserInfo*)out;
					info = info->next;
					memset( info, 0, sizeof ( UserInfo ) );
				}

				out += sizeof ( UserInfo );

				len = (ISC_USHORT)GDS->_vax_integer( p, sizeof ( ISC_USHORT ) );
				p += sizeof ( ISC_USHORT );
				strncpy( out, p, len );
				p += len;
				info->userName = out;
				out += len;
				*out++ = '\0';
				lengthOut -= len + 1;
				break;
            
			case isc_spb_sec_firstname:
				len = (ISC_USHORT)GDS->_vax_integer( p, sizeof ( ISC_USHORT ) );
				p += sizeof ( ISC_USHORT );
				strncpy( out, p, len );
				p += len;
				info->firstName = out;
				out += len;
				*out++ = '\0';
				lengthOut -= len + 1;
				break;
            
			case isc_spb_sec_middlename:
				len = (ISC_USHORT)GDS->_vax_integer( p, sizeof ( ISC_USHORT ) );
				p += sizeof( ISC_USHORT );
				strncpy( out, p, len );
				p += len;
				info->middleName = out;
				out += len;
				*out++ = '\0';
				lengthOut -= len + 1;
				break;
            
			case isc_spb_sec_lastname:
				len = (ISC_USHORT)GDS->_vax_integer( p, sizeof ( ISC_USHORT ) );
				p += sizeof ( ISC_USHORT );
				strncpy( out, p, len );
				p += len;
				info->lastName = out;
				out += len;
				*out++ = '\0';
				lengthOut -= len + 1;
				break;
            
			case isc_spb_sec_groupid:
				info->groupId = GDS->_vax_integer( p, sizeof ( ISC_ULONG ) );
				p += sizeof ( ISC_ULONG );
				break;
            
			case isc_spb_sec_userid:
				info->userId = GDS->_vax_integer( p, sizeof ( ISC_ULONG ) );
				p += sizeof ( ISC_ULONG );
				break;
			}
        }

		offset = out - outBuffer;

		if ( *p != isc_info_truncated && *p != isc_info_end )
		{
			length = snprintf( &outBuffer[offset], lengthOut, "\nFormat error ... encountered <%d>", *p );
			lengthOut -= length;
			offset += length;
		}

	} while ( false );
	
	lengthRealOut = offset;
	return nextQuery;
}

void CServiceManager::closeService()
{
	if ( svcHandle ) {

		ThrowStatusWrapper status( GDS->_status );

		try
		{
			svcHandle->detach( &status );
			svcHandle = nullptr;
		}
		catch( ... )
		{
			svcHandle->release();
			svcHandle = nullptr;
		}

		unloadShareLibrary();
	}
}

int	CServiceManager::getDriverBuildKey()
{
	return DRIVER_BUILD_KEY;
}

void CServiceManager::loadShareLibrary()
{
	const char *clientDefault = NULL;
	const char *client = properties->findValue( SETUP_CLIENT, NULL );

	if ( !client || !*client )
		client = NAME_CLIENT_SHARE_LIBRARY,
		clientDefault = NAME_DEFAULT_CLIENT_SHARE_LIBRARY;

	GDS = new CFbDll();
	if ( !GDS->LoadDll( client, clientDefault ) )
	{
		JString text;
		text.Format( "Unable to connect to data source: library '%s' failed to load", client );
		throw SQLEXCEPTION( -904, 335544375l, text );
	}
}

void CServiceManager::unloadShareLibrary()
{
	if ( GDS )
	{
		GDS->Release();
		GDS = NULL;
	}
}

void CServiceManager::addRef()
{
	++useCount;
}

int CServiceManager::release()
{
	if ( !--useCount )
		delete this;

	return useCount;
}

}; // end namespace IscDbcLibrary
