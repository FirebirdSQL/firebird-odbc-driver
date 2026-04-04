#pragma once

#include "Sqlda.h"

namespace IscDbcLibrary {

/// Free functions for Sqlda metadata queries.
/// Extracted from Sqlda class (Phase 14.4.7.5b) so metadata logic
/// is independent of the Sqlda container.

int sqlda_get_sql_type(const CAttrSqlVar* var, int& realSqlType);
const char* sqlda_get_sql_type_name(const CAttrSqlVar* var);
int sqlda_get_column_display_size(const SqlProperties* props, const CAttrSqlVar* fullVar);
int sqlda_get_precision(const CAttrSqlVar* var);
int sqlda_get_num_prec_radix(const CAttrSqlVar* var);

} // end namespace IscDbcLibrary
