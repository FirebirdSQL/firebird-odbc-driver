// Blob.h: interface for the Blob class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_BLOB_H_)
#define _BLOB_H_

namespace IscDbcLibrary {

enum enumTypeBlob { enTypeBlob = 1, enTypeClob, enTypeArray };
class Statement;

class Blob
{
public:
	enumTypeBlob	enType;
	virtual void	addRef() = 0;
	virtual int		release() = 0;
	virtual void	clear() = 0;
	virtual void	getBytes (long pos, long length, void *buffer) = 0;
	virtual void	getBytesW (long pos, long length, void *buffer) = 0;
	virtual void	getBinary (long pos, long length, void *buffer) = 0;
	virtual void	getHexString (long pos, long length, void *buffer) = 0;
	virtual int		length() = 0;
	virtual int		getSegmentLength (int pos) = 0;
	virtual void	putSegment (int length, const char *data, bool copyFlag) = 0;
	virtual void	putLongSegment (int length, const char *data) = 0;
	virtual void	*getSegment (int pos) = 0;
	virtual void	writeBlob(char * sqldata) = 0;
	virtual void	writeStreamHexToBlob(char * sqldata) = 0;
	virtual void	writeBlob(char * sqldata, char *data, long length) = 0;
	virtual void	writeStringHexToBlob(char * sqldata, char *data, long length) = 0;
	virtual void	directCreateBlob( char * sqldata ) = 0;
	virtual void	directOpenBlob(char * sqldata) = 0;
	virtual bool	directFetchBlob(char *data, int length, int &lengthRead) = 0;
	virtual bool	directGetSegmentToHexStr( char * bufData, int lenData, int &lenRead ) = 0;
	virtual void	directWriteBlob( char *data, long length ) = 0;
	virtual void	directCloseBlob() = 0;
	virtual int		getOffset() = 0;

	virtual void	bind(Statement *stmt, char * sqldata) = 0;
	virtual void	attach(char * pointBlob, bool fetched, bool clear) = 0;
	virtual bool	isBlob() = 0;
	virtual bool	isClob() = 0;
	virtual bool	isArray() = 0;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_BLOB_H_)
