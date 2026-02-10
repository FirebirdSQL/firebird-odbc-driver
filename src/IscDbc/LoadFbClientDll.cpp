#include "IscDbc.h"
#ifndef _WINDOWS
#include <dlfcn.h>
#include <stdio.h>
#endif

using namespace Firebird;

namespace IscDbcLibrary {

CFbDll::CFbDll() :
	_isMsAccess{ detectMsAccess() },
	_master{nullptr},
	_prov{nullptr},
	_status{nullptr}
{ 
	_Handle = NULL;
}

CFbDll::~CFbDll() 
{ 
	Release();
}

bool CFbDll::LoadDll (const char * client, const char * clientDef)
{
#ifdef _WINDOWS
	_Handle = LoadLibraryEx (client, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if ( !_Handle && clientDef )
		_Handle = LoadLibraryEx (clientDef, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
#else
	_Handle = dlopen (client, RTLD_NOW);
	if ( !_Handle && clientDef ) {
		_Handle = dlopen (clientDef, RTLD_NOW);
		if ( !_Handle )
			fputs (dlerror(), stderr);
	}
#endif
	if ( !_Handle )
		return false;

	_CFbDllVersion = 12;

#ifdef _WINDOWS
#define __ENTRYPOINT(X) _##X = (X*)GetProcAddress(_Handle, "isc_"#X)
#define __ENTRYPOINT_OOAPI(X) _##X = (X*)GetProcAddress(_Handle, "fb_"#X)
#else
#define __ENTRYPOINT(X) _##X = (X*)dlsym(_Handle, "isc_"#X)
#define __ENTRYPOINT_OOAPI(X) _##X = (X*)dlsym(_Handle, "fb_"#X)
#endif					

	// Active ISC function pointers only (Phase 9.3: removed ~35 dead entries)

	// Array operations (no OO API equivalent)
	__ENTRYPOINT(array_get_slice);
	__ENTRYPOINT(array_put_slice);
	__ENTRYPOINT(array_lookup_bounds);

	// Error handling
	__ENTRYPOINT(sqlcode);

	// BLR parsing
	__ENTRYPOINT(print_blr);

	/* OOAPI */
	__ENTRYPOINT_OOAPI(get_master_interface);
	__ENTRYPOINT_OOAPI(get_transaction_handle);
	__ENTRYPOINT_OOAPI(get_database_handle);

	_master = _get_master_interface();
	_prov   = _master->getDispatcher();
	_status = _master->getStatus();

	return true;
}

void CFbDll::Release(void)
{
	if( _prov ) {

		CheckStatusWrapper status( _status );
		try {
			//TODO: for some reasons this code doesnt't work properly
			//_prov->shutdown( &status, 0, fb_shutrsn_app_stopped );
		} catch ( ... ) {
			//TODO: some logging?..
		}

		_master->getPluginManager()->releasePlugin( _prov );
		_prov = nullptr;
	}

	if( _status ) {
		_status->dispose();
		_status = nullptr;
	}

	_master = nullptr;

//  Do not remove the comment!!!
//  OdbcFb this intermediate link

	if ( _Handle )
#ifdef _WINDOWS
		FreeLibrary(_Handle);
#else
		dlclose (_Handle);
#endif

	_Handle = NULL;
}

}; // end namespace IscDbcLibrary
