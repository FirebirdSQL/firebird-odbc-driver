/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
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

// Stream.cpp: implementation of the Stream class.
//
//////////////////////////////////////////////////////////////////////

#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "IscDbc.h"
#include "Stream.h"
#include "SQLError.h"
#include "Blob.h"
#include "BinToHexStr.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Stream::Stream()
{
	segments = NULL;
	current = NULL;
	totalLength = 0;
	minSegment = 0;
}

Stream::Stream(int minSegmentSize)
{
	segments = NULL;
	current = NULL;
	totalLength = 0;
	minSegment = minSegmentSize;
}

Stream::~Stream()
{
	Segment *segment;

	while (segment = segments)
		{
		segments = segment->next;
		if (segment != &first)
			free (segment);
		}
}

void Stream::putCharacter(char c)
{
	if (!segments || current->length >= currentLength)
		allocSegment (MAX (100, minSegment));

	current->address [current->length] = c;
	++current->length;
	++totalLength;
}

void Stream::putSegment(int length, const char *ptr, bool copy)
{
	const char *address = (char*) ptr;
	totalLength += length;

	if (!segments)
		{
		if (copyFlag = copy)
			{
			allocSegment (MAX (length, minSegment));
			current->length = length;
			memcpy (current->address, address, length);
			}
		else
			{
			//copyFlag = copy;
			current = segments = &first;
			current->length = length;
			current->address = (char*) address;
			current->next = NULL;
			}
		}
	else if (copyFlag)
		{
		int l = currentLength - current->length;
		if (l > 0)
			{
			int l2 = MIN (l, length);
			memcpy (current->address + current->length, address, l2);
			current->length += l2;
			length -= l2;
			address += l2;
			}
		if (length)
			{
			allocSegment (MAX (length, minSegment));
			current->length = length;
			memcpy (current->address, address, length);
			}
		}
	else
		{
		allocSegment (0);
		current->address = (char*) address;
		current->length = length;
		}
}

int Stream::getSegmentToHexStr(int offset, int len, void * ptr)
{
	int n = 0;
	int length = len;
	short *address = (short*) ptr;

	for (Segment *segment = segments; segment; n += segment->length, segment = segment->next)
		if (n + segment->length >= offset)
		{
			int off = offset - n;
			int l = MIN (length, segment->length - off);
			unsigned char * ptSours = (unsigned char *)segment->address + off;

			length -= l;
			offset += l;

			while( l > 0 )
				*address++ = conwBinToHexStr[*ptSours++],--l;

			if (!length)
				break;
		}

	return len - length;
}

int Stream::getSegment(int offset, int len, void * ptr)
{
	int n = 0;
	int length = len;
	char *address = (char*) ptr;

	for (Segment *segment = segments; segment; n += segment->length, segment = segment->next)
		if (n + segment->length >= offset)
			{
			int off = offset - n;
			int l = MIN (length, segment->length - off);
			memcpy (address, segment->address + off, l);
			address += l;
			length -= l;
			offset += l;
			if (!length)
				break;
			}

	return len - length;
}

void Stream::setSegment(Segment * segment, int length, void* address)
{
	segment->length = length;
	totalLength += length;

	if (copyFlag)
		{
		segment->address = new char [length];
		memcpy (segment->address, address, length);
		}
	else
		segment->address = (char*) address;
}

char* Stream::transferRecord()
{
	if (!copyFlag && (segments == &first) && !segments->next)
		return segments->address;

	char *buffer = new char [totalLength];
	getSegment (0, totalLength, buffer);

	return buffer;
}

void Stream::setMinSegment(int length)
{
	minSegment = length;
}

Segment* Stream::allocSegment(int tail)
{
	Segment *segment = (Segment*) malloc (sizeof (struct Segment) + tail);
	segment->address = (char*) segment + sizeof (struct Segment);
	segment->next = NULL;
	segment->length = 0;
	currentLength = tail;

	if (current)
		{
		current->next = segment;
		current = segment;
		}
	else
		segments = current = segment;

	return segment;
}

int Stream::getSegment(int offset, int len, void * ptr, char delimiter)
{
	int n = 0;
	int length = len;
	char *address = (char*) ptr;

	for (Segment *segment = segments; segment; n += segment->length, segment = segment->next)
		if (n + segment->length >= offset)
			{
			int off = offset - n;
			int l = MIN (length, segment->length - off);
			char *p = segment->address + off;
			for (char *end = p + l; p < end;)
				{
				char c = *address++ = *p++;
				--length;
				if (c == delimiter)
					return len - length;
				}
			if (!length)
				break;
			}

	return len - length;
}


char* Stream::alloc(long length)
{
	Segment *segment = allocSegment (length);
	segment->length = length;
	totalLength += length;

	return segment->address;
}

void Stream::putSegment(const char * string)
{
	if (string [0])
		putSegment (strlen (string), string, true);
}

char* Stream::getString()
{
	char *string = new char [totalLength + 1];
	getSegment (0, totalLength, string);
	string [totalLength] = 0;

	return string;
}

void Stream::compress(int length, void * address)
{
	//printShorts ("Original data", (length + 1) / 2, (short*) address);
	Segment *segment = allocSegment (length + 5);
	short *q = (short*) segment->address;
	short *p = (short*) address;
	short *end = p + (length + 1) / 2;
	short *yellow = end - 2;
	*q++ = length;

	while (p < end)
		{
		short *start = ++q;
		while (p < end && 
			   ((p > yellow) || (p [0] != p [1] || p [1] != p [2])))
			*q++ = *p++;
		int n = q - start;
		if (n)
			start [-1] = -n;
		else
			--q;
		if (p >= end)
			break;
		start = p++;
		while (p < end && *p == *start)
			++p;
		n = p - start;
		*q++ = n;
		*q++ = *start;
		}

	totalLength = segment->length = (char*) q - segment->address;
	//printShorts ("compressed", q - (short*) segment->address, (short*) segment->address);
}

char* Stream::decompress()
{
	char *data;
	short *q, *limit;
	int run = 0;
	decompressedLength = 0;

	for (Segment *segment = segments; segment; segment = segment->next)
		{
		if (segment->length == 0)
			continue;
		short *p = (short*) segment->address;
		short *end = (short*) (segment->address + segment->length);
		if (decompressedLength == 0)
			{
			decompressedLength = *p++;
			if (decompressedLength <= 0)
				throw SQLEXCEPTION (RUNTIME_ERROR, "corrupted record");
			data = new char [(decompressedLength + 1) / 2 * 2];
			limit = (short*) (data + decompressedLength);
			q = (short*) data;
			}
		while (p < end)
			{
			short n = *p++;
			if (n == 0 && run == 0)
				{
				printShorts ("Zero run", (segment->length + 1)/2, (short*) segment->address);
				printChars ("Zero run", segment->length, segment->address);
				}
			if (run > 0)
				for (; run; --run)
					*q++ = n;
			else if (run < 0)
				{
				*q++ = n;
				++run;
				}
			else
				{
				run = n;
				if (q + run > limit)
					{
					printShorts ("Compressed", (segment->length + 1)/2, (short*) segment->address);
					printChars ("Compressed", segment->length, segment->address);
					if (q == limit)
						return data;
					throw SQLEXCEPTION (RUNTIME_ERROR, "corrupted record");
					}
				}
			}
		}
	
	//printShorts ("Decompressed", (decompressedLength + 1) / 2, (short*) data);	
	return data;
}

void Stream::printShorts(const char * msg, int length, short * data)
{
	printf ("%s", msg);

	for (int n = 0; n < length; ++n)
		{
		if (n % 10 == 0)
			printf ("\n    ");
		printf ("%d, ", data [n]);
		}

	printf ("\n");
}

void Stream::clear()
{
	Segment *segment;

	while (segment = segments)
		{
		segments = segment->next;
		if (segment != &first)
			free (segment);
		}

	current = NULL;
	totalLength = 0;
}

void Stream::putSegment(Stream * stream)
{
	for (Segment *segment = stream->segments; segment; segment = segment->next)
		putSegment (segment->length, segment->address, true);
}

void Stream::putSegment(int length, const unsigned short *chars)
{
	totalLength += length;
	const unsigned short *wc = chars;

	if (!segments)
		{
		allocSegment (MAX (length, minSegment));
		current->length = length;
		}
	else
		{
		int l = currentLength - current->length;
		if (l > 0)
			{
			int l2 = MIN (l, length);
			char *p = current->address + current->length;
			for (int n = 0; n < l2; ++n)
				*p++ = (char) *wc++;
			//memcpy (current->address + current->length, address, l2);
			current->length += l2;
			length -= l2;
			//address += l2;
			}
		if (length)
			{
			allocSegment (MAX (length, minSegment));
			current->length = length;
			//memcpy (current->address, address, length);
			}
		}

	char *p = current->address;

	for (int n = 0; n < length; ++n)
		*p++ = (char) *wc++;
}

int Stream::getLength()
{
	return totalLength;
}


void Stream::printChars(const char * msg, int length, const char * data)
{
	printf ("%s", msg);

	for (int n = 0; n < length; ++n)
		{
		if (n % 50 == 0)
			printf ("\n    ");
		char c = data [n];
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
			putchar (c);
		else
			putchar ('.');
		}

	printf ("\n");
}

int Stream::getSegmentLength(int offset)
{
	int n = 0;

	for (Segment *segment = segments; segment; segment = segment->next)
		{
		if (offset >= n && offset < n + segment->length)
			return n + segment->length - offset;
		n += segment->length;
		}

	return 0;
}

void* Stream::getSegment(int offset)
{
	int n = 0;

	for (Segment *segment = segments; segment; segment = segment->next)
		{
		if (offset >= n && offset < n + segment->length)
			return segment->address + offset - n;
		n += segment->length;
		}

	return NULL;
}

void Stream::putSegment(Blob * blob)
{
	for (int n, offset = 0; n = blob->getSegmentLength (offset); offset += n)
		putSegment (n, (const char*) blob->getSegment (offset), true);
}

}; // end namespace IscDbcLibrary
