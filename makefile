#Written by Ostash! <osspam@ukrpost.net>, 2:463/1080@fidonet

#C++ compiler name
CXX=g++
#C++ compiler flags. Should include path for InterBase/FireBird 
#headers (ibase.h)
CXXFLAGS=-g -w -D_REENTRANT -D_PTHREADS -DEXTERNAL 
#Extra libraries. Should include path for InterBase/FireBird 
#library (libgds.so) 
EXTLIBS=-lcrypt -lgds

ISCDBCDIR=IscDbc
ISCDBCLIB=IscDbc
#ODBCJDBCDIR=OdbcJdbc
ODBCJDBCDIR=.
ODBCJDBCLIB=OdbcJdbc
JDBCTESTCDIR=JdbcTest

ISCDBC= \
	AsciiBlob.o \
	Attachment.o \
	BinaryBlob.o \
	Blob.o \
	DateTime.o \
	Error.o \
	IscBlob.o \
	IscCallableStatement.o \
	IscColumnPrivilegesResultSet.o \
	IscColumnsResultSet.o \
	IscConnection.o \
	IscCrossReferenceResultSet.o \
	IscDatabaseMetaData.o \
	IscIndexInfoResultSet.o \
	IscMetaDataResultSet.o \
	IscPreparedStatement.o \
	IscPrimaryKeysResultSet.o \
	IscProcedureColumnsResultSet.o \
	IscProceduresResultSet.o \
	IscResultSet.o \
	IscResultSetMetaData.o \
	IscSpecialColumnsResultSet.o \
	IscSqlType.o \
	IscStatement.o \
	IscStatementMetaData.o \
	IscTablePrivilegesResultSet.o \
	IscTablesResultSet.o \
	JString.o \
	LinkedList.o \
	Lock.o \
	Mutex.o \
	Parameter.o \
	Parameters.o \
	Sqlda.o \
	SQLError.o \
	SqlTime.o \
	Stream.o \
	TimeStamp.o \
	TypesResultSet.o \
	Value.o \
	Values.o

ODBCJDBC= \
	Attributes.o \
	DescRecord.o \
	Main.o \
	OdbcConnection.o \
	OdbcDateTime.o \
	OdbcDesc.o \
	OdbcEnv.o \
	OdbcError.o \
	OdbcObject.o \
	OdbcStatement.o

ODBCJDBCEXTRA= \
	DateTime.o \
	JString.o \
	TimeStamp.o \
	SQLError.o \
        SqlTime.o \

all:	$(ISCDBCLIB) $(ODBCJDBCLIB) JDBCTEST

$(ISCDBCLIB)	: $(ISCDBC)
	ar crs $(ISCDBCLIB).a $(ISCDBC)
	g++ -rdynamic -export-dynamic $(ISCDBC) $(EXTLIBS) -shared -o $(ISCDBCLIB).so

$(ODBCJDBCLIB)	: $(ODBCJDBC)
	ar crs $(ODBCJDBCLIB).a $(ODBCJDBC) $(ODBCJDBCEXTRA)
	g++ -rdynamic -export-dynamic $(ODBCJDBC) $(ODBCJDBCEXTRA) $(EXTLIBS) -shared -o $(ODBCJDBCLIB).so

JDBCTEST:
	 $(CXX) $(CXXFLAGS) $(JDBCTESTCDIR)/Test.cpp $(ISCDBCLIB).a -I$(ISCDBCDIR) -o test $(EXTLIBS)

AsciiBlob.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/AsciiBlob.cpp -I$(ISCDBCDIR)
Attachment.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/Attachment.cpp -I$(ISCDBCDIR)
BinaryBlob.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/BinaryBlob.cpp -I$(ISCDBCDIR)
Blob.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/Blob.cpp -I$(ISCDBCDIR)
DateTime.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/DateTime.cpp -I$(ISCDBCDIR)
Error.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/Error.cpp -I$(ISCDBCDIR)
IscBlob.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/IscBlob.cpp -I$(ISCDBCDIR)
IscCallableStatement.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/IscCallableStatement.cpp -I$(ISCDBCDIR)
IscColumnPrivilegesResultSet.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/IscColumnPrivilegesResultSet.cpp -I$(ISCDBCDIR)
IscColumnsResultSet.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/IscColumnsResultSet.cpp -I$(ISCDBCDIR)
IscConnection.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/IscConnection.cpp -I$(ISCDBCDIR)
IscCrossReferenceResultSet.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/IscCrossReferenceResultSet.cpp -I$(ISCDBCDIR)
IscDatabaseMetaData.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/IscDatabaseMetaData.cpp -I$(ISCDBCDIR)
IscIndexInfoResultSet.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/IscIndexInfoResultSet.cpp -I$(ISCDBCDIR)
IscMetaDataResultSet.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/IscMetaDataResultSet.cpp -I$(ISCDBCDIR)
IscPreparedStatement.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/IscPreparedStatement.cpp -I$(ISCDBCDIR)
IscPrimaryKeysResultSet.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/IscPrimaryKeysResultSet.cpp -I$(ISCDBCDIR)
IscProcedureColumnsResultSet.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/IscProcedureColumnsResultSet.cpp -I$(ISCDBCDIR)
IscProceduresResultSet.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/IscProceduresResultSet.cpp -I$(ISCDBCDIR)
IscResultSet.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/IscResultSet.cpp -I$(ISCDBCDIR)
IscResultSetMetaData.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/IscResultSetMetaData.cpp -I$(ISCDBCDIR)
IscSpecialColumnsResultSet.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/IscSpecialColumnsResultSet.cpp -I$(ISCDBCDIR)
IscSqlType.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/IscSqlType.cpp -I$(ISCDBCDIR)
IscStatement.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/IscStatement.cpp -I$(ISCDBCDIR)
IscStatementMetaData.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/IscStatementMetaData.cpp -I$(ISCDBCDIR)
IscTablePrivilegesResultSet.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/IscTablePrivilegesResultSet.cpp -I$(ISCDBCDIR)
IscTablesResultSet.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/IscTablesResultSet.cpp -I$(ISCDBCDIR)
JString.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/JString.cpp -I$(ISCDBCDIR)
LinkedList.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/LinkedList.cpp -I$(ISCDBCDIR)
Lock.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/Lock.cpp -I$(ISCDBCDIR)
Mutex.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/Mutex.cpp -I$(ISCDBCDIR)
Parameter.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/Parameter.cpp -I$(ISCDBCDIR)
Parameters.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/Parameters.cpp -I$(ISCDBCDIR)
Sqlda.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/Sqlda.cpp -I$(ISCDBCDIR)
SQLError.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/SQLError.cpp -I$(ISCDBCDIR)
SqlTime.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/SqlTime.cpp -I$(ISCDBCDIR)
Stream.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/Stream.cpp -I$(ISCDBCDIR)
TimeStamp.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/TimeStamp.cpp -I$(ISCDBCDIR)
TypesResultSet.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/TypesResultSet.cpp -I$(ISCDBCDIR)
Value.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/Value.cpp -I$(ISCDBCDIR)
Values.o:
	 $(CXX) $(CXXFLAGS) -c $(ISCDBCDIR)/Values.cpp -I$(ISCDBCDIR)


Attributes.o:
	 $(CXX) $(CXXFLAGS) -c $(ODBCJDBCDIR)/Attributes.cpp -I$(ISCDBCDIR) -I$(ODBCJDBCDIR)
DescRecord.o:
	 $(CXX) $(CXXFLAGS) -c $(ODBCJDBCDIR)/DescRecord.cpp -I$(ISCDBCDIR) -I$(ODBCJDBCDIR)
Main.o:
	 $(CXX) $(CXXFLAGS) -c $(ODBCJDBCDIR)/Main.cpp -I$(ISCDBCDIR) -I$(ODBCJDBCDIR)
OdbcConnection.o:
	 $(CXX) $(CXXFLAGS) -c $(ODBCJDBCDIR)/OdbcConnection.cpp -I$(ISCDBCDIR) -I$(ODBCJDBCDIR)
OdbcDateTime.o:
	 $(CXX) $(CXXFLAGS) -c $(ODBCJDBCDIR)/OdbcDateTime.cpp -I$(ISCDBCDIR) -I$(ODBCJDBCDIR)
OdbcDesc.o:
	 $(CXX) $(CXXFLAGS) -c $(ODBCJDBCDIR)/OdbcDesc.cpp -I$(ISCDBCDIR) -I$(ODBCJDBCDIR)
OdbcEnv.o:
	 $(CXX) $(CXXFLAGS) -c $(ODBCJDBCDIR)/OdbcEnv.cpp -I$(ISCDBCDIR) -I$(ODBCJDBCDIR)
OdbcError.o:
	 $(CXX) $(CXXFLAGS) -c $(ODBCJDBCDIR)/OdbcError.cpp -I$(ISCDBCDIR) -I$(ODBCJDBCDIR)
OdbcObject.o:
	 $(CXX) $(CXXFLAGS) -c $(ODBCJDBCDIR)/OdbcObject.cpp -I$(ISCDBCDIR) -I$(ODBCJDBCDIR)
OdbcStatement.o:
	 $(CXX) $(CXXFLAGS) -c $(ODBCJDBCDIR)/OdbcStatement.cpp -I$(ISCDBCDIR) -I$(ODBCJDBCDIR)
