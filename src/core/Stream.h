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
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 */

// Stream.h: interface for the Stream class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_STREAM_H_INCLUDED_)
#define _STREAM_H_INCLUDED_

namespace IscDbcLibrary {

struct Segment
{
	int		length;
	char	*address;
	Segment	*next;
};

class Blob;

class Stream  
{
public:
	void putSegment (Blob *blob);
	void* getSegment (int offset);
	int getSegmentLength(int offset);
	void printChars (const char *msg, int length, const char *data);
	void putCharacter (char c);
	void putSegment (int length, const unsigned short *chars);
	void putSegment (Stream *stream);
	void clear();
	void printShorts (const char *msg, int length, short *data);
	char* decompress();
	void compress (int length, void *address);
	virtual char*	getString();
	virtual int		getSegment (int offset, int len, void *ptr, char delimiter);
	virtual void	setSegment (Segment *segment, int length, void *address);
	virtual char*	convStrHexToBinary (char * ptr, int len);
	virtual int		getSegmentToBinary(int offset, int len, void * ptr);
	virtual int		getSegmentToHexStr(int offset, int len, void * ptr);
	virtual int		getSegment (int offset, int length, void* address);
	virtual int		getSegmentW (int offset, int length, void* address);
	virtual void	putSegment (const char *string);
	virtual void	putSegment (int length, const char *address, bool copy);
	virtual int		getLength();

	char*			alloc (int length);
	Segment*		allocSegment (int tail);
	char*			transferRecord();
	void			setMinSegment (int length);
	void			attach(Stream &dst, bool clear);
	void			setConsecutiveRead (bool status) { consecutiveRead = status; }

	Stream();
	Stream (int minSegmentSize);
	virtual ~Stream();

	int		totalLength;
	int		minSegment;
	int		sizeStructSegment;
	int		currentLength;
	int		decompressedLength;
	int		useCount;
	bool	copyFlag;
	bool	bClear;
	Segment	first;
	Segment	*ptFirst;
	Segment	*segments;
	Segment *current;
	bool	consecutiveRead;
	Segment *currentRead;
	int		currentN;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_STREAM_H_INCLUDED_)
