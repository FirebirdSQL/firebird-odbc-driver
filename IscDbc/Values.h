// Values.h: interface for the Values class.
//
//////////////////////////////////////////////////////////////////////

// copyright (c) 1999 - 2000 by James A. Starkey for IBPhoenix.


#if !defined(AFX_VALUES_H__02AD6A4C_A433_11D2_AB5B_0000C01D2301__INCLUDED_)
#define AFX_VALUES_H__02AD6A4C_A433_11D2_AB5B_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

namespace IscDbcLibrary {

class Value;

class Values  
{
public:
	void clear();
	void alloc (int n);
	Values();
	~Values();

	int		count;
	Value	*values;
};

}; // end namespace IscLibrary

#endif // !defined(AFX_VALUES_H__02AD6A4C_A433_11D2_AB5B_0000C01D2301__INCLUDED_)
