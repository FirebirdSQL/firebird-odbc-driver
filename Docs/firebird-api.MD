# Firebird OO API Reference for Driver Authors

Source analyzed: `firebird_doc/Using_OO_API.html`

This reference is optimized for implementers of ODBC and other language drivers.
It is concise, but complete for the API surface documented in that file.

## 1. API model (important for bindings)

- Interfaces are language-neutral and versioned.
- Most calls take `IStatus*` first; status holds errors/warnings and should be checked after each call.
- Most interfaces are reference-counted (`addRef`/`release` pattern); some methods auto-release on success (`detach`, `dropDatabase`, `close`, etc.).
- Entry point for clients: `fb_get_master_interface()` -> `IMaster`.
- Bridge from legacy ISC handles to OO API (batch with ISC): `fb_get_transaction_interface`, `fb_get_statement_interface`.

## 2. Driver capability map (what to implement)

- Connect/create DB: `IProvider::attachDatabase`, `IProvider::createDatabase`.
- Disconnect/drop DB: `IAttachment::detach`, `IAttachment::dropDatabase`.
- Transactions: `IAttachment::startTransaction`; commit/rollback family in `ITransaction`.
- Prepare/execute SQL: `IAttachment::prepare`, `IAttachment::execute`, `IStatement::execute`.
- Cursors/fetch: `IAttachment::openCursor` / `IStatement::openCursor` + `IResultSet::*fetch*`.
- Parameter/column metadata: `IStatement::getInputMetadata`, `IStatement::getOutputMetadata`, `IMessageMetadata`.
- Message layout/binding: `IMessageMetadata::{getOffset,getNullOffset,getAlignedLength}` + `IUtil::setOffsets`.
- BLOB streaming: `IAttachment::{createBlob,openBlob}` + `IBlob::{getSegment,putSegment}`.
- High-throughput DML: `IAttachment::createBatch` / `IStatement::createBatch` + `IBatch`.
- Event notifications: `IAttachment::queEvents` + `IEventCallback` + `IEvents`.
- Services API: `IProvider::attachServiceManager` + `IService::{query,start}`.
- Cancel/ping: `IAttachment::cancelOperation`, `IAttachment::ping`, `IService::cancel` (embedded).

## 3. Full interface catalog (documented surface)

### Core data access

- `IMaster` (root access): `getStatus`, `getDispatcher`, `getPluginManager`, `getTimerControl`, `getDtc`, `getUtilInterface`, `getConfigManager`.
- `IProvider` (start DB/service access): `attachDatabase`, `createDatabase`, `attachServiceManager`, `shutdown`, `setDbCryptCallback`.
- `IAttachment` (`isc_db_handle` replacement): `getInfo`, `startTransaction`, `reconnectTransaction`, `compileRequest`, `transactRequest`, `createBlob`, `openBlob`, `getSlice`, `putSlice`, `executeDyn`, `prepare`, `execute`, `openCursor`, `createBatch`, `queEvents`, `cancelOperation`, `ping`, `detach`, `dropDatabase`.
- `ITransaction` (`isc_tr_handle` replacement): `getInfo`, `prepare`, `commit`, `commitRetaining`, `rollback`, `rollbackRetaining`, `disconnect`, `join`, `validate`, `enterDtc`.
- `IStatement` (`isc_stmt_handle` partial replacement): `getInfo`, `getType`, `getPlan`, `getAffectedRecords`, `getInputMetadata`, `getOutputMetadata`, `execute`, `openCursor`, `createBatch`, `setCursorName`, `free`, `getFlags`.
- `IResultSet` (cursor/fetch API): `fetchNext`, `fetchPrior`, `fetchFirst`, `fetchLast`, `fetchAbsolute`, `fetchRelative`, `isEof`, `isBof`, `getMetadata`, `close`, `getInfo`.
- `IBlob` (`isc_blob_handle` replacement): `getInfo`, `getSegment`, `putSegment`, `cancel`, `close`, `seek`.
- `IService` (`isc_svc_handle` replacement): `detach`, `query`, `start`, `cancel`.
- `IEventCallback` (event callback): `eventCallbackFunction`.
- `IEvents` (event handle object): `cancel`.
- `IDtc` (distributed transaction coordinator): `join`, `startBuilder`.
- `IDtcStart` (distributed tx builder): `addAttachment`, `addWithTpb`, `start`.

### Metadata and message layout

- `IMessageMetadata` (XSQLDA-like metadata): `getCount`, `getField`, `getRelation`, `getOwner`, `getAlias`, `getType`, `isNullable`, `getSubType`, `getLength`, `getScale`, `getCharSet`, `getOffset`, `getNullOffset`, `getBuilder`, `getMessageLength`, `getAlignment`, `getAlignedLength`.
- `IMetadataBuilder` (mutable metadata): `setType`, `setSubType`, `setLength`, `setCharSet`, `setScale`, `truncate`, `moveNameToIndex`, `remove`, `addField`, `getMetadata`, `setField`, `setRelation`, `setOwner`, `setAlias`.
- `IOffsetsCallback`: `setOffset`.

### Batch/bulk execution

- `IBatch`: `add`, `addBlob`, `appendBlobData`, `addBlobStream`, `registerBlob`, `execute`, `cancel`, `getBlobAlignment`, `getMetadata`, `setDefaultBpb`, `getInfo`.
- `IBatchCompletionState`: `getSize`, `getState`, `findError`, `getStatus`.

### Utility/numeric/time helpers

- `IStatus`: `init`, `getState`, `setErrors2`, `setWarnings2`, `setErrors`, `setWarnings`, `getErrors`, `getWarnings`, `clone`.
- `IUtil`: `getFbVersion`, `loadBlob`, `dumpBlob`, `getPerfCounters`, `executeCreateDatabase`, `decodeDate`, `decodeTime`, `encodeDate`, `encodeTime`, `formatStatus`, `getClientVersion`, `getXpbBuilder`, `setOffsets`, `getDecFloat16`, `getDecFloat34`, `decodeTimeTz`, `decodeTimeStampTz`, `encodeTimeTz`, `encodeTimeStampTz`, `getInt128`, `decodeTimeTzEx`, `decodeTimeStampTzEx`.
- `IXpbBuilder`: `clear`, `removeCurrent`, `insertInt`, `insertBigInt`, `insertBytes`, `insertTag`, `isEof`, `moveNext`, `rewind`, `findFirst`, `findNext`, `getTag`, `getLength`, `getInt`, `getBigInt`, `getString`, `getBytes`, `getBufferLength`, `getBuffer`.
- `IDecFloat16` / `IDecFloat34`: `toBcd`, `toString`, `fromBcd`, `fromString`.
- `IInt128`: `toString`, `fromString`.
- `ITimer`: `handler`.
- `ITimerControl`: `start`, `stop`.
- `IVersionCallback`: `callback`.

### Configuration and plugin framework

- `IConfig`: `find`, `findValue`, `findPos`.
- `IConfigEntry`: `getName`, `getValue`, `getIntValue`, `getBoolValue`, `getSubConfig`.
- `IConfigManager`: `getDirectory`, `getFirebirdConf`, `getDatabaseConf`, `getPluginConfig`, `getInstallDirectory`, `getRootDirectory`, `getDefaultSecurityDb`.
- `IFirebirdConf` (unanchored in A-Z section): `getKey`, `asInteger`, `asString`, `asBoolean`, `getVersion`.
- `IPluginConfig`: `getConfigFileName`, `getDefaultConfig`, `getFirebirdConf`, `setReleaseDelay`.
- `IPluginFactory`: `createPlugin`.
- `IPluginManager`: `registerPluginFactory`, `registerModule`, `unregisterModule`, `getPlugins`, `getConfig`, `releasePlugin`.
- `IPluginModule`: `doClean`.
- `IPluginSet` (unanchored in A-Z section): `getName`, `getModuleName`, `getPlugin`, `next`, `set`.
- `IPluginBase` (described in plugin implementation section): `setOwner`, `getOwner`; plugin objects are reference-counted.

### Authentication, user management, and crypt plugins

- `ICryptKey` (unanchored in wire-crypto section): `setSymmetric`, `setAsymmetric`, `getEncryptKey`, `getDecryptKey`.
- `IWireCryptPlugin`: `getKnownTypes`, `setKey`, `encrypt`, `decrypt`.
- `IWriter`: `reset`, `add`, `setType`, `setDb`.
- `IServerBlock`: `getLogin`, `getData`, `putData`, `newKey`.
- `IServer`: `authenticate`.
- `IClientBlock`: `getLogin`, `getPassword`, `getData`, `putData`, `newKey`.
- `IClient`: `authenticate`.
- `IUserField`: `entered`, `specified`, `setEntered`.
- `ICharUserField`: `get`, `set`.
- `IIntUserField`: `get`, `set`.
- `IUser`: `operation`, `userName`, `password`, `firstName`, `lastName`, `middleName`, `comment`, `attributes`, `active`, `admin`, `clear`.
- `IListUsers`: `list`.
- `ILogonInfo`: `name`, `role`, `networkProtocol`, `remoteAddress`, `authBlock`.
- `IManagement` (user management plugin): `start`, `execute`, `commit`, `rollback`.
- `ICryptKeyCallback`: `callback`, `getHashLength`, `getHashData`.
- `IDbCryptInfo`: `getDatabaseFullPath`.
- `IDbCryptPlugin`: `setKey`, `encrypt`, `decrypt`, `setInfo`.
- `IKeyHolderPlugin`: `keyCallback`, `keyHandle`, `useOnlyOwnKeys`, `chainHandle`.

### C++ helper objects (`Message.h`, for `FB_MESSAGE`)

- `FbDate`: `decode`, `getYear`, `getMonth`, `getDay`, `encode`.
- `FbTime`: `decode`, `getHours`, `getMinutes`, `getSeconds`, `getFractions`, `encode`.
- `FbTimestamp`: members `FbDate date`, `FbTime time`.
- `FbChar<N>`: `char str[N]`.
- `FbVarChar<N>`: `ISC_USHORT length`, `char str[N]`, `set(const char*)`.

## 4. Constants, flags, and return codes you must handle

### Statement / cursor

- Prepare prefetch flags: `PREPARE_PREFETCH_NONE`, `PREPARE_PREFETCH_TYPE`, `PREPARE_PREFETCH_INPUT_PARAMETERS`, `PREPARE_PREFETCH_OUTPUT_PARAMETERS`, `PREPARE_PREFETCH_LEGACY_PLAN`, `PREPARE_PREFETCH_DETAILED_PLAN`, `PREPARE_PREFETCH_AFFECTED_RECORDS`, `PREPARE_PREFETCH_FLAGS`, `PREPARE_PREFETCH_METADATA`, `PREPARE_PREFETCH_ALL`.
- `IStatement::getFlags` values: `FLAG_HAS_CURSOR`, `FLAG_REPEAT_EXECUTE`.
- Cursor open flag: `CURSOR_TYPE_SCROLLABLE`.
- `IResultSet::getInfo` item: `INF_RECORD_COUNT`.

### Status and fetch/segment completion

- `IStatus::getState` flags: `STATE_WARNINGS`, `STATE_ERRORS`.
- Completion codes: `RESULT_ERROR`, `RESULT_OK`, `RESULT_NO_DATA`, `RESULT_SEGMENT`.

### Batch API

- Batch PB tags: `VERSION1`, `TAG_MULTIERROR`, `TAG_RECORD_COUNTS`, `TAG_BUFFER_BYTES_SIZE`, `TAG_BLOB_POLICY`, `TAG_DETAILED_ERRORS`.
- Blob policies (doc uses both singular/plural naming in different places): `BLOB_NONE`, `BLOB_ID_ENGINE` / `BLOB_IDS_ENGINE`, `BLOB_ID_USER` / `BLOB_IDS_USER`, `BLOB_STREAM`.
- `IBatch::getInfo` items: `INF_BUFFER_BYTES_SIZE`, `INF_DATA_BYTES_SIZE`, `INF_BLOBS_BYTES_SIZE`, `INF_BLOB_ALIGNMENT`.
- `IBatchCompletionState` values: `EXECUTE_FAILED`, `SUCCESS_NO_INFO`, `NO_MORE_ERRORS`.

### XPB builder kinds

- `BATCH`, `BPB`, `DPB`, `SPB_ATTACH`, `SPB_START`, `SPB_SEND`, `SPB_RECEIVE`, `SPB_RESPONSE`, `TPB`.

### Plugin manager types

- `TYPE_PROVIDER`, `TYPE_AUTH_SERVER`, `TYPE_AUTH_CLIENT`, `TYPE_AUTH_USER_MANAGEMENT`, `TYPE_EXTERNAL_ENGINE`, `TYPE_TRACE`, `TYPE_WIRE_CRYPT`, `TYPE_DB_CRYPT`, `TYPE_KEY_HOLDER`.

### Config manager directory codes

- `DIR_BIN`, `DIR_SBIN`, `DIR_CONF`, `DIR_LIB`, `DIR_INC`, `DIR_DOC`, `DIR_UDF`, `DIR_SAMPLE`, `DIR_SAMPLEDB`, `DIR_INTL`, `DIR_MISC`, `DIR_SECDB`, `DIR_MSG`, `DIR_LOG`, `DIR_GUARD`, `DIR_PLUGINS`.

### Authentication and user-management constants

- Auth return codes: `AUTH_FAILED`, `AUTH_SUCCESS`, `AUTH_MORE_DATA`, `AUTH_CONTINUE`.
- User operation codes: `OP_USER_ADD`, `OP_USER_MODIFY`, `OP_USER_DELETE`, `OP_USER_DISPLAY`, `OP_USER_SET_MAP`, `OP_USER_DROP_MAP`.

## 5. Driver implementation notes from the source document

- Event support in FB3 is minimal; `IEvents` objects must be explicitly released after use.
- `IResultSet` backward/absolute/relative fetch requires scrollable cursor mode.
- Batch API supports inline blobs and external blob registration; use `IBatch::registerBlob` for very large pre-created blobs.
- For message buffers, rely on metadata offsets/alignment instead of hardcoded struct layout.
- For plugins, use `IMaster*` passed to `FB_PLUGIN_ENTRY_POINT`, not `fb_get_master_interface()`.
- The source doc explicitly says it omits details for some legacy/less-used interfaces (example: `IRequest`), and notes missing docs for ExternalEngine and Trace plugin interfaces.
