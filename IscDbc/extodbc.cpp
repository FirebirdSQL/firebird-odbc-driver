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
 *  Copyright (c) 2003 Vladimir Tsvigun
 *  All Rights Reserved.
 */

// ExtOdbc.cpp
//
//////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#ifdef __FreeBSD__
#include <sys/time.h>
#endif
#include <sys/timeb.h>
#include <stdio.h>
#include <string.h>
#include "IscDbc.h"

namespace IscDbcLibrary {

int getTypeStatement(IscConnection * connection, isc_stmt_handle Stmt,const void * buffer, int bufferLength,int *lengthPtr)
{
	ISC_STATUS	statusVector[20];
	char type_info[] = { isc_info_sql_stmt_type };
	char * info_buffer=(char*)buffer;
	int &l=*lengthPtr;
	CFbDll * GDS = connection->GDS;

	if(GDS->_dsql_sql_info(statusVector,&Stmt,sizeof(type_info),type_info,bufferLength,info_buffer))
		return -1;
	l = GDS->_vax_integer((char*)info_buffer+1,2);
	return 0;
}

int getInfoCountRecordsStatement(IscConnection * connection, isc_stmt_handle Stmt,const void * buffer, int bufferLength,int *lengthPtr)
{
	ISC_STATUS	statusVector[20];
	char records_info[] = { isc_info_sql_records,isc_info_end };
	char * info_buffer=(char*)buffer;
	int &l=*lengthPtr;
	CFbDll * GDS = connection->GDS;

	if(GDS->_dsql_sql_info(statusVector,&Stmt,sizeof(records_info),records_info,bufferLength,info_buffer))
		return -1;
	l = GDS->_vax_integer((char*)info_buffer+1,2);
	return 0;
}

int getPlanStatement(IscConnection * connection, isc_stmt_handle Stmt,const void * buffer, int bufferLength,int *lengthPtr)
{
	ISC_STATUS	statusVector[20];
	char plan_info[] = { isc_info_sql_get_plan };
	char * plan_buffer=(char*)buffer;
	int &l=*lengthPtr;
	CFbDll * GDS = connection->GDS;

	if(GDS->_dsql_sql_info(statusVector,&Stmt,sizeof(plan_info),plan_info,bufferLength,plan_buffer))
		return -1;
	else if(plan_buffer[0] == isc_info_sql_get_plan) 
	{
		l = GDS->_vax_integer((char*)plan_buffer+1,2)+3;
		plan_buffer[0] = plan_buffer[1] = 32; // ' '
		plan_buffer[2] = '\n';
		if(l+1<bufferLength)plan_buffer[l++] = '\n'; 
		plan_buffer[l] = 0;
	}
	return 0;
}

int getInfoDatabase(IscConnection * connection, const void * info_buffer, int bufferLength,short *lengthPtr,
						char * db_items, int item_length);

int getPageDatabase(IscConnection * connection, const void * info_buffer, int bufferLength,short *lengthPtr)
{
	static char db_info[] = { isc_info_page_size,isc_info_end };
	int nRet=getInfoDatabase(connection,info_buffer,bufferLength,lengthPtr,db_info,sizeof(db_info));
	return nRet;
}

int getWalDatabase(IscConnection * connection, const void * info_buffer, int bufferLength,short *lengthPtr)
{
	static char db_info[] = { 		
		isc_info_num_wal_buffers,
		isc_info_wal_buffer_size,
		isc_info_wal_grpc_wait_usecs,
		isc_info_wal_ckpt_length,
		isc_info_end
 };
	int nRet=getInfoDatabase(connection,info_buffer,bufferLength,lengthPtr,db_info,sizeof(db_info));
	return nRet;
}

void print_set(int &set_used, char *& buf, short &len)
{
	if (!set_used)
	{
		len=sprintf(buf,"  SET ");
		buf+=len;
		set_used = 1;
	}
	else
	{
		len=sprintf(buf,", \n");
		buf+=len;
	}
}

int getInfoDatabase(IscConnection * connection, const void * info_buffer, int bufferLength,short *lengthPtr,char * db_info, int db_info_length)
{
	char *d, buffer[256], item, *info;
	int length;
	ISC_STATUS	statusVector[20];
	char * info_buf=(char*)info_buffer;
	short &l=*lengthPtr,len;
	int set_used=0;
	long value_out;
	CFbDll * GDS = connection->GDS;
	isc_db_handle Db = connection->getHandleDb();

	*info_buf = '\0'; l=0;

	if (GDS->_database_info(statusVector, &Db,db_info_length,db_info,256,buffer))
		return -1;

	for(d = buffer, info = info_buf; *d != isc_info_end;) 
	{
		item = *d++;
		length = GDS->_vax_integer(d, 2);
		d += 2;
		switch (item) 
		{
		case isc_info_end:
			break;

		case isc_info_page_size:
			value_out = GDS->_vax_integer(d, length);
			len = sprintf(info, "PAGE_SIZE %ld\n", value_out);
			break;
		case isc_info_num_wal_buffers:
			print_set(set_used,info,len);
			value_out = GDS->_vax_integer(d, length);
			len = sprintf(info,"NUM_LOG_BUFFERS = %ld", value_out);
			break;
		case isc_info_wal_buffer_size:
			value_out = GDS->_vax_integer(d, length);
			print_set(set_used,info,len);
			len = sprintf(info,"LOG_BUFFER_SIZE = %ld", value_out);
			break;
		case isc_info_wal_grpc_wait_usecs:
			value_out = GDS->_vax_integer(d, length);
			print_set(set_used,info,len);
			len = sprintf(info,"GROUP_COMMIT_WAIT_TIME = %ld",value_out);
			break;
		case isc_info_wal_ckpt_length:
			value_out = GDS->_vax_integer(d, length);
			print_set(set_used,info,len);
			len = sprintf(info,"CHECK_POINT_LENGTH = %ld",value_out);
			break;
		}
		d += length;
		info += len;
		l += len;
	}
	return 0;
}

signed int getVaxInteger(const unsigned char * ptr, signed short length)
{ // FIREBIRD
	signed int value;
	signed short shift;

	value = shift = 0;

	while (--length >= 0) {
		value += ((signed int) * ptr++) << shift;
		shift += 8;
	}
	return value;
}

signed long get_parameter(char ** ptr)
{
	signed long parameter;
	signed short l;

	l = *(*ptr)++;
	l += (*(*ptr)++) << 8;
	parameter = getVaxInteger((const unsigned char *)*ptr, l);
	*ptr += l;

	return parameter;
}

typedef struct stStatInfo {
	long fetches;
	long marks;
	long reads;
	long writes;
	long current_memory;
	long max_memory;
	long buffers;
	long page_size;
	long elapsed;

public:

	stStatInfo(){ Remove(); }

	void Remove(){ memset(this,0,sizeof(*this));}

} StatInfo;

static StatInfo StatInfoBefore, StatInfoAfter;

void getStatInformations(IscConnection * connection, char bNumberCall)
{
	struct timeb time_buffer;
#define LARGE_NUMBER 696600000	/* to avoid overflow, get rid of decades) */

	char items[] = 
	{ 
		isc_info_reads, isc_info_writes, isc_info_fetches,
		isc_info_marks,
		isc_info_page_size, isc_info_num_buffers,
		isc_info_current_memory, isc_info_max_memory
	};
	StatInfo * ptStat;
	char *p, buffer[256];
	signed short l, buffer_length, item_length;
	ISC_STATUS	statusVector[20];
	CFbDll * GDS = connection->GDS;
	isc_db_handle Db = connection->getHandleDb();

	buffer_length = sizeof(buffer);
	item_length = sizeof(items);

	if(bNumberCall==1)
		ptStat = &StatInfoBefore;
	else if(bNumberCall==2)
		ptStat = &StatInfoAfter;
	else
	{
		StatInfoBefore.Remove();
		StatInfoAfter.Remove();
		return;
	}
	ptStat->Remove();

	ftime(&time_buffer);
	ptStat->elapsed =
		(long)(time_buffer.time - LARGE_NUMBER) * 100 + (time_buffer.millitm / 10);

	GDS->_database_info(statusVector, &Db,item_length, items, buffer_length, buffer);

	p = buffer;

	while (1)
		switch (*p++) 
		{
		case isc_info_reads:
			ptStat->reads = get_parameter(&p);
			break;

		case isc_info_writes:
			ptStat->writes = get_parameter(&p);
			break;

		case isc_info_marks:
			ptStat->marks = get_parameter(&p);
			break;

		case isc_info_fetches:
			ptStat->fetches = get_parameter(&p);
			break;

		case isc_info_num_buffers:
			ptStat->buffers = get_parameter(&p);
			break;

		case isc_info_page_size:
			ptStat->page_size = get_parameter(&p);
			break;

		case isc_info_current_memory:
			ptStat->current_memory = get_parameter(&p);
			break;

		case isc_info_max_memory:
			ptStat->max_memory = get_parameter(&p);
			break;

		case isc_info_end:
			return;

		case isc_info_error:
			if (p[2] == isc_info_marks)
				ptStat->marks = 0;
			else if (p[2] == isc_info_current_memory)
				ptStat->current_memory = 0;
			else if (p[2] == isc_info_max_memory)
				ptStat->max_memory = 0;

			l = (signed short)getVaxInteger((const unsigned char *)p, 2);

			p += l + 2;
			ptStat->marks = 0;
			break;
		default:
			return;
		}
}

int getStatInformations(IscConnection * connection, const void * info_buffer, int bufferLength,short *lengthPtr)
{
	struct timeb time_buffer;
#define LARGE_NUMBER 696600000	/* to avoid overflow, get rid of decades) */

	char items[] = 
	{ 
		isc_info_reads, isc_info_writes, isc_info_fetches,
		isc_info_marks,
		isc_info_page_size, isc_info_num_buffers,
		isc_info_current_memory, isc_info_max_memory
	};
	StatInfo * ptStat = (StatInfo *)info_buffer;
	char *p, buffer[256];
	signed short l, buffer_length, item_length;
	ISC_STATUS	statusVector[20];
	CFbDll * GDS = connection->GDS;
	isc_db_handle Db = connection->getHandleDb();

	buffer_length = sizeof(buffer);
	item_length = sizeof(items);

	*lengthPtr = bufferLength;

	ftime(&time_buffer);
	ptStat->elapsed =
		(long)(time_buffer.time - LARGE_NUMBER) * 100 + (time_buffer.millitm / 10);

	GDS->_database_info(statusVector, &Db,item_length, items, buffer_length, buffer);

	p = buffer;

	while (1)
		switch (*p++) 
		{
		case isc_info_reads:
			ptStat->reads = get_parameter(&p);
			break;

		case isc_info_writes:
			ptStat->writes = get_parameter(&p);
			break;

		case isc_info_marks:
			ptStat->marks = get_parameter(&p);
			break;

		case isc_info_fetches:
			ptStat->fetches = get_parameter(&p);
			break;

		case isc_info_num_buffers:
			ptStat->buffers = get_parameter(&p);
			break;

		case isc_info_page_size:
			ptStat->page_size = get_parameter(&p);
			break;

		case isc_info_current_memory:
			ptStat->current_memory = get_parameter(&p);
			break;

		case isc_info_max_memory:
			ptStat->max_memory = get_parameter(&p);
			break;

		case isc_info_end:
			return 1;

		case isc_info_error:
			if (p[2] == isc_info_marks)
				ptStat->marks = 0;
			else if (p[2] == isc_info_current_memory)
				ptStat->current_memory = 0;
			else if (p[2] == isc_info_max_memory)
				ptStat->max_memory = 0;

			l = (signed short)getVaxInteger((const unsigned char *)p, 2);

			p += l + 2;
			ptStat->marks = 0;
			break;
		default:
			return 1;
		}
}

const char * strFormatReport = 
		"\nCurrent memory = !c"
		"\nDelta memory   = !d"
		"\nMax memory     = !x"
		"\nBuffers        = !b"
		"\nReads          = !r"
		"\nWrites         = !w"
		"\nFetches        = !f"
		"\nElapsed time   = !e sec"
		"\nPage size      = !p$";


int strBuildStatInformations(const void * info_buffer, int bufferLength, short *lengthPtr)
{
	signed int delta, length;
	char *p, c;
	const char * string = strFormatReport;

	p = (char *)info_buffer;

	while ((c = *string++),c && c != '$')
	{
		if (c != '!')
			*p++ = c;
		else 
		{
			switch (c = *string++) 
			{
			case 'r':
				delta = StatInfoAfter.reads - StatInfoBefore.reads;
				break;
			case 'w':
				delta = StatInfoAfter.writes - StatInfoBefore.writes;
				break;
			case 'f':
				delta = StatInfoAfter.fetches - StatInfoBefore.fetches;
				break;
			case 'm':
				delta = StatInfoAfter.marks - StatInfoBefore.marks;
				break;
			case 'd':
				delta =	StatInfoAfter.current_memory - StatInfoBefore.current_memory;
				break;
			case 'p':
				delta = StatInfoAfter.page_size;
				break;
			case 'b':
				delta = StatInfoAfter.buffers;
				break;
			case 'c':
				delta = StatInfoAfter.current_memory;
				break;
			case 'x':
				delta = StatInfoAfter.max_memory;
				break;
			case 'e':
				delta = StatInfoAfter.elapsed - StatInfoBefore.elapsed;
				break;
			default:
				sprintf((char*)p, "?%c?", c);
				while (*p)
					p++;
			}
			switch (c) 
			{
			case 'r':
			case 'w':
			case 'f':
			case 'm':
			case 'd':
			case 'p':
			case 'b':
			case 'c':
			case 'x':
				sprintf((char*)p, "%ld", delta);
				while (*p)
					p++;
				break;
			case 'e':
				sprintf(p, "%ld.%.2ld", delta / 100, delta % 100);
				while (*p)
					p++;
				break;
			}
		}
	}
	*p = 0;
	length = p - (char *)info_buffer;
	if (bufferLength && (bufferLength -= length) >= 0)
	{
		do
			*p++ = ' ';
		while (--bufferLength);
	}
	return length;
}

}; // end namespace IscDbcLibrary
