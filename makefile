CPPFLAGS = -g -Wall -I$(ISCDIR) -I$(ODBC)/include -D_REENTRANT -D_PTHREADS -DEXTERNAL
ISCDIR = IscDbc
ODBC = /opt/odbc
LIBRARIES = -L$(ODBC)/lib -llibodbcinst.so -ldl
LIBRARIES = -L$(ODBC)/lib -lodbcinst -ldl
INTERBASE = -L/usr/interbase/lib -lgds.so.0 -ldl -lcrypt
LIBRARY = ivInterBase00.so
LIB = ivInterbase00.a
LIBS =  $(LIBRARIES) -ldl -lpthread -lm 

REMOTE = \


ISCDBC = \
	$(ISCDIR)/DateTime.o \
	$(ISCDIR)/JString.o \
	$(ISCDIR)/TimeStamp.o \
	$(ISCDIR)/SQLError.o \
        $(ISCDIR)/SqlTime.o \

UNUSED = \
	Error.o \

MODULES = \
	Main.o \
	OdbcDesc.o \
	OdbcError.o \
	OdbcEnv.o \
	OdbcConnection.o \
	OdbcStatement.o \
	OdbcObject.o \
	DescRecord.o \
	OdbcDateTime.o \
	$(REMOTE) \
	$(ISCDBC)

TEST = \
	Test/SimpleTest.o \
	Test/Print.o \
	Test/Odbc.o \


all:	$(LIBRARY) test odbc

test: $(TEST) $(LIBRARY) $(LIB) $(MODULES)
	g++ -g $(TEST) $(MODULES) $(LIBRARIES) -o test
	
odbc: $(TEST) $(LIBRARY) $(LIB) JString.o
	g++ $(TEST) JString.o $(ODBC)/lib/libodbc.so -o odbc

$(LIBRARY)	: $(MODULES)
	ar crs $(LIB) $(MODULES)
	g++ -g -rdynamic -export-dynamic $(MODULES) $(LIBRARIES) -shared -o $(LIBRARY)

$(LIB)	: $(MODULES)
	ar crs $(LIB) $(MODULES)

driver: $(DRIVER) $(LIBRARY)
	g++ -g $(DRIVER) $(LIBRARY) $(LIBS) -o driver

clean: 
	echo $(ISCDIR)
	rm $(MODULES)

dependencies:
	-make clean >/dev/null
	-rm *.d
	make CPPFLAGS='$(CPPFLAGS) -MMD'
	cat *.d > makefile.in

assemble:
	make CPPFLAGS='$(CPPFLAGS) -S'
	
include makefile.in

Main.o : Main.cpp
OdbcObject.o : OdbcObject.cpp
OdbcDesc.o : OdbcDesc.cpp
OdbcEnv.o : OdbcEnv.cpp
OdbcConnection.o : OdbcConnection.cpp
OdbcStatement.o : OdbcStatement.cpp
OdbcError.o : OdbcError.cpp
DescRecord.o : DescRecord.cpp
OdbcDateTime.o : OdbcDateTime.cpp

DateTime.o : $(ISCDIR)/DateTime.cpp
	$(COMPILE.C) $< $(OUTPUT_OPTION)

Error.o : $(ISCDIR)/Error.cpp
	$(COMPILE.C) $< $(OUTPUT_OPTION)

JString.o : $(ISCDIR)/JString.cpp
	$(COMPILE.C) $< $(OUTPUT_OPTION)

LinkedList.o : $(ISCDIR)/LinkedList.cpp
	$(COMPILE.C) $< $(OUTPUT_OPTION)

Protocol.o : $(ISCDIR)/Protocol.cpp
	$(COMPILE.C) $< $(OUTPUT_OPTION)

SQLError.o : $(ISCDIR)/SQLError.cpp
	$(COMPILE.C) $< $(OUTPUT_OPTION)

SQLTime.o : $(ISCDIR)/SqlTime.cpp
	$(COMPILE.C) $< $(OUTPUT_OPTION)

Synchronize.o : $(ISCDIR)/Synchronize.cpp
	$(COMPILE.C) $< $(OUTPUT_OPTION)

TimeStamp.o : $(ISCDIR)/TimeStamp.cpp
	$(COMPILE.C) $< $(OUTPUT_OPTION)

Test.o : $(TESTDIR)/Test.cpp
	$(COMPILE.C) $< $(OUTPUT_OPTION)

