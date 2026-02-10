#ifndef __LOAD_FB_CLIENT_DLL__
#define __LOAD_FB_CLIENT_DLL__

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

class CFbDll
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
	CFbDll();
	~CFbDll();

	bool LoadDll(const char * client, const char * clientDef);
	void Release(void);

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
	/// Creates a temporary IStatus, populates it from the legacy vector,
	/// then uses IUtil::formatStatus() — no fb_interpret needed.
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
};

}; // end namespace IscDbcLibrary

#endif // __LOAD_FB_CLIENT_DLL__
