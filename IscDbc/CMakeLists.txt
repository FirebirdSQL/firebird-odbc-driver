# IscDbc — Firebird database connectivity library (static)

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

# Create IscDbc static library
add_library(IscDbc STATIC ${ISCDBC_SOURCES})

# Include directories
target_include_directories(IscDbc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${FIREBIRD_INCLUDE_DIR}
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(IscDbc PRIVATE WIN32 _WINDOWS)
    target_link_libraries(IscDbc PRIVATE ws2_32 advapi32)
else()
    target_compile_definitions(IscDbc PRIVATE UNIX)
    target_link_libraries(IscDbc PRIVATE dl pthread)
endif()
