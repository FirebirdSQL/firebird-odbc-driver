#include "IscDbc.h"
#ifndef _WIN32
#include <dlfcn.h>
#endif

namespace IscDbcLibrary {

CFbDll::CFbDll() 
{ 
	_Handle = NULL;
}

CFbDll::~CFbDll() 
{ 
	Release(); 
}

bool CFbDll::LoadDll(const char * client)
{
#ifdef _WIN32
	_Handle = LoadLibrary (client);
#else
	_Handle = dlopen (client, RTLD_NOW);
#endif
	if ( !_Handle )
		return false;

	_CFbDllVersion = 12;

#ifdef _WIN32
#define __ENTRYPOINT(X) _##X = (X*)GetProcAddress(_Handle, "isc_"#X)
#else
#define __ENTRYPOINT(X) _##X = (X*)dlsym(_Handle, "isc_"#X)
#endif					

	__ENTRYPOINT(create_database);
	__ENTRYPOINT(attach_database);
	__ENTRYPOINT(detach_database);
	__ENTRYPOINT(drop_database);
	__ENTRYPOINT(database_info);
	__ENTRYPOINT(open_blob2);
	__ENTRYPOINT(create_blob);
	__ENTRYPOINT(create_blob2);
	__ENTRYPOINT(close_blob);
	__ENTRYPOINT(cancel_blob);
	__ENTRYPOINT(get_segment);
	__ENTRYPOINT(put_segment);
	__ENTRYPOINT(blob_info);

	__ENTRYPOINT(array_get_slice);
	__ENTRYPOINT(array_put_slice);
	__ENTRYPOINT(array_lookup_bounds);

	__ENTRYPOINT(vax_integer);
	__ENTRYPOINT(start_transaction);
	__ENTRYPOINT(sqlcode);
	__ENTRYPOINT(sql_interprete);
	__ENTRYPOINT(interprete);
	__ENTRYPOINT(que_events);
	__ENTRYPOINT(cancel_events);
	__ENTRYPOINT(start_multiple);
	__ENTRYPOINT(commit_transaction);
	__ENTRYPOINT(commit_retaining);
	__ENTRYPOINT(rollback_transaction);
	__ENTRYPOINT(rollback_retaining);
	__ENTRYPOINT(dsql_execute_immediate);
	__ENTRYPOINT(dsql_allocate_statement);
	__ENTRYPOINT(dsql_describe);
	__ENTRYPOINT(dsql_describe_bind);
	__ENTRYPOINT(dsql_prepare);
	__ENTRYPOINT(dsql_execute);
	__ENTRYPOINT(dsql_execute2);
	__ENTRYPOINT(dsql_fetch);
	__ENTRYPOINT(dsql_free_statement);
	__ENTRYPOINT(dsql_set_cursor_name);
	__ENTRYPOINT(dsql_sql_info);
	__ENTRYPOINT(decode_date);
	__ENTRYPOINT(encode_date);
	__ENTRYPOINT(add_user);
	__ENTRYPOINT(modify_user);
	__ENTRYPOINT(delete_user);

	__ENTRYPOINT(service_attach);
	__ENTRYPOINT(service_detach);
	__ENTRYPOINT(service_start);
	__ENTRYPOINT(service_query);
	__ENTRYPOINT(decode_sql_date);
	__ENTRYPOINT(decode_sql_time);
	__ENTRYPOINT(decode_timestamp);
	__ENTRYPOINT(encode_sql_date);
	__ENTRYPOINT(encode_sql_time);
	__ENTRYPOINT(encode_timestamp);
	__ENTRYPOINT(print_blr);

	return true;
}

void CFbDll::Release(void)
{
	if ( _Handle )
#ifdef _WIN32
		FreeLibrary(_Handle);
#endif
#ifdef ELF
		dlclose (_Handle);
#endif

	_Handle = NULL;
}

}; // end namespace IscDbcLibrary
