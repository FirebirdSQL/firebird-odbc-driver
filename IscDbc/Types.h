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
 *
 */



#ifndef __TYPE_H
#define __TYPE_H

namespace IscDbcLibrary {

enum Type {
	Null,
	String,			// generic, null terminated
	Char,			// fixed length string, also null terminated
	Varchar,		// variable length, counted string

	Short,
	Long,
	Quad,

	Float,
	Double,

	Date,
	TimeType,
	Timestamp,

	Asciiblob,		// on disk blob
	Binaryblob,		// on disk blob
	BlobPtr,		// pointer to Blob object
	SqlTimestamp,	// 64 bit version

	Boolean
	};

// Type Encoded Record Types

#define TER_ENCODING_INTEL		1	// Intel encoding
#define TER_FORMAT_VERSION_1	1
#define TER_FORMAT				TER_FORMAT_VERSION_1

enum TerType {
	terNull,
	terString,			// 32 bit count followed by string followed by null byte
	terUnicode,			// 32 bit count followed by string followed by null character

	terShort,
	terLong,
	terQuad,

	terFloat,
	terDouble,

	terDate,
	terTime,
	terTimestamp,

	terBinaryBlob,		// 32 bit length followed by blob
	};


struct FieldType {
	Type type;
	int length;
	int scale;
	};


}; // end namespace IscDbcLibrary

#endif
