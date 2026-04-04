#pragma once

/// CAttrSqlVar.h: Column/parameter variable descriptor and supporting types.
/// Extracted from Sqlda.h (Phase 14.4.7.5c).

#include <vector>
#include <cstddef>
#include "IscArray.h"
#include <firebird/Interface.h>

namespace IscDbcLibrary {

/// Allocator that aligns memory to cache-line boundaries (64 bytes).
/// Used for fetch buffers to improve prefetch efficiency during fetch loops.
template <typename T, std::size_t Alignment = 64>
struct AlignedAllocator {
    using value_type = T;

    AlignedAllocator() noexcept = default;
    template <typename U> AlignedAllocator(const AlignedAllocator<U, Alignment>&) noexcept {}

    T* allocate(std::size_t n) {
        void* ptr = nullptr;
        std::size_t bytes = n * sizeof(T);
#ifdef _WIN32
        ptr = _aligned_malloc(bytes, Alignment);
        if (!ptr) throw std::bad_alloc();
#else
        if (posix_memalign(&ptr, Alignment, bytes)) throw std::bad_alloc();
#endif
        return static_cast<T*>(ptr);
    }

    void deallocate(T* ptr, std::size_t) noexcept {
#ifdef _WIN32
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }

    template <typename U> struct rebind { using other = AlignedAllocator<U, Alignment>; };
    template <typename U> bool operator==(const AlignedAllocator<U, Alignment>&) const noexcept { return true; }
    template <typename U> bool operator!=(const AlignedAllocator<U, Alignment>&) const noexcept { return false; }
};

struct SqlProperties
{
	unsigned		offsetData;
	unsigned		offsetNull;
	bool			isNullable;
	const char*		sqlname;
	const char*		relname;
	const char*		aliasname;
	short			sqltype;			/* datatype of field */
	short			sqlscale;			/* scale factor */
	short			sqlsubtype;			/* datatype subtype - BLOBs & Text types only */
	unsigned		sqlcharset;
	unsigned		sqllen;				/* length of data area */

	inline SqlProperties() :
		offsetData{ 0 },
		offsetNull{ 0 },
		isNullable{ false },
		sqlname{ nullptr },
		relname{ nullptr },
		aliasname{ nullptr },
		sqltype{ 0 },
		sqlscale{ 0 },
		sqlsubtype{ 0 },
		sqlcharset{ 0 },
		sqllen{ 0 }
	{}

	inline bool operator==(const SqlProperties& other) const
	{
		return
			sqltype == other.sqltype &&
			sqlscale == other.sqlscale &&
			sqlsubtype == other.sqlsubtype &&
			sqllen == other.sqllen;
	}
};

class CAttrSqlVar : public SqlProperties
{
public:
	CAttrSqlVar() :
		SqlProperties{},

		sqldata{ nullptr },
		sqlind{ nullptr },

		eff_sqldata{ nullptr }, eff_sqlind{ nullptr },

		array{ nullptr },
		index{ 0 },
		replaceForParamArray{ false }
	{}

	~CAttrSqlVar()	
	{ 
		if ( array ) 
			delete array; 
	}

	using buffer_t = std::vector<char, AlignedAllocator<char, 64>>;

private:
	inline void bindProperties( Firebird::ThrowStatusWrapper& status, Firebird::IMessageMetadata* _meta, unsigned _index ) {
		index = _index;
		//
		offsetData = _meta->getOffset    ( &status, index );
		offsetNull = _meta->getNullOffset( &status, index );
		isNullable = _meta->isNullable   ( &status, index );
		sqlname    = _meta->getField     ( &status, index );
		relname    = _meta->getRelation  ( &status, index );
		aliasname  = _meta->getAlias     ( &status, index );
		//attention - ooapi may return type+bit if getType() is called from IMetadataBuilder!
		sqltype    = _meta->getType      ( &status, index ) & ~1;
		//
		sqlscale   = _meta->getScale     ( &status, index );
		sqlsubtype = _meta->getSubType   ( &status, index );
		sqlcharset = _meta->getCharSet   ( &status, index );
		sqllen     = _meta->getLength    ( &status, index );

		//OOAPI provides subtype & charset separately
		if (sqltype == SQL_TEXT || sqltype == SQL_VARYING)
		{
			sqlsubtype = sqlcharset;
		}
		//
		++index; //to make it 1-based)

		orgSqlProperties = *this; // save original props
		lastSqlProperties = *this;
	}

public:
	inline void assign(Firebird::ThrowStatusWrapper& status, Firebird::IMessageMetadata* _meta, buffer_t& _buffer, unsigned _index)
	{
		bindProperties( status, _meta, _index );
		assignBuffer( _buffer );
	}

	inline void assignBuffer(buffer_t& buffer ) {
		eff_sqldata = sqldata = &buffer.at( offsetData );
		eff_sqlind  = sqlind  = (short*)&buffer.at( offsetNull );
	}

	/// Phase 14.4.7.1: Assign sqlvar pointers into a raw external buffer.
	inline void assignBuffer(char* buf, [[maybe_unused]] size_t bufSize) {
		eff_sqldata = sqldata = buf + offsetData;
		eff_sqlind  = sqlind  = (short*)(buf + offsetNull);
	}

	inline bool propertiesOverriden() {
		return !( *this == lastSqlProperties);
	}

	char*           sqldata;
	short*          sqlind;
	char*			eff_sqldata;
	short*			eff_sqlind;
	CAttrArray		*array;
	unsigned		index;				// 1-based parameter index

	bool			replaceForParamArray;
	SqlProperties	orgSqlProperties;	// original properties after prepare
	SqlProperties	lastSqlProperties;	// last used properties after prev exec
};

} // end namespace IscDbcLibrary
