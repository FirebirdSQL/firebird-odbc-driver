// Print.h: interface for the Print class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PRINT_H__34310C02_2C14_11D4_98E0_0000C01D2301__INCLUDED_)
#define AFX_PRINT_H__34310C02_2C14_11D4_98E0_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

struct Column
{
	char	name [32];
	SWORD	sqlType;
	DWORD	precision;
	SWORD	scale;
	SWORD	nullable;
	int		length;					// max (column name, precision)
	int		headerLength;
	int		offset;					// offset in print line
public:
	void reset();
};

class Print  
{
public:
	void printAll();
	void execute();
	void skip();
	void reset();
	void getDescription();
	void printLine();
	void printHeaders();
	Print(HSTMT statementHandle);
	virtual ~Print();

	HSTMT	statement;
	int		numberColumns;
	Column	*columns;
	char	*buffer;
};

#endif // !defined(AFX_PRINT_H__34310C02_2C14_11D4_98E0_0000C01D2301__INCLUDED_)
