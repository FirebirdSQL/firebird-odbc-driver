
#ifndef _WIN32
#include <odbcinstext.h>

#include "SetupAttributes.h"

static const char *charsets []= 
{ 
	"NONE", "ASCII", "BIG_5", "CYRL", "DOS437", "DOS850", "DOS852", "DOS857", "DOS860",
	"DOS861", "DOS863", "DOS865", "DOS866", "EUCJ_0208", "GB_2312", "ISO8859_1", 
	"ISO8859_2", "KSC_5601", "OCTETS", "SJIS_0208", "UNICODE_FSS", 
	"WIN1250", "WIN1251", "WIN1252", "WIN1253", "WIN1254", NULL
};

static const char *aYesNo[] =
{
	"Yes",
	"No",
	NULL
};

static const char *d3_1[] =
{
	"3",
	"1",
	NULL
};

static const char *szHelpPassword = "Your Password will be used to gain additional information from the DBMS and will not be saved anywhere.";
static const char *szHelpReadOnly = "Init transaction (default Write)";
static const char *szHelpNoWait = "Init transaction (default Wait)";
static const char *szHelpQuotedIdentifier = "Quoted identifier (default Yes)";
static const char *szHelpSensitiveIdentifier = "Sensitive identifier (default No)";
static const char *szHelpAutoQuotedIdentifier = "On auto quoted identifier (default No)";

int ODBCINSTGetProperties( HODBCINSTPROPERTY hLastProperty )
{ 

    hLastProperty->pNext 	    = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty 		    = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType	    = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, SETUP_DBNAME, INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "localhost:", INI_MAX_PROPERTY_VALUE );

    hLastProperty->pNext 	    = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty 		    = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType	    = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, SETUP_CLIENT, INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

    hLastProperty->pNext 	    = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty 		    = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType	    = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, SETUP_USER, INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

    hLastProperty->pNext 	    = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty 		    = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType	    = ODBCINST_PROMPTTYPE_TEXTEDIT;
    hLastProperty->pszHelp	    = (char *)strdup( szHelpPassword );
    strncpy( hLastProperty->szName, SETUP_PASSWORD, INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

    hLastProperty->pNext 	    = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty 		    = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType	    = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy( hLastProperty->szName, SETUP_ROLE, INI_MAX_PROPERTY_NAME );
    strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

    hLastProperty->pNext            = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty                   = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType      = ODBCINST_PROMPTTYPE_COMBOBOX;
    hLastProperty->bRefresh	    = TRUE;
    hLastProperty->aPromptData      = (char**)malloc( sizeof(charsets) );
    memcpy( hLastProperty->aPromptData, charsets, sizeof(charsets) );
    strncpy( hLastProperty->szName, SETUP_CHARSET, INI_MAX_PROPERTY_NAME );
    strcpy( hLastProperty->szValue, "NONE" );
	
    hLastProperty->pNext            = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty                   = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->pszHelp	    = (char *)strdup( szHelpReadOnly );
    hLastProperty->nPromptType      = ODBCINST_PROMPTTYPE_COMBOBOX;
    hLastProperty->bRefresh	    = TRUE;
    hLastProperty->aPromptData      = (char**)malloc( sizeof(aYesNo) );
    memcpy( hLastProperty->aPromptData, aYesNo, sizeof(aYesNo) );
    strncpy( hLastProperty->szName, SETUP_READONLY_TPB, INI_MAX_PROPERTY_NAME );
    strcpy( hLastProperty->szValue, "No" );
	
    hLastProperty->pNext            = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty                   = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->pszHelp	    = (char *)strdup( szHelpNoWait );
    hLastProperty->nPromptType      = ODBCINST_PROMPTTYPE_COMBOBOX;
    hLastProperty->bRefresh	    = TRUE;
    hLastProperty->aPromptData      = (char**)malloc( sizeof(aYesNo) );
    memcpy( hLastProperty->aPromptData, aYesNo, sizeof(aYesNo) );
    strncpy( hLastProperty->szName, SETUP_NOWAIT_TPB, INI_MAX_PROPERTY_NAME );
    strcpy( hLastProperty->szValue, "No" );

    hLastProperty->pNext            = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty                   = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->pszHelp	    = (char *)strdup( szHelpNoWait );
    hLastProperty->nPromptType      = ODBCINST_PROMPTTYPE_COMBOBOX;
    hLastProperty->bRefresh	    = TRUE;
    hLastProperty->aPromptData      = (char**)malloc( sizeof(aYesNo) );
    memcpy( hLastProperty->aPromptData, d3_1, sizeof(aYesNo) );
    strncpy( hLastProperty->szName, SETUP_DIALECT, INI_MAX_PROPERTY_NAME );
    strcpy( hLastProperty->szValue, "3" );

    hLastProperty->pNext            = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty                   = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->pszHelp	    = (char *)strdup( szHelpQuotedIdentifier );
    hLastProperty->nPromptType      = ODBCINST_PROMPTTYPE_COMBOBOX;
    hLastProperty->bRefresh	    = TRUE;
    hLastProperty->aPromptData      = (char**)malloc( sizeof(aYesNo) );
    memcpy( hLastProperty->aPromptData, aYesNo, sizeof(aYesNo) );
    strncpy( hLastProperty->szName, SETUP_QUOTED, INI_MAX_PROPERTY_NAME );
    strcpy( hLastProperty->szValue, "Yes" );

    hLastProperty->pNext            = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty                   = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->pszHelp	    = (char *)strdup( szHelpSensitiveIdentifier );
    hLastProperty->nPromptType      = ODBCINST_PROMPTTYPE_COMBOBOX;
    hLastProperty->bRefresh	    = TRUE;
    hLastProperty->aPromptData      = (char**)malloc( sizeof(aYesNo) );
    memcpy( hLastProperty->aPromptData, aYesNo, sizeof(aYesNo) );
    strncpy( hLastProperty->szName, SETUP_SENSITIVE, INI_MAX_PROPERTY_NAME );
    strcpy( hLastProperty->szValue, "No" );

    hLastProperty->pNext            = (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
    hLastProperty                   = hLastProperty->pNext;
    memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->pszHelp	    = (char *)strdup( szHelpAutoQuotedIdentifier );
    hLastProperty->nPromptType      = ODBCINST_PROMPTTYPE_COMBOBOX;
    hLastProperty->bRefresh	    = TRUE;
    hLastProperty->aPromptData      = (char**)malloc( sizeof(aYesNo) );
    memcpy( hLastProperty->aPromptData, aYesNo, sizeof(aYesNo) );
    strncpy( hLastProperty->szName, SETUP_AUTOQUOTED, INI_MAX_PROPERTY_NAME );
    strcpy( hLastProperty->szValue, "No" );

    return 1;
}

#endif
