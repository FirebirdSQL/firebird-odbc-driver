#ifndef __LOAD_FB_CLIENT_DLL__
#define __LOAD_FB_CLIENT_DLL__

#ifdef _WINDOWS
#include <windows.h>
#endif

namespace IscDbcLibrary {

typedef ISC_STATUS ISC_EXPORT create_database (ISC_STATUS ISC_FAR *, 
					    short, 
					    char ISC_FAR *, 
					    isc_db_handle ISC_FAR *, 
					    short, 
					    char ISC_FAR *, 
					    short);

typedef ISC_STATUS ISC_EXPORT array_gen_sdl(ISC_STATUS ISC_FAR*,
						ISC_ARRAY_DESC ISC_FAR*,
						short ISC_FAR*,
						char ISC_FAR*,
						short ISC_FAR*);

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

typedef ISC_STATUS ISC_EXPORT array_lookup_desc(ISC_STATUS ISC_FAR*,
						isc_db_handle ISC_FAR*,
						isc_tr_handle ISC_FAR*,
						char ISC_FAR*,
						char ISC_FAR*,
						ISC_ARRAY_DESC ISC_FAR*);

typedef ISC_STATUS ISC_EXPORT array_set_desc(ISC_STATUS ISC_FAR*,
						char ISC_FAR*,
						char ISC_FAR*,
						short ISC_FAR*,
						short ISC_FAR*,
						short ISC_FAR*,
						ISC_ARRAY_DESC ISC_FAR*);

typedef ISC_STATUS ISC_EXPORT array_put_slice(ISC_STATUS ISC_FAR*,
						isc_db_handle ISC_FAR*,
						isc_tr_handle ISC_FAR*,
						ISC_QUAD ISC_FAR*,
						ISC_ARRAY_DESC ISC_FAR*,
						void ISC_FAR*,
						ISC_LONG ISC_FAR*);

typedef ISC_STATUS ISC_EXPORT attach_database (ISC_STATUS ISC_FAR *, 
					    short, 
					    char ISC_FAR *, 
					    isc_db_handle ISC_FAR *, 
					    short, 
					    char ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT detach_database (ISC_STATUS ISC_FAR *,  
					    isc_db_handle ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT drop_database (ISC_STATUS ISC_FAR *,  
					  isc_db_handle ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT database_info (ISC_STATUS ISC_FAR *, 
					  isc_db_handle ISC_FAR *, 
					  short, 
					  char ISC_FAR *, 
					  short, 
					  char ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT dsql_execute_immediate (ISC_STATUS ISC_FAR *, 
						   isc_db_handle ISC_FAR *, 
						   isc_tr_handle ISC_FAR *, 
						   unsigned short, 
						   char ISC_FAR *, 
						   unsigned short, 
						   XSQLDA ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT open_blob2 (ISC_STATUS ISC_FAR *, 
				       isc_db_handle ISC_FAR *, 
				       isc_tr_handle ISC_FAR *,
				       isc_blob_handle ISC_FAR *, 
				       ISC_QUAD ISC_FAR *, 
				       short,  
				       char ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT create_blob (ISC_STATUS ISC_FAR *, 
					isc_db_handle ISC_FAR *, 
					isc_tr_handle ISC_FAR *, 
					isc_blob_handle ISC_FAR *, 
					ISC_QUAD ISC_FAR *);

typedef ISC_STATUS	ISC_EXPORT create_blob2(ISC_STATUS ISC_FAR*,
					isc_db_handle ISC_FAR*,
					isc_tr_handle ISC_FAR*,
					isc_blob_handle ISC_FAR*,
					ISC_QUAD ISC_FAR*,
					short,
					char ISC_FAR*);

typedef ISC_STATUS  ISC_EXPORT close_blob (ISC_STATUS ISC_FAR *, 
				       isc_blob_handle ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT cancel_blob (ISC_STATUS ISC_FAR *, 
				        isc_blob_handle ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT get_segment (ISC_STATUS ISC_FAR *, 
				        isc_blob_handle ISC_FAR *, 
				        unsigned short ISC_FAR *, 
				        unsigned short, 
				        char ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT put_segment (ISC_STATUS ISC_FAR *, 
					isc_blob_handle ISC_FAR *, 
					unsigned short, 
					char ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT blob_info (ISC_STATUS ISC_FAR *, 
				      isc_blob_handle ISC_FAR *, 
				      short,
 				      char ISC_FAR *, 
				      short, 
				      char ISC_FAR *);

typedef ISC_LONG    ISC_EXPORT vax_integer (char ISC_FAR *, 
					short);

typedef ISC_STATUS ISC_EXPORT_VARARG start_transaction(ISC_STATUS ISC_FAR*,
					isc_tr_handle ISC_FAR*,
					short, ...);

typedef ISC_LONG    ISC_EXPORT sqlcode (ISC_STATUS ISC_FAR *);

typedef void        ISC_EXPORT sql_interprete (short, 
					   char ISC_FAR *, 
					   short);

typedef ISC_STATUS  ISC_EXPORT interprete (char ISC_FAR *, 
				       ISC_STATUS ISC_FAR * ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT que_events (ISC_STATUS ISC_FAR *, 
				       isc_db_handle ISC_FAR *, 
				       ISC_LONG ISC_FAR *, 
				       short, 
				       char ISC_FAR *, 
				       isc_callback, 
				       void ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT cancel_events (ISC_STATUS ISC_FAR *, 
					  isc_db_handle ISC_FAR *, 
					  ISC_LONG ISC_FAR *);

typedef ISC_STATUS ISC_EXPORT wait_for_event(ISC_STATUS ISC_FAR *,
										 isc_db_handle ISC_FAR *,
										 short,
										 char *,
										 char *);

typedef ISC_STATUS  ISC_EXPORT start_multiple (ISC_STATUS ISC_FAR *, 
					   isc_tr_handle ISC_FAR *, 
					   short, 
					   void ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT commit_transaction (ISC_STATUS ISC_FAR *, 
					       isc_tr_handle ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT commit_retaining (ISC_STATUS ISC_FAR *, 
					     isc_tr_handle ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT rollback_transaction (ISC_STATUS ISC_FAR *, 
						 isc_tr_handle ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT rollback_retaining (ISC_STATUS ISC_FAR *, 
						 isc_tr_handle ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT prepare_transaction2 (ISC_STATUS ISC_FAR *,
						isc_tr_handle ISC_FAR *,
						unsigned short,
						const unsigned char ISC_FAR * );

typedef ISC_STATUS  ISC_EXPORT dsql_allocate_statement (ISC_STATUS ISC_FAR *, 
						    isc_db_handle ISC_FAR *, 
						    isc_stmt_handle ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT dsql_describe (ISC_STATUS ISC_FAR *, 
					  isc_stmt_handle ISC_FAR *, 
					  unsigned short, 
					  XSQLDA ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT dsql_describe_bind (ISC_STATUS ISC_FAR *, 
					       isc_stmt_handle ISC_FAR *, 
					       unsigned short, 
					       XSQLDA ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT dsql_execute (ISC_STATUS ISC_FAR *, 
					 isc_tr_handle ISC_FAR *,
					 isc_stmt_handle ISC_FAR *, 
					 unsigned short, 
					 XSQLDA ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT dsql_execute2 (ISC_STATUS ISC_FAR *, 
					  isc_tr_handle ISC_FAR *,
					  isc_stmt_handle ISC_FAR *, 
					  unsigned short, 
					  XSQLDA ISC_FAR *,
					  XSQLDA ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT dsql_fetch (ISC_STATUS ISC_FAR *, 
				       isc_stmt_handle ISC_FAR *, 
				       unsigned short, 
				       XSQLDA ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT dsql_free_statement (ISC_STATUS ISC_FAR *, 
						isc_stmt_handle ISC_FAR *, 
						unsigned short);

typedef ISC_STATUS  ISC_EXPORT dsql_prepare (ISC_STATUS ISC_FAR *, 
					 isc_tr_handle ISC_FAR *, 
					 isc_stmt_handle ISC_FAR *, 
					 unsigned short, 
					 char ISC_FAR *, 
					 unsigned short, 
				 	 XSQLDA ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT dsql_set_cursor_name (ISC_STATUS ISC_FAR *, 
						 isc_stmt_handle ISC_FAR *, 
						 char ISC_FAR *, 
						 unsigned short);

typedef ISC_STATUS  ISC_EXPORT dsql_sql_info (ISC_STATUS ISC_FAR *, 
					  isc_stmt_handle ISC_FAR *, 
					  short, 
					  char ISC_FAR *, 
					  short, 
					  char ISC_FAR *);

typedef void        ISC_EXPORT decode_date (ISC_QUAD ISC_FAR *, 
					void ISC_FAR *);

typedef void        ISC_EXPORT encode_date (void ISC_FAR *, 
					ISC_QUAD ISC_FAR *);

typedef int	ISC_EXPORT add_user (ISC_STATUS ISC_FAR *, USER_SEC_DATA *);
typedef int	ISC_EXPORT delete_user (ISC_STATUS ISC_FAR *, USER_SEC_DATA *);
typedef int	ISC_EXPORT modify_user (ISC_STATUS ISC_FAR *, USER_SEC_DATA *);


typedef ISC_STATUS  ISC_EXPORT service_attach (ISC_STATUS ISC_FAR *, 
					   unsigned short, 
					   char ISC_FAR *,
					   isc_svc_handle ISC_FAR *, 
					   unsigned short, 
					   char ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT service_detach (ISC_STATUS ISC_FAR *, 
					   isc_svc_handle ISC_FAR *);

typedef ISC_STATUS  ISC_EXPORT service_query (ISC_STATUS ISC_FAR *, 
					  isc_svc_handle ISC_FAR *,
                      		          isc_resv_handle ISC_FAR *,
					  unsigned short, 
					  char ISC_FAR *, 
					  unsigned short, 
					  char ISC_FAR *, 
					  unsigned short, 
					  char ISC_FAR *);

typedef ISC_STATUS ISC_EXPORT service_start (ISC_STATUS ISC_FAR *,
    					 isc_svc_handle ISC_FAR *,
                         		 isc_resv_handle ISC_FAR *,
    					 unsigned short,
    					 char ISC_FAR*);

typedef void        ISC_EXPORT decode_sql_date (ISC_DATE ISC_FAR *, 
					void ISC_FAR *);

typedef void        ISC_EXPORT decode_sql_time (ISC_TIME ISC_FAR *, 
					void ISC_FAR *);

typedef void        ISC_EXPORT decode_timestamp (ISC_TIMESTAMP ISC_FAR *, 
					void ISC_FAR *);

typedef void        ISC_EXPORT encode_sql_date (void ISC_FAR *, 
					ISC_DATE ISC_FAR *);

typedef void        ISC_EXPORT encode_sql_time (void ISC_FAR *, 
					ISC_TIME ISC_FAR *);

typedef void        ISC_EXPORT encode_timestamp (void ISC_FAR *, 
					ISC_TIMESTAMP ISC_FAR *);

typedef void        ISC_EXPORT print_blr(char ISC_FAR*,
					isc_callback,
					void ISC_FAR*,
					short);

class CFbDll
{
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

	// FbClient.Dll Entry Points
	create_database*			_create_database;
	attach_database*			_attach_database;
	detach_database*			_detach_database;
	drop_database*				_drop_database;
	start_transaction*			_start_transaction;
	database_info*				_database_info;
	dsql_execute_immediate*		_dsql_execute_immediate;
	array_lookup_bounds*		_array_lookup_bounds;
	array_get_slice*			_array_get_slice;
	array_put_slice*			_array_put_slice;
	open_blob2*					_open_blob2;
	create_blob*				_create_blob;
	create_blob2*				_create_blob2;
	close_blob*					_close_blob;
	cancel_blob*				_cancel_blob;
	get_segment*				_get_segment;
	put_segment*				_put_segment;
	blob_info*					_blob_info;

	vax_integer*				_vax_integer;
	sqlcode*					_sqlcode;
	sql_interprete*				_sql_interprete;
	interprete*					_interprete;
	que_events*					_que_events;
	cancel_events* 				_cancel_events;
	wait_for_event*				_wait_for_event;
	start_multiple*				_start_multiple;
	commit_transaction*			_commit_transaction;
	commit_retaining*			_commit_retaining;
	rollback_transaction*		_rollback_transaction;
	rollback_retaining*			_rollback_retaining;
	prepare_transaction2*		_prepare_transaction2;
	dsql_allocate_statement*	_dsql_allocate_statement;
	dsql_describe*				_dsql_describe;
	dsql_describe_bind*			_dsql_describe_bind;
	dsql_prepare*				_dsql_prepare;
	dsql_execute*				_dsql_execute;
	dsql_execute2*				_dsql_execute2;
	dsql_fetch*					_dsql_fetch;
	dsql_free_statement*		_dsql_free_statement;
	dsql_set_cursor_name*		_dsql_set_cursor_name;
	dsql_sql_info* 				_dsql_sql_info;
	decode_date*				_decode_date;
	encode_date*				_encode_date;
	add_user*					_add_user;
	delete_user*				_delete_user;
	modify_user*				_modify_user;

	service_attach*				_service_attach;
	service_detach*				_service_detach;
	service_start*				_service_start;
	service_query*				_service_query;
	decode_sql_date*			_decode_sql_date;
	decode_sql_time*			_decode_sql_time;
	decode_timestamp*			_decode_timestamp;
	encode_sql_date*			_encode_sql_date;
	encode_sql_time*			_encode_sql_time;
	encode_timestamp*			_encode_timestamp;
	print_blr*					_print_blr;
};

}; // end namespace IscDbcLibrary

#endif // __LOAD_FB_CLIENT_DLL__
