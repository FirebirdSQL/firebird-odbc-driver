#pragma once

#include <vector>
#include <cassert>
#include "Sqlda.h"

namespace IscDbcLibrary {

class IscStatement;

/// Client-side static cursor for scrollable result sets.
/// Materializes all rows in blocks, handles BLOB/ARRAY column deep-copy.
/// Extracted from Sqlda.cpp (Phase 14.4.7.5a).
class CDataStaticCursor
{
public:
	using vchar_t = CAttrSqlVar::buffer_t;
	using orgsqlvar_t = std::vector<CAttrSqlVar>;
	using rowBlock_t = std::vector<vchar_t>;

	struct RowBlock {
		rowBlock_t rows;
		size_t size() { return rows.size(); }
		RowBlock(size_t row_count = 0, int row_len = 0) :
			rows{ row_count, static_cast<vchar_t>(row_len) }
		{}
	};

	CDataStaticCursor(IscStatement* stmt, vchar_t& buffer, orgsqlvar_t& sqlVars, unsigned columnsCount, int lnRow);
	~CDataStaticCursor();

	vchar_t& addRow();

	inline void restoreOriginalAdressFieldsSqlDa()
	{
		for (auto& var : ptSqlVars) var.assignBuffer(ptOrgRowBlock);
	}

	bool current(int nRow);
	void getAdressFieldFromCurrentRowInBufferStaticCursor(int column, char*& sqldata, short*& sqlind);
	vchar_t& nextPosition();

	inline int getCountRowsStaticCursor()
	{
		return countAllRows;
	}

	inline void operator<<(vchar_t& buf)
	{
		nextPosition() = buf;
	}

	inline void operator>>(vchar_t& buf)
	{
		buf = nextPosition();
	}

	inline void copyToBuffer(vchar_t& buf)
	{
		buf = *itCurrentRow;
	}

	inline void copyToCurrentSqlda(vchar_t& buf)
	{
		*itCurrentRow = buf;
	}

	rowBlock_t::reverse_iterator itCurrentRow;

private:
	orgsqlvar_t& ptSqlVars;
	bool	bYesBlob;
	static constexpr int nMAXROWBLOCK = 40;
	int		lenRow;
	int		countBlocks;
	int		countAllRows;
	int		curBlock;

	std::vector<RowBlock> listBlocks;

	vchar_t& ptOrgRowBlock;
	unsigned numberColumns;
	int		minRow;
	int		maxRow;
	int		curRow;
	std::vector<short> numColumnBlob;
	short	countColumnBlob;
	IscStatement* statement;
};

} // end namespace IscDbcLibrary
