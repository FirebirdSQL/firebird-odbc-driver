#ifndef __LOAD_FB_CLIENT_DLL__
#define __LOAD_FB_CLIENT_DLL__

// LoadFbClientDll.h — ISC API typedefs and backward-compatible CFbDll alias.
//
// Phase 14.2.1: The CFbDll class has been replaced by FbClient (in FbClient.h).
// This header is kept for:
//   1. ISC function pointer typedefs (used by IscArray, extodbc, etc.)
//   2. fb_vax_integer inline helper
//   3. Backward compatibility: `using CFbDll = FbClient;` so existing code compiles

#ifdef _WINDOWS
#include <windows.h>
#endif
#include <sstream>

#include <filesystem>
#include <algorithm>

namespace IscDbcLibrary {

// ============================================================
// ISC API typedefs — only functions still actively used
// Phase 9.3: Removed ~35 dead ISC function pointer typedefs.
// Kept: array ops, events, error/BLR, OO API bridge handles.
// ============================================================

// Array operations (no OO API equivalent exists)
typedef ISC_STATUS ISC_EXPORT array_get_slice(ISC_STATUS ISC_FAR*,
						isc_db_handle ISC_FAR*,
						isc_tr_handle ISC_FAR*,
						ISC_QUAD ISC_FAR*,
						ISC_ARRAY_DESC ISC_FAR*,
						void ISC_FAR*,
						ISC_LONG ISC_FAR*);

typedef ISC_STATUS ISC_EXPORT array_lookup_bounds(ISC_STATUS ISC_FAR*,
						isc_db_handle ISC_FAR*,
						isc_tr_handle ISC_FAR*,
						char ISC_FAR*,
						char ISC_FAR*,
						ISC_ARRAY_DESC ISC_FAR*);

typedef ISC_STATUS ISC_EXPORT array_put_slice(ISC_STATUS ISC_FAR*,
						isc_db_handle ISC_FAR*,
						isc_tr_handle ISC_FAR*,
						ISC_QUAD ISC_FAR*,
						ISC_ARRAY_DESC ISC_FAR*,
						void ISC_FAR*,
						ISC_LONG ISC_FAR*);

// Error handling (used by THROW_ISC_EXCEPTION macro chain)
typedef ISC_LONG    ISC_EXPORT sqlcode (ISC_STATUS ISC_FAR *);

// BLR parsing (used by IscProceduresResultSet)
typedef void        ISC_EXPORT print_blr(char ISC_FAR*,
					isc_callback,
					void ISC_FAR*,
					short);

/* OOAPI */

typedef Firebird::IMaster* ISC_EXPORT get_master_interface();
typedef ISC_STATUS ISC_EXPORT get_transaction_handle(ISC_STATUS* userStatus, isc_tr_handle* handle, void* obj);
typedef ISC_STATUS ISC_EXPORT get_database_handle(ISC_STATUS* userStatus, isc_db_handle* handle, void* obj);

// ============================================================
// Inline replacement for isc_vax_integer (Phase 9.9)
// Converts bytes in VAX (little-endian) format to a host integer.
// ============================================================
static inline ISC_LONG fb_vax_integer(const char* ptr, short length)
{
	ISC_LONG value = 0;
	for (short i = 0; i < length; ++i)
		value |= static_cast<ISC_LONG>(static_cast<unsigned char>(ptr[i])) << (i * 8);
	return value;
}

}; // end namespace IscDbcLibrary

// Phase 14.2.1: FbClient class definition (replaces CFbDll).
// CFbDll is now a type alias for FbClient — see FbClient.h.
#include "FbClient.h"

#endif // __LOAD_FB_CLIENT_DLL__
