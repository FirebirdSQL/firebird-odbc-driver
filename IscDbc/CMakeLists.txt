# IscDbc — Firebird database connectivity library (object library)
# Built as OBJECT to avoid an unnecessary intermediate static archive.
# Object files are consumed directly by the main OdbcFb target.

set(ISCDBC_SOURCES
    Attachment.cpp
    BinaryBlob.cpp
    Blob.cpp
    DateTime.cpp
    EnvShare.cpp
    extodbc.cpp
    IscArray.cpp
    IscBlob.cpp
    IscCallableStatement.cpp
    IscColumnKeyInfo.cpp
    IscColumnPrivilegesResultSet.cpp
    IscColumnsResultSet.cpp
    IscConnection.cpp
    IscCrossReferenceResultSet.cpp
    IscDatabaseMetaData.cpp
    IscIndexInfoResultSet.cpp
    IscMetaDataResultSet.cpp
    IscOdbcStatement.cpp
    IscPreparedStatement.cpp
    IscPrimaryKeysResultSet.cpp
    IscProcedureColumnsResultSet.cpp
    IscProceduresResultSet.cpp
    IscResultSet.cpp
    IscResultSetMetaData.cpp
    IscSpecialColumnsResultSet.cpp
    IscSqlType.cpp
    IscStatement.cpp
    IscStatementMetaData.cpp
    IscTablePrivilegesResultSet.cpp
    IscTablesResultSet.cpp
    IscUserEvents.cpp
    JString.cpp
    LinkedList.cpp
    LoadFbClientDll.cpp
    Lock.cpp
    MultibyteConvert.cpp
    Mutex.cpp
    Parameter.cpp
    ParameterEvent.cpp
    Parameters.cpp
    ParametersEvents.cpp
    ServiceManager.cpp
    Sqlda.cpp
    SQLError.cpp
    SqlTime.cpp
    Stream.cpp
    SupportFunctions.cpp
    TimeStamp.cpp
    TypesResultSet.cpp
    Value.cpp
    Values.cpp
)

# Create IscDbc object library
add_library(IscDbc OBJECT ${ISCDBC_SOURCES})

# Include directories
target_include_directories(IscDbc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${FIREBIRD_INCLUDE_DIR}
)
