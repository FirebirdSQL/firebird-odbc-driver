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

// ParametersEvents.h: interface for the ParametersEvents class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ParametersEvents_H_)
#define _ParametersEvents_H_

#include "Connection.h"

namespace IscDbcLibrary {

class ParameterEvent;

class ParametersEvents : public PropertiesEvents
{
public:
	ParametersEvents();
	~ParametersEvents();

	virtual void		putNameEvent( const char *name );
	virtual int			getCount();
	virtual const char	*getNameEvent( int index );
	virtual int			findIndex( const char * name );
	virtual unsigned long getCountExecutedEvents( int index );
	virtual bool		isChanged( int index );
	virtual void		addRef();
	virtual int			release();

	int	lengthNameEvent( int index );
	void updateCountExecutedEvents( int index, unsigned long newCount );
	void clear();

public:
	int				useCount;
	ParameterEvent	*parameters;
	int				count;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_ParametersEvents_H_)
