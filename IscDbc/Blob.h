// Blob.h: interface for the Blob class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BLOB_H__84FD196A_A97F_11D2_AB5C_0000C01D2301__INCLUDED_)
#define AFX_BLOB_H__84FD196A_A97F_11D2_AB5C_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

namespace IscDbcLibrary {

enum enumTypeBlob { enTypeBlob = 1, enTypeClob, enTypeArray };

class Blob
{
public:
	enumTypeBlob	enType;
	virtual void	addRef() = 0;
	virtual int		release() = 0;
	virtual void	getBytes (long pos, long length, void *buffer) = 0;
	virtual void	getHexString (long pos, long length, void *buffer) = 0;
	virtual int		length() = 0;
	virtual int		getSegmentLength (int pos) = 0;
	virtual void	*getSegment (int pos) = 0;
};

}; // end namespace IscDbcLibrary

#endif // !defined(AFX_BLOB_H__84FD196A_A97F_11D2_AB5C_0000C01D2301__INCLUDED_)
