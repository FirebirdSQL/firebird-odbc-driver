#pragma once

// FbClient.h — Modern wrapper around the Firebird client library.
//
// Phase 14.2.1: Replaces CFbDll for fbclient loading.
// Loads the native fbclient shared library, initializes the Firebird OO API,
// creates an fbcpp::Client instance, and exposes legacy ISC function pointers
// needed for array operations (which have no OO API equivalent).

#include "LoadFbClientDll.h"
#include <fb-cpp/Client.h>
#include <memory>

namespace IscDbcLibrary {

/// Modern Firebird client wrapper combining fbcpp::Client with legacy ISC functions.
///
/// Usage:
///   FbClient* client = new FbClient();
///   client->LoadDll("fbclient", nullptr);
///   fbcpp::Client& fbClient = client->getClient();
///   // Use fbClient for modern API, or client->_array_* for legacy ISC array ops
class FbClient
{
private:
	bool	_isMsAccess;
	bool	detectMsAccess()
	{
#ifdef _WINDOWS
		try
		{
			char buf[1024] = {};
			auto n = GetModuleFileName(NULL, buf, sizeof(buf));
			if (!n) return false;

			std::filesystem::path fpath = std::string(buf, n);

			{
				std::stringstream ss;
				ss << "Loaded from: " << fpath << "\n";
				OutputDebugString(ss.str().c_str());
			}

			auto stem = fpath.stem().string();
			auto extn = fpath.extension().string();
			std::transform(stem.begin(), stem.end(), stem.begin(), [](unsigned char c) { return std::toupper(c); });
			std::transform(extn.begin(), extn.end(), extn.begin(), [](unsigned char c) { return std::toupper(c); });

			bool res = stem.find("MSACCESS") != std::string::npos && extn == ".EXE";
			{
				if (res) OutputDebugString("MS Access detected! Special patch will be applied.\n");
			}
			return res;
		}
		catch (...)
		{
			OutputDebugString("Unknown error in detectMsAccess().\n");
			return false;
		}

#else
		return false;
#endif
	}

public:
	FbClient();
	~FbClient();

	bool LoadDll(const char * client, const char * clientDef);
	void Release(void);

	/// Access the fb-cpp Client instance (available after LoadDll succeeds).
	fbcpp::Client& getClient()
	{
		return *client_;
	}

#ifdef _WINDOWS
	HMODULE		_Handle;
#else
	void		*_Handle;
#endif

	int _CFbDllVersion; 		

	// Active ISC function pointers (array, events, error, BLR only)
	array_lookup_bounds*		_array_lookup_bounds;
	array_get_slice*			_array_get_slice;
	array_put_slice*			_array_put_slice;

	sqlcode*					_sqlcode;

	print_blr*					_print_blr;

	/* OOAPI */
	get_master_interface*		_get_master_interface;
	get_transaction_handle*		_get_transaction_handle;
	get_database_handle*		_get_database_handle;

public:
    Firebird::IMaster*		_master;
	Firebird::IProvider*	_prov;
	Firebird::IStatus*		_status;

	/// Format error text from OO API IStatus (modern path).
	inline classJString::JString getIscStatusText( Firebird::IStatus* status )
	{
		char text [4096];
		_master->getUtilInterface()->formatStatus( text, sizeof(text), status );
		return text;
	}

	/// Format error text from raw ISC_STATUS[] vector (Phase 9.7: unified path).
	inline classJString::JString getIscStatusTextFromVector( const ISC_STATUS* statusVector )
	{
		Firebird::IStatus* tmpStatus = _master->getStatus();
		tmpStatus->setErrors( statusVector );
		char text [4096];
		_master->getUtilInterface()->formatStatus( text, sizeof(text), tmpStatus );
		tmpStatus->dispose();
		return text;
	}

	inline ISC_LONG getSqlCode( const ISC_STATUS* ev ) { return this->_sqlcode( const_cast<ISC_STATUS*>( ev ) ); }

	inline bool isMsAccess() { return _isMsAccess; }

private:
	std::unique_ptr<fbcpp::Client> client_;
};

// Backward compatibility alias — existing code uses CFbDll throughout.
// New code should use FbClient directly.
using CFbDll = FbClient;

}; // end namespace IscDbcLibrary
