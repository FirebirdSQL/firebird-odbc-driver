// Blob.h: interface for the Blob class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_BLOB_H_)
#define _BLOB_H_

enum enumTypeBlob { enTypeBlob = 1, enTypeClob, enTypeArray };
class Connection;

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

	virtual void	bind(Connection *connect, char * sqldata) = 0;
	virtual void	attach(char * pointBlob, bool fetched, bool clear) = 0;
	virtual bool	isBlob() = 0;
	virtual bool	isClob() = 0;
	virtual bool	isArray() = 0;
};

#endif // !defined(_BLOB_H_)
