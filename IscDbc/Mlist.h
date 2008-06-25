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

// Mlist.h
//
//////////////////////////////////////////////////////////////////////

#if !defined(_MLIST_H_)
#define _MLIST_H_

#include <stdlib.h>

namespace IscDbcLibrary {

// Template for default value comparsion
template <typename T>
class DefaultComparator 
{
public:
	static int compare(const T *a, const T *b) 
	{
	    return 1;
	}
};

template <class T, typename Cmp = DefaultComparator<T> >
class MList
{
private:
	T	 	*ptRoot, // Head list
			*ptActiveNode; // active list node
	int		count;	// count node
	char	bOk;
	
	const int	diffCache;
	int		numberStartCache;
	int		countCache;		 
	int		rowActive;

private:
	void OnInitList()
	{
		count = 0;
		countCache = numberStartCache;
		rowActive = 0;
		ptActiveNode = NULL;
		ptRoot = (T*)calloc(1, countCache * sizeof(T));
		if ( ptRoot )
			bOk = true;
		else
			bOk = false;
	}

public:
	MList( int cache = 50 ): diffCache( cache )
	{
		numberStartCache = diffCache;
		OnInitList();
	}

	void removeAll()
	{
		clear();
		OnInitList();
	}

	~MList()
	{
		clear();
	}

	char reInit(int checkCount = 0)
	{
		if ( bOk == false )
			return false;

		if ( checkCount && checkCount < countCache )
			return true;

		char bRez=false;
		T * tmp;

		if ( checkCount > countCache + diffCache )
			checkCount += diffCache;
		else
			checkCount = countCache + diffCache;

		tmp = (T*)realloc(ptRoot, checkCount * sizeof(T));
		if ( tmp )
		{
			countCache = checkCount;
			ptRoot=tmp;
			bRez=true;
		}
		return bRez;
	}
	
	T & operator [](int pos){ return  ptRoot[pos];}
	T & operator ()(int pos)
	{ 
		reInit(pos);
		if ( count <= pos )
			count = pos + 1;
		return  ptRoot[pos];
	}

	void operator =(MList &Lst)
	{
		if ( !Lst.GetCount() )
			return;

		clear();
		ptRoot=(T*)calloc(1,Lst.countCache * sizeof(T));
		if ( ptRoot )
		{
			bOk = true;
			count = Lst.count;
			countCache = Lst.countCache;
			numberStartCache = Lst.numberStartCache;
			memcpy(ptRoot,Lst.ptRoot,sizeof(T)*count);
		}
		else
			bOk=false;
	}
	char GetSuccess(){ return bOk; }
	T * GetRoot(){ return ptRoot; }
	int GetCount(){ return count; }

	T *GetHeadPosition(int pos = 0)
	{
		if ( count == 0 )
			return NULL;

		if ( pos < count )
		{
			ptActiveNode = &ptRoot[pos];
			rowActive = pos;
		}
		else
		{
			ptActiveNode = ptRoot;
			rowActive = 0;
		}
		return ptActiveNode;
	}
	
	T *GetNext()
	{
		if ( rowActive + 1 >= count )
			return NULL;

		ptActiveNode = &ptRoot[++rowActive];
		return ptActiveNode;
	}
	
	int SearchAndInsert(T * key)
	{
		register int ret = 1, i, l = 0, u = count - 1;
		int size = sizeof(T);
		T * buf = ptRoot;

		while ( u >= l )
		{
			i = (l+u) >> 1;
			if(ret = Cmp::compare(key, &buf[i]), ret < 0)
				u=i-1;
			else if ( ret > 0 )
				l=i+1;
			else 
				break;
		}

		if ( ret )
		{
			if( ret > 0 )i = l;
			else i = u+1;

			memmove(&buf[i+1],&buf[i],(count++-i)*size);
			memset(&buf[i++],0,size);

			if ( count == countCache )
				reInit();

			i=-i;
		}

		return i;
	}

	int Search(T * key)
	{
		register int ret = 1, i, l = 0, u = count - 1;
		int size = sizeof(T);
		T * buf = ptRoot;

		while ( u >= l )
		{
			i = (l+u) >> 1;
			if(ret = Cmp::compare(key, &buf[i]), ret < 0)
				u=i-1;
			else if ( ret > 0 )
				l=i+1;
			else 
				break;
		}

		if ( ret ) i = -1;
		return i;
	}

	void clear()
	{
		if ( ptRoot )
		{
			T * p = ptRoot;

			while ( count-- )
				(p++)->remove();

			free( ptRoot );
			ptRoot = NULL;
		}

		count = 0; 
		countCache = numberStartCache;
	}
};

}; // end namespace IscDbcLibrary

#endif // !defined(_MLIST_H_)
