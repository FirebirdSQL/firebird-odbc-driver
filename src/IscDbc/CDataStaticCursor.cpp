#include "IscDbc.h"
#include "CDataStaticCursor.h"
#include "IscBlob.h"
#include "IscArray.h"
#include "IscStatement.h"

namespace IscDbcLibrary {

CDataStaticCursor::CDataStaticCursor(IscStatement* stmt, vchar_t& buffer, orgsqlvar_t& sqlVars, unsigned columnsCount, int lnRow)
	: ptSqlVars{ sqlVars },
	  ptOrgRowBlock{ buffer }
{
	statement = stmt;
	bYesBlob = false;
	lenRow = lnRow;

	countBlocks = 10;
	countAllRows = 0;
	listBlocks.resize(countBlocks);
	listBlocks.at(0) = { nMAXROWBLOCK, lnRow };
	itCurrentRow = std::prev(listBlocks.at(0).rows.rend()); // == begin()

	curBlock = 0;
	minRow = 0;
	maxRow = nMAXROWBLOCK;
	curRow = 0;
	numberColumns = columnsCount;

	for (auto& var : ptSqlVars)
	{
		switch (var.sqltype)
		{
		case SQL_ARRAY:
		case SQL_BLOB:
			numColumnBlob.push_back(var.index - 1);
			break;
		}
		var.assignBuffer(*itCurrentRow);
	}
	countColumnBlob = (short)numColumnBlob.size();
	bYesBlob = countColumnBlob > 0;
}

CDataStaticCursor::~CDataStaticCursor()
{
	if (bYesBlob)
	{
		for (auto i = 0; i < countColumnBlob; ++i)
		{
			auto& var = ptSqlVars.at(numColumnBlob[i]);

			if (var.sqltype == SQL_ARRAY || var.sqltype == SQL_BLOB)
			{
				for (auto& rowBlock : listBlocks)
				{
					for (unsigned i = 0; i < rowBlock.size(); ++i)
					{
						auto& row = rowBlock.rows.at(i);
						auto* pt = &row.at(var.offsetData);

						if (pt && *(intptr_t*)pt)
						{
							if (var.sqltype == SQL_ARRAY)
							{
								free(((CAttrArray*)*(intptr_t*)pt)->arrBufData);
								delete (CAttrArray*)*(intptr_t*)pt;
							}
							else
							{
								delete (IscBlob*)*(intptr_t*)pt;
							}
						}
					}
				}
			}
		}
	}
}

CDataStaticCursor::vchar_t& CDataStaticCursor::addRow()
{
	if (bYesBlob)
	{
		int n;
		auto& sqlvar = ptSqlVars;
		for (n = 0; n < countColumnBlob; ++n)
		{
			auto& var = sqlvar.at(numColumnBlob[n]);
			if (*var.sqlind == -1)
				*(intptr_t*)var.sqldata = 0;
			else if ((var.sqltype) == SQL_ARRAY)
			{
				CAttrArray* ptArr = new CAttrArray;
				IscArray iscArr(statement, &var);
				iscArr.getBytesFromArray();
				iscArr.detach(ptArr);
				*(intptr_t*)var.sqldata = (intptr_t)ptArr;
			}
			else if (var.sqltype == SQL_BLOB)
			{
				IscBlob* ptBlob = new IscBlob(statement, var.sqldata, var.sqlsubtype);
				ptBlob->fetchBlob();
				*(intptr_t*)var.sqldata = (intptr_t)ptBlob;
			}
		}
	}

	this->copyToCurrentSqlda(ptOrgRowBlock);	//save fetched buffer to current row
	nextPosition();								//scroll to the next position
	auto& row = *itCurrentRow;

	for (auto& var : ptSqlVars) var.assignBuffer(row);

	++countAllRows;
	return row;
}

bool CDataStaticCursor::current(int nRow)
{
	int i, n;

	assert(nRow >= 0);

	if (!(nRow >= minRow && nRow < maxRow))
	{
		for (i = 0, n = listBlocks.at(i).size();
			nRow > n && i < countBlocks;
			n += listBlocks.at(++i).size());

		curBlock = i;
		maxRow = n;
		minRow = maxRow - listBlocks.at(curBlock).size();
	}

	curRow = nRow;
	itCurrentRow = listBlocks.at(curBlock).rows.rend();
	std::advance(itCurrentRow, -(curRow - minRow));
	--curRow; // We put previous for use next() !!!

	return true;
}

void CDataStaticCursor::getAdressFieldFromCurrentRowInBufferStaticCursor(int column, char*& sqldata, short*& sqlind)
{
	auto it = std::prev(itCurrentRow);
	auto& row = *it;
	auto& var = ptSqlVars.at(column - 1);
	sqldata = &row.at(var.offsetData);
	sqlind = (short*)&row.at(var.offsetNull);
}

CDataStaticCursor::vchar_t& CDataStaticCursor::nextPosition()
{
	if (++curRow < maxRow)
	{
		std::advance(itCurrentRow, -1); // == ++it
	}
	else
	{
		if (++curBlock == countBlocks)
		{
			countBlocks += 10;
			listBlocks.resize(countBlocks);
		}

		if (listBlocks.at(curBlock).rows.size() == 0)
		{
			listBlocks.at(curBlock) = { nMAXROWBLOCK, lenRow };
		}

		itCurrentRow = std::prev(listBlocks.at(curBlock).rows.rend());
		minRow = curRow;
		maxRow = minRow + listBlocks.at(curBlock).size();
	}

	return *itCurrentRow;
}

} // end namespace IscDbcLibrary
