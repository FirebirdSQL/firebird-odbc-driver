// Blob.h: interface for the Blob class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BLOB_H__84FD196A_A97F_11D2_AB5C_0000C01D2301__INCLUDED_)
#define AFX_BLOB_H__84FD196A_A97F_11D2_AB5C_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class Blob
{
public:
	virtual void	addRef() = 0;
	virtual int		release() = 0;
	virtual void	getBytes (long pos, long length, void *buffer) = 0;
	virtual int		length() = 0;
	virtual int		getSegmentLength (int pos) = 0;
	virtual void	*getSegment (int pos) = 0;
};

class Clob
{
public:
	virtual void	addRef() = 0;
	virtual int		release() = 0;
	virtual void	getSubString (long pos, long length, char *buffer) = 0;
	virtual int		length() = 0;
	virtual int		getSegmentLength (int pos) = 0;
	virtual const char *getSegment (int pos) = 0;
};

#endif // !defined(AFX_BLOB_H__84FD196A_A97F_11D2_AB5C_0000C01D2301__INCLUDED_)
