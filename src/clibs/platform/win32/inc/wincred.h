#ifndef _WINCRED_H
#define _WINCRED_H

#ifdef __ORANGEC__ 
#pragma once
#endif

/* Windows Credential Manager API definitions */

#define WINADVAPI DECLSPEC_IMPORT
#define CREDUIAPI DECLSPEC_IMPORT

#ifdef __cplusplus
extern "C" {
#endif

#define STATUS_LOGON_FAILURE  ((NTSTATUS)0xC000006DL)
#define STATUS_WRONG_PASSWORD  ((NTSTATUS)0xC000006AL)
#define STATUS_PASSWORD_EXPIRED  ((NTSTATUS)0xC0000071L)
#define STATUS_PASSWORD_MUST_CHANGE  ((NTSTATUS)0xC0000224L)
#define STATUS_ACCESS_DENIED  ((NTSTATUS)0xC0000022L)
#define STATUS_DOWNGRADE_DETECTED  ((NTSTATUS)0xC0000388L)
#define STATUS_AUTHENTICATION_FIREWALL_FAILED  ((NTSTATUS)0xC0000413L)
#define STATUS_ACCOUNT_DISABLED  ((NTSTATUS)0xC0000072L)
#define STATUS_ACCOUNT_RESTRICTION  ((NTSTATUS)0xC000006EL)
#define STATUS_ACCOUNT_LOCKED_OUT  ((NTSTATUS)0xC0000234L)
#define STATUS_ACCOUNT_EXPIRED  ((NTSTATUS)0xC0000193L)
#define STATUS_LOGON_TYPE_NOT_GRANTED  ((NTSTATUS)0xC000015BL)

#define NERR_BASE  2100
#define NERR_PasswordExpired  (NERR_BASE+142)

#define CREDUIP_IS_USER_PASSWORD_ERROR(_Status)  ( \
    (_Status) == ERROR_LOGON_FAILURE || \
    (_Status) == HRESULT_FROM_WIN32(ERROR_LOGON_FAILURE) || \
    (_Status) == STATUS_LOGON_FAILURE || \
    (_Status) == HRESULT_FROM_NT(STATUS_LOGON_FAILURE) || \
    (_Status) == ERROR_ACCESS_DENIED || \
    (_Status) == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) || \
    (_Status) == STATUS_ACCESS_DENIED || \
    (_Status) == HRESULT_FROM_NT(STATUS_ACCESS_DENIED) || \
    (_Status) == ERROR_INVALID_PASSWORD || \
    (_Status) == HRESULT_FROM_WIN32(ERROR_INVALID_PASSWORD) || \
    (_Status) == STATUS_WRONG_PASSWORD || \
    (_Status) == HRESULT_FROM_NT(STATUS_WRONG_PASSWORD) || \
    (_Status) == SEC_E_NO_CREDENTIALS || \
    (_Status) == SEC_E_LOGON_DENIED || \
    (_Status) == SEC_E_NO_CONTEXT || \
    (_Status) == STATUS_NO_SECURITY_CONTEXT \
)

#define CREDUIP_IS_DOWNGRADE_ERROR(_Status)  ( \
    (_Status) == ERROR_DOWNGRADE_DETECTED || \
    (_Status) == HRESULT_FROM_WIN32(ERROR_DOWNGRADE_DETECTED) || \
    (_Status) == STATUS_DOWNGRADE_DETECTED || \
    (_Status) == HRESULT_FROM_NT(STATUS_DOWNGRADE_DETECTED) \
)

#define CREDUIP_IS_EXPIRED_ERROR(_Status)  ( \
    (_Status) == ERROR_PASSWORD_EXPIRED || \
    (_Status) == HRESULT_FROM_WIN32(ERROR_PASSWORD_EXPIRED) || \
    (_Status) == STATUS_PASSWORD_EXPIRED || \
    (_Status) == HRESULT_FROM_NT(STATUS_PASSWORD_EXPIRED) || \
    (_Status) == ERROR_PASSWORD_MUST_CHANGE || \
    (_Status) == HRESULT_FROM_WIN32(ERROR_PASSWORD_MUST_CHANGE) || \
    (_Status) == STATUS_PASSWORD_MUST_CHANGE || \
    (_Status) == HRESULT_FROM_NT(STATUS_PASSWORD_MUST_CHANGE) || \
    (_Status) == NERR_PasswordExpired || \
    (_Status) == HRESULT_FROM_WIN32(NERR_PasswordExpired) \
)

#define CREDUI_IS_AUTHENTICATION_ERROR(_Status)  ( \
    CREDUIP_IS_USER_PASSWORD_ERROR(_Status) || \
    CREDUIP_IS_DOWNGRADE_ERROR(_Status) || \
    CREDUIP_IS_EXPIRED_ERROR(_Status) \
)

#define CREDUI_NO_PROMPT_AUTHENTICATION_ERROR( _Status ) ( \
    (_Status) == ERROR_AUTHENTICATION_FIREWALL_FAILED || \
    (_Status) == HRESULT_FROM_WIN32(ERROR_AUTHENTICATION_FIREWALL_FAILED) || \
    (_Status) == STATUS_AUTHENTICATION_FIREWALL_FAILED || \
    (_Status) == HRESULT_FROM_NT(STATUS_AUTHENTICATION_FIREWALL_FAILED) || \
    (_Status) == ERROR_ACCOUNT_DISABLED || \
    (_Status) == HRESULT_FROM_WIN32(ERROR_ACCOUNT_DISABLED) || \
    (_Status) == STATUS_ACCOUNT_DISABLED || \
    (_Status) == HRESULT_FROM_NT(STATUS_ACCOUNT_DISABLED) || \
    (_Status) == ERROR_ACCOUNT_RESTRICTION || \
    (_Status) == HRESULT_FROM_WIN32(ERROR_ACCOUNT_RESTRICTION) || \
    (_Status) == STATUS_ACCOUNT_RESTRICTION || \
    (_Status) == HRESULT_FROM_NT(STATUS_ACCOUNT_RESTRICTION) || \
    (_Status) == ERROR_ACCOUNT_LOCKED_OUT || \
    (_Status) == HRESULT_FROM_WIN32(ERROR_ACCOUNT_LOCKED_OUT) || \
    (_Status) == STATUS_ACCOUNT_LOCKED_OUT || \
    (_Status) == HRESULT_FROM_NT(STATUS_ACCOUNT_LOCKED_OUT) || \
    (_Status) == ERROR_ACCOUNT_EXPIRED || \
    (_Status) == HRESULT_FROM_WIN32(ERROR_ACCOUNT_EXPIRED) || \
    (_Status) == STATUS_ACCOUNT_EXPIRED || \
    (_Status) == HRESULT_FROM_NT(STATUS_ACCOUNT_EXPIRED) || \
    (_Status) == ERROR_LOGON_TYPE_NOT_GRANTED || \
    (_Status) == HRESULT_FROM_WIN32(ERROR_LOGON_TYPE_NOT_GRANTED) || \
    (_Status) == STATUS_LOGON_TYPE_NOT_GRANTED || \
    (_Status) == HRESULT_FROM_NT(STATUS_LOGON_TYPE_NOT_GRANTED) \
)

#define CRED_MAX_STRING_LENGTH  256
#define CRED_MAX_USERNAME_LENGTH  (256+1+256)
#define CRED_MAX_GENERIC_TARGET_NAME_LENGTH  32767
#define CRED_MAX_DOMAIN_TARGET_NAME_LENGTH  (256+1+80)
#define CRED_MAX_TARGETNAME_NAMESPACE_LENGTH  (256)
#define CRED_MAX_TARGETNAME_ATTRIBUTE_LENGTH ( 256)
#define CRED_MAX_VALUE_SIZE  256
#define CRED_MAX_ATTRIBUTES  64

#define CRED_SESSION_WILDCARD_NAME_W  L"*Session"
#define CRED_SESSION_WILDCARD_NAME_A  "*Session"
#define CRED_UNIVERSAL_WILDCARD_W  L'*'
#define CRED_UNIVERSAL_WILDCARD_A  '*'
#define CRED_SESSION_WILDCARD_NAME_LENGTH  (sizeof(CRED_SESSION_WILDCARD_NAME_A)-1)
#define CRED_TARGETNAME_DOMAIN_NAMESPACE_W  L"Domain"
#define CRED_TARGETNAME_DOMAIN_NAMESPACE_A  "Domain"
#define CRED_TARGETNAME_DOMAIN_NAMESPACE_LENGTH  (sizeof(CRED_TARGETNAME_DOMAIN_NAMESPACE_A)-1)
#define CRED_UNIVERSAL_WILDCARD_W  L'*'
#define CRED_UNIVERSAL_WILDCARD_A  '*'
#define CRED_TARGETNAME_LEGACYGENERIC_NAMESPACE_W  L"LegacyGeneric"
#define CRED_TARGETNAME_LEGACYGENERIC_NAMESPACE_A  "LegacyGeneric"
#define CRED_TARGETNAME_LEGACYGENERIC_NAMESPACE_LENGTH  (sizeof(CRED_TARGETNAME_LEGACYGENERIC_NAMESPACE_A)-1)
#define CRED_TARGETNAME_NAMESPACE_SEPERATOR_W  L':'
#define CRED_TARGETNAME_NAMESPACE_SEPERATOR_A  ':'
#define CRED_TARGETNAME_ATTRIBUTE_SEPERATOR_W  L'='
#define CRED_TARGETNAME_ATTRIBUTE_SEPERATOR_A  '='
#define CRED_TARGETNAME_DOMAIN_EXTENDED_USERNAME_SEPARATOR_W  L'|'
#define CRED_TARGETNAME_DOMAIN_EXTENDED_USERNAME_SEPARATOR_A  '|'
#define CRED_TARGETNAME_ATTRIBUTE_TARGET_W  L"target"
#define CRED_TARGETNAME_ATTRIBUTE_TARGET_A  "target"
#define CRED_TARGETNAME_ATTRIBUTE_TARGET_LENGTH  (sizeof(CRED_TARGETNAME_ATTRIBUTE_TARGET_A)-1)
#define CRED_TARGETNAME_ATTRIBUTE_NAME_W  L"name"
#define CRED_TARGETNAME_ATTRIBUTE_NAME_A  "name"
#define CRED_TARGETNAME_ATTRIBUTE_NAME_LENGTH  (sizeof(CRED_TARGETNAME_ATTRIBUTE_NAME_A)-1)
#define CRED_TARGETNAME_ATTRIBUTE_BATCH_W  L"batch"
#define CRED_TARGETNAME_ATTRIBUTE_BATCH_A  "batch"
#define CRED_TARGETNAME_ATTRIBUTE_BATCH_LENGTH  (sizeof(CRED_TARGETNAME_ATTRIBUTE_BATCH_A)-1)
#define CRED_TARGETNAME_ATTRIBUTE_INTERACTIVE_W  L"interactive"
#define CRED_TARGETNAME_ATTRIBUTE_INTERACTIVE_A  "interactive"
#define CRED_TARGETNAME_ATTRIBUTE_INTERACTIVE_LENGTH  (sizeof(CRED_TARGETNAME_ATTRIBUTE_INTERACTIVE_A)-1)
#define CRED_TARGETNAME_ATTRIBUTE_SERVICE_W  L"service"
#define CRED_TARGETNAME_ATTRIBUTE_SERVICE_A  "service"
#define CRED_TARGETNAME_ATTRIBUTE_SERVICE_LENGTH  (sizeof(CRED_TARGETNAME_ATTRIBUTE_SERVICE_A)-1)
#define CRED_TARGETNAME_ATTRIBUTE_NETWORK_W  L"network"
#define CRED_TARGETNAME_ATTRIBUTE_NETWORK_A  "network"
#define CRED_TARGETNAME_ATTRIBUTE_NETWORK_LENGTH  (sizeof(CRED_TARGETNAME_ATTRIBUTE_NETWORK_A)-1)
#define CRED_TARGETNAME_ATTRIBUTE_NETWORKCLEARTEXT_W  L"networkcleartext"
#define CRED_TARGETNAME_ATTRIBUTE_NETWORKCLEARTEXT_A  "networkcleartext"
#define CRED_TARGETNAME_ATTRIBUTE_NETWORKCLEARTEXT_LENGTH  (sizeof(CRED_TARGETNAME_ATTRIBUTE_NETWORKCLEARTEXT_A)-1)
#define CRED_TARGETNAME_ATTRIBUTE_REMOTEINTERACTIVE_W  L"remoteinteractive"
#define CRED_TARGETNAME_ATTRIBUTE_REMOTEINTERACTIVE_A  "remoteinteractive"
#define CRED_TARGETNAME_ATTRIBUTE_REMOTEINTERACTIVE_LENGTH  (sizeof(CRED_TARGETNAME_ATTRIBUTE_REMOTEINTERACTIVE_A)-1)
#define CRED_TARGETNAME_ATTRIBUTE_CACHEDINTERACTIVE_W  L"cachedinteractive"
#define CRED_TARGETNAME_ATTRIBUTE_CACHEDINTERACTIVE_A  "cachedinteractive"
#define CRED_TARGETNAME_ATTRIBUTE_CACHEDINTERACTIVE_LENGTH  (sizeof(CRED_TARGETNAME_ATTRIBUTE_CACHEDINTERACTIVE_A)-1)

#define CRED_LOGON_TYPES_MASK  0xF000

#define CredAppendLogonTypeToFlags(Flags,LogonType)  (Flags) |= ((LogonType) << 12)
#define CredGetLogonTypeFromFlags(Flags)  ((SECURITY_LOGON_TYPE)(((Flags) & CRED_LOGON_TYPES_MASK) >> 12))
#define CredRemoveLogonTypeFromFlags(Flags)  (Flags) &= ~CRED_LOGON_TYPES_MASK

#define CRED_FLAGS_PASSWORD_FOR_CERT  0x0001
#define CRED_FLAGS_PROMPT_NOW  0x0002
#define CRED_FLAGS_USERNAME_TARGET  0x0004
#define CRED_FLAGS_PERSIST_CRED_BLOB  0x0008
#define CRED_FLAGS_REQUIRE_CONFIRMATION  0x0010
#define CRED_FLAGS_WILDCARD_MATCH  0x0020
#define CRED_FLAGS_VSM_PROTECTED  0x0040
#define CRED_FLAGS_NGC_CERT  0x0080
#define CRED_FLAGS_VALID_FLAGS  0xF0FF

#define CRED_FLAGS_VALID_INPUT_FLAGS  0xF09F

#define CRED_TYPE_GENERIC  1
#define CRED_TYPE_DOMAIN_PASSWORD  2
#define CRED_TYPE_DOMAIN_CERTIFICATE  3
#define CRED_TYPE_DOMAIN_VISIBLE_PASSWORD  4
#define CRED_TYPE_GENERIC_CERTIFICATE  5
#define CRED_TYPE_DOMAIN_EXTENDED  6
#define CRED_TYPE_MAXIMUM  7
#define CRED_TYPE_MAXIMUM_EX  (CRED_TYPE_MAXIMUM+1000)

#define CRED_MAX_CREDENTIAL_BLOB_SIZE  (5*512)

#define CRED_PERSIST_NONE  0
#define CRED_PERSIST_SESSION  1
#define CRED_PERSIST_LOCAL_MACHINE  2
#define CRED_PERSIST_ENTERPRISE  3

#define CRED_TI_SERVER_FORMAT_UNKNOWN  0x0001
#define CRED_TI_DOMAIN_FORMAT_UNKNOWN  0x0002
#define CRED_TI_ONLY_PASSWORD_REQUIRED  0x0004
#define CRED_TI_USERNAME_TARGET  0x0008
#define CRED_TI_CREATE_EXPLICIT_CRED  0x0010
#define CRED_TI_WORKGROUP_MEMBER  0x0020
#define CRED_TI_DNSTREE_IS_DFS_SERVER  0x0040
#define CRED_TI_VALID_FLAGS  0xF07F

#define CERT_HASH_LENGTH  20

#define CRED_PACK_PROTECTED_CREDENTIALS  0x1
#define CRED_PACK_WOW_BUFFER  0x2
#define CRED_PACK_GENERIC_CREDENTIALS  0x4
#define CRED_PACK_ID_PROVIDER_CREDENTIALS  0x8

#define CREDUI_MAX_MESSAGE_LENGTH  1024 /*32767*/
#define CREDUI_MAX_CAPTION_LENGTH  128
#define CREDUI_MAX_GENERIC_TARGET_LENGTH  CRED_MAX_GENERIC_TARGET_NAME_LENGTH
#define CREDUI_MAX_DOMAIN_TARGET_LENGTH  CRED_MAX_DOMAIN_TARGET_NAME_LENGTH
#define CREDUI_MAX_USERNAME_LENGTH  CRED_MAX_USERNAME_LENGTH
#define CREDUI_MAX_PASSWORD_LENGTH  (CRED_MAX_CREDENTIAL_BLOB_SIZE / 2)
#define CREDUI_MAX_PACKED_CREDENTIALS_LENGTH  ((MAXUSHORT / 2) - 2)
#define CREDUI_MAX_CREDENTIALS_BLOB_SIZE  (MAXUSHORT)

#define CREDUI_FLAGS_INCORRECT_PASSWORD  0x00001
#define CREDUI_FLAGS_DO_NOT_PERSIST  0x00002
#define CREDUI_FLAGS_REQUEST_ADMINISTRATOR  0x00004
#define CREDUI_FLAGS_EXCLUDE_CERTIFICATES  0x00008
#define CREDUI_FLAGS_REQUIRE_CERTIFICATE  0x00010
#define CREDUI_FLAGS_SHOW_SAVE_CHECK_BOX  0x00040
#define CREDUI_FLAGS_ALWAYS_SHOW_UI  0x00080
#define CREDUI_FLAGS_REQUIRE_SMARTCARD  0x00100
#define CREDUI_FLAGS_PASSWORD_ONLY_OK  0x00200
#define CREDUI_FLAGS_VALIDATE_USERNAME  0x00400
#define CREDUI_FLAGS_COMPLETE_USERNAME  0x00800
#define CREDUI_FLAGS_PERSIST  0x01000
#define CREDUI_FLAGS_SERVER_CREDENTIAL  0x04000
#define CREDUI_FLAGS_EXPECT_CONFIRMATION  0x20000
#define CREDUI_FLAGS_GENERIC_CREDENTIALS  0x40000
#define CREDUI_FLAGS_USERNAME_TARGET_CREDENTIALS  0x80000
#define CREDUI_FLAGS_KEEP_USERNAME  0x100000

#define CREDUI_FLAGS_PROMPT_VALID  ( \
    CREDUI_FLAGS_INCORRECT_PASSWORD | \
    CREDUI_FLAGS_DO_NOT_PERSIST | \
    CREDUI_FLAGS_REQUEST_ADMINISTRATOR | \
    CREDUI_FLAGS_EXCLUDE_CERTIFICATES | \
    CREDUI_FLAGS_REQUIRE_CERTIFICATE | \
    CREDUI_FLAGS_SHOW_SAVE_CHECK_BOX | \
    CREDUI_FLAGS_ALWAYS_SHOW_UI | \
    CREDUI_FLAGS_REQUIRE_SMARTCARD | \
    CREDUI_FLAGS_PASSWORD_ONLY_OK | \
    CREDUI_FLAGS_VALIDATE_USERNAME | \
    CREDUI_FLAGS_COMPLETE_USERNAME | \
    CREDUI_FLAGS_PERSIST | \
    CREDUI_FLAGS_SERVER_CREDENTIAL | \
    CREDUI_FLAGS_EXPECT_CONFIRMATION | \
    CREDUI_FLAGS_GENERIC_CREDENTIALS | \
    CREDUI_FLAGS_USERNAME_TARGET_CREDENTIALS | \
    CREDUI_FLAGS_KEEP_USERNAME \
)

#define CREDUIWIN_GENERIC  0x00000001
#define CREDUIWIN_CHECKBOX  0x00000002
#define CREDUIWIN_AUTHPACKAGE_ONLY  0x00000010
#define CREDUIWIN_IN_CRED_ONLY  0x00000020
#define CREDUIWIN_ENUMERATE_ADMINS  0x00000100
#define CREDUIWIN_ENUMERATE_CURRENT_USER  0x00000200
#define CREDUIWIN_SECURE_PROMPT  0x00001000
#define CREDUIWIN_PREPROMPTING  0X00002000
#define CREDUIWIN_PACK_32_WOW  0x10000000

#define CREDUIWIN_VALID_FLAGS  ( \
    CREDUIWIN_GENERIC | \
    CREDUIWIN_CHECKBOX | \
    CREDUIWIN_AUTHPACKAGE_ONLY | \
    CREDUIWIN_IN_CRED_ONLY | \
    CREDUIWIN_ENUMERATE_ADMINS | \
    CREDUIWIN_ENUMERATE_CURRENT_USER | \
    CREDUIWIN_SECURE_PROMPT | \
    CREDUIWIN_PREPROMPTING | \
    CREDUIWIN_PACK_32_WOW \
)

#define CRED_PRESERVE_CREDENTIAL_BLOB  0x1

#define CRED_ENUMERATE_ALL_CREDENTIALS  0x1

#define CRED_CACHE_TARGET_INFORMATION  0x1

#define CRED_ALLOW_NAME_RESOLUTION 0x1

#ifndef __SECHANDLE_DEFINED__
typedef struct _SecHandle {
    ULONG_PTR dwLower;
    ULONG_PTR dwUpper;
} SecHandle, *PSecHandle;
#define __SECHANDLE_DEFINED__
#endif

typedef PSecHandle PCtxtHandle;

#ifndef _WINBASE_H
#ifndef _FILETIME_
#define _FILETIME_
typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;
#endif /* _FILETIME_ */
#endif /* _WINBASE_H */

#ifndef _NTDEF_
typedef LONG NTSTATUS, *PNTSTATUS;
#endif

typedef struct _CREDENTIAL_ATTRIBUTEA {
    LPSTR Keyword;
    DWORD Flags;
    DWORD ValueSize;
    LPBYTE Value;
} CREDENTIAL_ATTRIBUTEA, *PCREDENTIAL_ATTRIBUTEA;

typedef struct _CREDENTIAL_ATTRIBUTEW {
    LPWSTR  Keyword;
    DWORD Flags;
    DWORD ValueSize;
    LPBYTE Value;
} CREDENTIAL_ATTRIBUTEW, *PCREDENTIAL_ATTRIBUTEW;

typedef struct _CREDENTIALA {
    DWORD Flags;
    DWORD Type;
    LPSTR TargetName;
    LPSTR Comment;
    FILETIME LastWritten;
    DWORD CredentialBlobSize;
    LPBYTE CredentialBlob;
    DWORD Persist;
    DWORD AttributeCount;
    PCREDENTIAL_ATTRIBUTEA Attributes;
    LPSTR TargetAlias;
    LPSTR UserName;
} CREDENTIALA, *PCREDENTIALA;

typedef struct _CREDENTIALW {
    DWORD Flags;
    DWORD Type;
    LPWSTR TargetName;
    LPWSTR Comment;
    FILETIME LastWritten;
    DWORD CredentialBlobSize;
    LPBYTE CredentialBlob;
    DWORD Persist;
    DWORD AttributeCount;
    PCREDENTIAL_ATTRIBUTEW Attributes;
    LPWSTR TargetAlias;
    LPWSTR UserName;
} CREDENTIALW, *PCREDENTIALW;

typedef struct _CREDENTIAL_TARGET_INFORMATIONA {
    LPSTR TargetName;
    LPSTR NetbiosServerName;
    LPSTR DnsServerName;
    LPSTR NetbiosDomainName;
    LPSTR DnsDomainName;
    LPSTR DnsTreeName;
    LPSTR PackageName;
    ULONG Flags;
    DWORD CredTypeCount;
    LPDWORD CredTypes;
} CREDENTIAL_TARGET_INFORMATIONA, *PCREDENTIAL_TARGET_INFORMATIONA;

typedef struct _CREDENTIAL_TARGET_INFORMATIONW {
    LPWSTR TargetName;
    LPWSTR NetbiosServerName;
    LPWSTR DnsServerName;
    LPWSTR NetbiosDomainName;
    LPWSTR DnsDomainName;
    LPWSTR DnsTreeName;
    LPWSTR PackageName;
    ULONG Flags;
    DWORD CredTypeCount;
    LPDWORD CredTypes;
} CREDENTIAL_TARGET_INFORMATIONW, *PCREDENTIAL_TARGET_INFORMATIONW;

typedef struct _CERT_CREDENTIAL_INFO {
    ULONG cbSize;
    UCHAR rgbHashOfCert[CERT_HASH_LENGTH];
} CERT_CREDENTIAL_INFO, *PCERT_CREDENTIAL_INFO;

typedef struct _USERNAME_TARGET_CREDENTIAL_INFO {
    LPWSTR UserName;
} USERNAME_TARGET_CREDENTIAL_INFO, *PUSERNAME_TARGET_CREDENTIAL_INFO;

typedef struct _BINARY_BLOB_CREDENTIAL_INFO {
    ULONG cbBlob;
    LPBYTE pbBlob;
} BINARY_BLOB_CREDENTIAL_INFO, *PBINARY_BLOB_CREDENTIAL_INFO;

typedef enum _CRED_MARSHAL_TYPE {
    CertCredential = 1,
    UsernameTargetCredential,
    BinaryBlobCredential,
} CRED_MARSHAL_TYPE, *PCRED_MARSHAL_TYPE;

typedef enum _CRED_PROTECTION_TYPE {
    CredUnprotected,
    CredUserProtection,
    CredTrustedProtection,
} CRED_PROTECTION_TYPE, *PCRED_PROTECTION_TYPE;

typedef struct _CREDUI_INFOA {
    DWORD cbSize;
    HWND hwndParent;
    PCSTR pszMessageText;
    PCSTR pszCaptionText;
    HBITMAP hbmBanner;
} CREDUI_INFOA, *PCREDUI_INFOA;

typedef struct _CREDUI_INFOW {
    DWORD cbSize;
    HWND hwndParent;
    PCWSTR pszMessageText;
    PCWSTR pszCaptionText;
    HBITMAP hbmBanner;
} CREDUI_INFOW, *PCREDUI_INFOW;

WINADVAPI BOOL WINAPI CredWriteW(PCREDENTIALW,DWORD);
WINADVAPI BOOL WINAPI CredWriteA(PCREDENTIALA,DWORD);
WINADVAPI BOOL WINAPI CredReadW(LPCWSTR,DWORD,DWORD,PCREDENTIALW*);
WINADVAPI BOOL WINAPI CredReadA(LPCSTR,DWORD,DWORD,PCREDENTIALA*);
WINADVAPI BOOL WINAPI CredEnumerateW(LPCWSTR,DWORD,DWORD*,PCREDENTIALW**);
WINADVAPI BOOL WINAPI CredEnumerateA(LPCSTR,DWORD,DWORD*,PCREDENTIALA**);
WINADVAPI BOOL WINAPI CredWriteDomainCredentialsW(PCREDENTIAL_TARGET_INFORMATIONW,PCREDENTIALW,DWORD);
WINADVAPI BOOL WINAPI CredWriteDomainCredentialsA(PCREDENTIAL_TARGET_INFORMATIONA,PCREDENTIALA,DWORD);
WINADVAPI BOOL WINAPI CredReadDomainCredentialsW(PCREDENTIAL_TARGET_INFORMATIONW,DWORD,DWORD*,PCREDENTIALW**);
WINADVAPI BOOL WINAPI CredReadDomainCredentialsA(PCREDENTIAL_TARGET_INFORMATIONA,DWORD,DWORD*,PCREDENTIALA**);
WINADVAPI BOOL WINAPI CredDeleteW(LPCWSTR,DWORD,DWORD);
WINADVAPI BOOL WINAPI CredDeleteA(LPCSTR,DWORD,DWORD);
WINADVAPI BOOL WINAPI CredRenameW(LPCWSTR,LPCWSTR,DWORD,DWORD);
WINADVAPI BOOL WINAPI CredRenameA(LPCSTR,LPCSTR,DWORD,DWORD);
WINADVAPI BOOL WINAPI CredGetTargetInfoW(LPCWSTR,DWORD,PCREDENTIAL_TARGET_INFORMATIONW*);
WINADVAPI BOOL WINAPI CredGetTargetInfoA(LPCSTR,DWORD,PCREDENTIAL_TARGET_INFORMATIONA*);
WINADVAPI BOOL WINAPI CredMarshalCredentialW(CRED_MARSHAL_TYPE,PVOID,LPWSTR*);
WINADVAPI BOOL WINAPI CredMarshalCredentialA(CRED_MARSHAL_TYPE,PVOID,LPSTR*);
WINADVAPI BOOL WINAPI CredUnmarshalCredentialW(LPCWSTR,PCRED_MARSHAL_TYPE,PVOID*);
WINADVAPI BOOL WINAPI CredUnmarshalCredentialA(LPCSTR,PCRED_MARSHAL_TYPE,PVOID*);
WINADVAPI BOOL WINAPI CredIsMarshaledCredentialW(LPCWSTR);
WINADVAPI BOOL WINAPI CredIsMarshaledCredentialA(LPCSTR);
CREDUIAPI BOOL WINAPI CredUnPackAuthenticationBufferW(DWORD,PVOID,DWORD,LPWSTR,DWORD*,LPWSTR,DWORD*,LPWSTR,DWORD*);
CREDUIAPI BOOL WINAPI CredUnPackAuthenticationBufferA(DWORD,PVOID,DWORD,LPSTR,DWORD*,LPSTR,DWORD*,LPSTR,DWORD*);
CREDUIAPI BOOL WINAPI CredPackAuthenticationBufferW(DWORD,LPWSTR,LPWSTR,PBYTE,DWORD*);
CREDUIAPI BOOL WINAPI CredPackAuthenticationBufferA(DWORD,LPSTR,LPSTR,PBYTE,DWORD*);
WINADVAPI BOOL WINAPI CredProtectW(BOOL,LPWSTR,DWORD,LPWSTR,DWORD*,CRED_PROTECTION_TYPE*);
WINADVAPI BOOL WINAPI CredProtectA(BOOL,LPSTR,DWORD,LPSTR,DWORD*, CRED_PROTECTION_TYPE*);
WINADVAPI BOOL WINAPI CredUnprotectW(BOOL,LPWSTR,DWORD,LPWSTR,DWORD*);
WINADVAPI BOOL WINAPI CredUnprotectA(BOOL,LPSTR,DWORD,LPSTR,DWORD*);
WINADVAPI BOOL WINAPI CredIsProtectedW(LPWSTR,CRED_PROTECTION_TYPE*);
WINADVAPI BOOL WINAPI CredIsProtectedA(LPSTR,CRED_PROTECTION_TYPE*);
WINADVAPI BOOL WINAPI CredFindBestCredentialW(LPCWSTR,DWORD,DWORD,PCREDENTIALW*);
WINADVAPI BOOL WINAPI CredFindBestCredentialA(LPCSTR,DWORD,DWORD,PCREDENTIALA*);
WINADVAPI BOOL WINAPI CredGetSessionTypes(DWORD,LPDWORD);
WINADVAPI VOID WINAPI CredFree(PVOID);

CREDUIAPI DWORD WINAPI CredUIPromptForCredentialsW(PCREDUI_INFOW,PCWSTR,PCtxtHandle,DWORD,PWSTR,ULONG,PWSTR,ULONG,BOOL*,DWORD);
CREDUIAPI DWORD WINAPI CredUIPromptForCredentialsA(PCREDUI_INFOA,PCSTR,PCtxtHandle,DWORD,PSTR,ULONG,PSTR,ULONG,BOOL*,DWORD);
CREDUIAPI DWORD WINAPI CredUIPromptForWindowsCredentialsW(PCREDUI_INFOW,DWORD,ULONG*,LPCVOID,ULONG,LPVOID*,ULONG*,BOOL*,DWORD);
CREDUIAPI DWORD WINAPI CredUIPromptForWindowsCredentialsA(PCREDUI_INFOA,DWORD,ULONG*,LPCVOID,ULONG,LPVOID*,ULONG*,BOOL*,DWORD);
CREDUIAPI DWORD WINAPI CredUIParseUserNameW(PCWSTR,PWSTR,ULONG,PWSTR,ULONG);
CREDUIAPI DWORD WINAPI CredUIParseUserNameA(PCSTR,PSTR,ULONG,PSTR,ULONG);
CREDUIAPI DWORD WINAPI CredUICmdLinePromptForCredentialsW(PCWSTR,PCtxtHandle,DWORD,PWSTR,ULONG,PWSTR,ULONG,PBOOL,DWORD);
CREDUIAPI DWORD WINAPI CredUICmdLinePromptForCredentialsA(PCSTR,PCtxtHandle,DWORD,PSTR,ULONG,PSTR,ULONG,PBOOL,DWORD);
CREDUIAPI DWORD WINAPI CredUIConfirmCredentialsW(PCWSTR,BOOL);
CREDUIAPI DWORD WINAPI CredUIConfirmCredentialsA(PCSTR,BOOL);
CREDUIAPI DWORD WINAPI CredUIStoreSSOCredW(PCWSTR,PCWSTR,PCWSTR,BOOL);
CREDUIAPI DWORD WINAPI CredUIReadSSOCredW(PCWSTR,PWSTR*);

#ifdef UNICODE
typedef CREDENTIAL_ATTRIBUTEW CREDENTIAL_ATTRIBUTE;
typedef PCREDENTIAL_ATTRIBUTEW PCREDENTIAL_ATTRIBUTE;
typedef CREDENTIALW CREDENTIAL;
typedef PCREDENTIALW PCREDENTIAL;
typedef CREDENTIAL_TARGET_INFORMATIONW CREDENTIAL_TARGET_INFORMATION;
typedef PCREDENTIAL_TARGET_INFORMATIONW PCREDENTIAL_TARGET_INFORMATION;
typedef CREDUI_INFOW CREDUI_INFO;
typedef PCREDUI_INFOW PCREDUI_INFO;
#define CRED_SESSION_WILDCARD_NAME  CRED_SESSION_WILDCARD_NAME_W
#define CRED_TARGETNAME_DOMAIN_NAMESPACE  CRED_TARGETNAME_DOMAIN_NAMESPACE_W
#define CRED_UNIVERSAL_WILDCARD  CRED_UNIVERSAL_WILDCARD_W
#define CRED_TARGETNAME_NAMESPACE_SEPERATOR  CRED_TARGETNAME_NAMESPACE_SEPERATOR_W
#define CRED_TARGETNAME_ATTRIBUTE_SEPERATOR  CRED_TARGETNAME_ATTRIBUTE_SEPERATOR_W
#define CRED_TARGETNAME_ATTRIBUTE_NAME  CRED_TARGETNAME_ATTRIBUTE_NAME_W
#define CRED_TARGETNAME_ATTRIBUTE_TARGET  CRED_TARGETNAME_ATTRIBUTE_TARGET_W
#define CRED_TARGETNAME_ATTRIBUTE_BATCH  CRED_TARGETNAME_ATTRIBUTE_BATCH_W
#define CRED_TARGETNAME_ATTRIBUTE_INTERACTIVE  CRED_TARGETNAME_ATTRIBUTE_INTERACTIVE_W
#define CRED_TARGETNAME_ATTRIBUTE_SERVICE  CRED_TARGETNAME_ATTRIBUTE_SERVICE_W
#define CRED_TARGETNAME_ATTRIBUTE_NETWORK  CRED_TARGETNAME_ATTRIBUTE_NETWORK_W
#define CRED_TARGETNAME_ATTRIBUTE_NETWORKCLEARTEXT  CRED_TARGETNAME_ATTRIBUTE_NETWORKCLEARTEXT_W
#define CRED_TARGETNAME_ATTRIBUTE_REMOTEINTERACTIVE  CRED_TARGETNAME_ATTRIBUTE_REMOTEINTERACTIVE_W
#define CRED_TARGETNAME_ATTRIBUTE_CACHEDINTERACTIVE  CRED_TARGETNAME_ATTRIBUTE_CACHEDINTERACTIVE_W
#define CredWrite CredWriteW
#define CredRead CredReadW
#define CredEnumerate CredEnumerateW
#define CredWriteDomainCredentials CredWriteDomainCredentialsW
#define CredReadDomainCredentials CredReadDomainCredentialsW
#define CredDelete CredDeleteW
#define CredRename CredRenameW
#define CredGetTargetInfo CredGetTargetInfoW
#define CredMarshalCredential CredMarshalCredentialW
#define CredUnmarshalCredential CredUnmarshalCredentialW
#define CredIsMarshaledCredential CredIsMarshaledCredentialW
#define CredUnPackAuthenticationBuffer CredUnPackAuthenticationBufferW
#define CredPackAuthenticationBuffer CredPackAuthenticationBufferW
#define CredProtect CredProtectW
#define CredUnprotect CredUnprotectW
#define CredIsProtected CredIsProtectedW
#define CredFindBestCredential CredFindBestCredentialW
#define CredUIPromptForCredentials CredUIPromptForCredentialsW
#define CredUIPromptForWindowsCredentials CredUIPromptForWindowsCredentialsW
#define CredUIParseUserName CredUIParseUserNameW
#define CredUICmdLinePromptForCredentials CredUICmdLinePromptForCredentialsW
#define CredUIConfirmCredentials CredUIConfirmCredentialsW
#else
typedef CREDENTIAL_ATTRIBUTEA CREDENTIAL_ATTRIBUTE;
typedef PCREDENTIAL_ATTRIBUTEA PCREDENTIAL_ATTRIBUTE;
typedef CREDENTIALA CREDENTIAL;
typedef PCREDENTIALA PCREDENTIAL;
typedef CREDENTIAL_TARGET_INFORMATIONA CREDENTIAL_TARGET_INFORMATION;
typedef PCREDENTIAL_TARGET_INFORMATIONA PCREDENTIAL_TARGET_INFORMATION;
typedef CREDUI_INFOA CREDUI_INFO;
typedef PCREDUI_INFOA PCREDUI_INFO;
#define CRED_SESSION_WILDCARD_NAME  CRED_SESSION_WILDCARD_NAME_A
#define CRED_TARGETNAME_DOMAIN_NAMESPACE  CRED_TARGETNAME_DOMAIN_NAMESPACE_A
#define CRED_UNIVERSAL_WILDCARD  CRED_UNIVERSAL_WILDCARD_A
#define CRED_TARGETNAME_NAMESPACE_SEPERATOR  CRED_TARGETNAME_NAMESPACE_SEPERATOR_A
#define CRED_TARGETNAME_ATTRIBUTE_SEPERATOR  CRED_TARGETNAME_ATTRIBUTE_SEPERATOR_A
#define CRED_TARGETNAME_ATTRIBUTE_NAME  CRED_TARGETNAME_ATTRIBUTE_NAME_A
#define CRED_TARGETNAME_ATTRIBUTE_TARGET  CRED_TARGETNAME_ATTRIBUTE_TARGET_A
#define CRED_TARGETNAME_ATTRIBUTE_BATCH  CRED_TARGETNAME_ATTRIBUTE_BATCH_A
#define CRED_TARGETNAME_ATTRIBUTE_INTERACTIVE  CRED_TARGETNAME_ATTRIBUTE_INTERACTIVE_A
#define CRED_TARGETNAME_ATTRIBUTE_SERVICE  CRED_TARGETNAME_ATTRIBUTE_SERVICE_A
#define CRED_TARGETNAME_ATTRIBUTE_NETWORK  CRED_TARGETNAME_ATTRIBUTE_NETWORK_A
#define CRED_TARGETNAME_ATTRIBUTE_NETWORKCLEARTEXT  CRED_TARGETNAME_ATTRIBUTE_NETWORKCLEARTEXT_A
#define CRED_TARGETNAME_ATTRIBUTE_REMOTEINTERACTIVE  CRED_TARGETNAME_ATTRIBUTE_REMOTEINTERACTIVE_A
#define CRED_TARGETNAME_ATTRIBUTE_CACHEDINTERACTIVE  CRED_TARGETNAME_ATTRIBUTE_CACHEDINTERACTIVE_A
#define CredWrite CredWriteA
#define CredRead CredReadA
#define CredEnumerate CredEnumerateA
#define CredWriteDomainCredentials CredWriteDomainCredentialsA
#define CredReadDomainCredentials CredReadDomainCredentialsA
#define CredDelete CredDeleteA
#define CredRename CredRenameA
#define CredGetTargetInfo CredGetTargetInfoA
#define CredMarshalCredential CredMarshalCredentialA
#define CredUnmarshalCredential CredUnmarshalCredentialA
#define CredIsMarshaledCredential CredIsMarshaledCredentialA
#define CredUnPackAuthenticationBuffer CredUnPackAuthenticationBufferA
#define CredPackAuthenticationBuffer CredPackAuthenticationBufferA
#define CredProtect CredProtectA
#define CredUnprotect CredUnprotectA
#define CredIsProtected CredIsProtectedA
#define CredFindBestCredential CredFindBestCredentialA
#define CredUIPromptForCredentials CredUIPromptForCredentialsA
#define CredUIPromptForWindowsCredentials CredUIPromptForWindowsCredentialsA
#define CredUIParseUserName CredUIParseUserNameA
#define CredUICmdLinePromptForCredentials CredUICmdLinePromptForCredentialsA
#define CredUIConfirmCredentials CredUIConfirmCredentialsA
#endif /* UNICODE */

#ifdef __cplusplus
}
#endif

#endif /* _WINCRED_H */
