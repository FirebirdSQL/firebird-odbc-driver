CPPFLAGS = -g -D_REENTRANT -D_PTHREADS -DEXTERNAL
ENGDIR = ../../netfrastructure/Engine
REMDIR = ../../netfrastructure/Remote
TESTDIR = ../JdbcTest
INTERBASE = /usr/interbase/lib/gds.so.0 -ldl -lcrypt
LIBRARY = IscDbc00.so
LIB = IscIscc00.a
LIBS =  -lpthread -lm

REMOTE = \


ENGINE = \
	AsciiBlob.o \
	Blob.o \
	BinaryBlob.o \
	DateTime.o \
	Error.o \
	JString.o \
	LinkedList.o \
	Parameter.o \
	Parameters.o \
	Stream.o \
	SQLError.o \
	TimeStamp.o \
	Value.o \
	Values.o \

MODULES = \
	AsciiBlob.o \
	BinaryBlob.o \
	DateTime.o \
	Error.o \
	JString.o \
	LinkedList.o \
	Parameter.o \
	Parameters.o \
	Stream.o \
	SQLError.o \
	TimeStamp.o \
	Value.o \
	Values.o \
	Attachment.o \
	IscBlob.o \
	IscColumnsResultSet.o \
	IscCallableStatement.o \
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
	IscSqlType.o \
	IscStatement.o \
	IscStatementMetaData.o \
	IscTablesResultSet.o \
	Lock.o \
	Mutex.o \
	Sqlda.o \

TEST = Test.o $(LIBRARY) 

main:	$(LIBRARY) test

$(LIBRARY)	: $(MODULES)
	g++ -g -shared -rdynamic -export-dynamic $(MODULES) $(INTERBASE) -o $(LIBRARY)

$(LIB)	: $(MODULES)
	ar crs $(LIB) $(MODULES)

driver: $(DRIVER) $(LIBRARY)
	g++ -g $(DRIVER) $(LIBRARY) $(LIBS) -o driver

clean: 
	rm $(MODULES)

dependencies:
	-make clean >/dev/null
	-rm *.d
	make CPPFLAGS='$(CPPFLAGS) -MMD'
	cat *.d > makefile.in

assemble:
	make CPPFLAGS='$(CPPFLAGS) -S'
	
include makefile.in

IscConnection.o : IscConnection.cpp
IscDatabaseMetaData.o : IscDatabaseMetaData.cpp
IscPreparedStatement.o : IscPreparedStatement.cpp
IscResultList.o : IscResultList.cpp
IscResultSet.o:IscResultSet.cpp
IscResultSetMetaData.o : IscResultSetMetaData.cpp
IscStatement.o: IscStatement.cpp
IscStatementMetaData.o: IscStatementMetaData.cpp
IscProceduresResultSet.o: IscProceduresResultSet.cpp
IscCallableStatement.o: IscCallableStatement.cpp
Mutex.o: Mutex.cpp
Lock.o: Lock.cpp
Attachment.o: Attachment.cpp

AsciiBlob.o : AsciiBlob.cpp
Blob.o : Blob.cpp
BinaryBlob.o : BinaryBlob.cpp
DateTime.o : DateTime.cpp
Error.o : Error.cpp
JString.o : JString.cpp
LinkedList.o : LinkedList.cpp
Parameter.o : Parameter.cpp
Parameters.o : Parameters.cpp
Protocol.o : Protocol.cpp
Socket.o : Socket.cpp
Stream.o : Stream.cpp
SQLError.o : SQLError.cpp
TimeStamp.o : TimeStamp.cpp
Value.o : Value.cpp
Values.o : Values.cpp

test	: Test.o $(TEST)
	g++ -g $(TEST) -o test
	
Test.o : $(TESTDIR)/Test.cpp
	$(COMPILE.C) $< -I../IscDbc $(OUTPUT_OPTION)

