#ifndef _SUBAUTH_H
#define _SUBAUTH_H

#ifdef __ORANGEC__ 
#pragma once
#endif

/* Subauthentication Packages definitions */

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_NTDEF_) && !defined(_NTSECAPI_)
typedef LONG NTSTATUS, *PNTSTATUS;

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _STRING {
    USHORT Length;
    USHORT MaximumLength;
    PCHAR Buffer;
} STRING, *PSTRING;
#endif

#ifndef _NTDEF_
typedef struct _OLD_LARGE_INTEGER {
    ULONG LowPart;
    LONG HighPart;
} OLD_LARGE_INTEGER, *POLD_LARGE_INTEGER;
#define NT_SUCCESS(Status)  ((NTSTATUS)(Status) >= 0)
#endif /* _NTDEF_ */

#ifndef _NTSAM_SAM_HANDLE_
typedef PVOID SAM_HANDLE, *PSAM_HANDLE;
#define _NTSAM_SAM_HANDLE_
#endif

#ifndef _NTSAM_USER_ACCOUNT_FLAGS_
#define USER_ACCOUNT_DISABLED  (0x00000001)
#define USER_HOME_DIRECTORY_REQUIRED  (0x00000002)
#define USER_PASSWORD_NOT_REQUIRED  (0x00000004)
#define USER_TEMP_DUPLICATE_ACCOUNT  (0x00000008)
#define USER_NORMAL_ACCOUNT  (0x00000010)
#define USER_MNS_LOGON_ACCOUNT  (0x00000020)
#define USER_INTERDOMAIN_TRUST_ACCOUNT  (0x00000040)
#define USER_WORKSTATION_TRUST_ACCOUNT  (0x00000080)
#define USER_SERVER_TRUST_ACCOUNT  (0x00000100)
#define USER_DONT_EXPIRE_PASSWORD  (0x00000200)
#define USER_ACCOUNT_AUTO_LOCKED  (0x00000400)
#define USER_ENCRYPTED_TEXT_PASSWORD_ALLOWED (0x00000800)
#define USER_SMARTCARD_REQUIRED  (0x00001000)
#define USER_TRUSTED_FOR_DELEGATION  (0x00002000)
#define USER_NOT_DELEGATED  (0x00004000)
#define USER_USE_DES_KEY_ONLY  (0x00008000)
#define USER_DONT_REQUIRE_PREAUTH  (0x00010000)
#define NEXT_FREE_ACCOUNT_CONTROL_BIT (USER_DONT_REQUIRE_PREAUTH << 1)

#define USER_MACHINE_ACCOUNT_MASK  (USER_INTERDOMAIN_TRUST_ACCOUNT|USER_WORKSTATION_TRUST_ACCOUNT|USER_SERVER_TRUST_ACCOUNT)
#define USER_ACCOUNT_TYPE_MASK  (USER_TEMP_DUPLICATE_ACCOUNT|USER_NORMAL_ACCOUNT|USER_MACHINE_ACCOUNT_MASK)

#define SAM_DAYS_PER_WEEK  (7)
#define SAM_HOURS_PER_WEEK  (24 * SAM_DAYS_PER_WEEK)
#define SAM_MINUTES_PER_WEEK (60 * SAM_HOURS_PER_WEEK)

typedef struct _LOGON_HOURS {
    USHORT UnitsPerWeek;
    PUCHAR LogonHours;
} LOGON_HOURS, *PLOGON_HOURS;

typedef struct _SR_SECURITY_DESCRIPTOR {
    ULONG Length;
    PUCHAR SecurityDescriptor;
} SR_SECURITY_DESCRIPTOR, *PSR_SECURITY_DESCRIPTOR;

#define _NTSAM_USER_ACCOUNT_FLAG_
#endif /* _NTSAM_USER_ACCOUNT_FLAGS_ */

#ifndef _NTSAM_USER_ALL_INFO_
#include "pshpack4.h"
typedef struct _USER_ALL_INFORMATION {
    LARGE_INTEGER LastLogon;
    LARGE_INTEGER LastLogoff;
    LARGE_INTEGER PasswordLastSet;
    LARGE_INTEGER AccountExpires;
    LARGE_INTEGER PasswordCanChange;
    LARGE_INTEGER PasswordMustChange;
    UNICODE_STRING UserName;
    UNICODE_STRING FullName;
    UNICODE_STRING HomeDirectory;
    UNICODE_STRING HomeDirectoryDrive;
    UNICODE_STRING ScriptPath;
    UNICODE_STRING ProfilePath;
    UNICODE_STRING AdminComment;
    UNICODE_STRING WorkStations;
    UNICODE_STRING UserComment;
    UNICODE_STRING Parameters;
    UNICODE_STRING LmPassword;
    UNICODE_STRING NtPassword;
    UNICODE_STRING PrivateData;
    SR_SECURITY_DESCRIPTOR SecurityDescriptor;
    ULONG UserId;
    ULONG PrimaryGroupId;
    ULONG UserAccountControl;
    ULONG WhichFields;
    LOGON_HOURS LogonHours;
    USHORT BadPasswordCount;
    USHORT LogonCount;
    USHORT CountryCode;
    USHORT CodePage;
    BOOLEAN LmPasswordPresent;
    BOOLEAN NtPasswordPresent;
    BOOLEAN PasswordExpired;
    BOOLEAN PrivateDataSensitive;
} USER_ALL_INFORMATION, *PUSER_ALL_INFORMATION;
#include "poppack.h"
#define _NTSAM_USER_ALL_INFO_
#endif /* _NTSAM_USER_ALL_INFO_ */

#ifndef _NTSAM_SAM_USER_PARMS_
#define USER_ALL_PARAMETERS  0x00200000
#define _NTSAM_SAM_USER_PARMS_
#endif

#define CLEAR_BLOCK_LENGTH  8
#define CYPHER_BLOCK_LENGTH  8
#define USER_SESSION_KEY_LENGTH  (CYPHER_BLOCK_LENGTH * 2)

#define MSV1_0_PASSTHRU  0x01
#define MSV1_0_GUEST_LOGON  0x02

#define MSV1_0_VALIDATION_LOGOFF_TIME  0x00000001
#define MSV1_0_VALIDATION_KICKOFF_TIME  0x00000002
#define MSV1_0_VALIDATION_LOGON_SERVER  0x00000004
#define MSV1_0_VALIDATION_LOGON_DOMAIN  0x00000008
#define MSV1_0_VALIDATION_SESSION_KEY  0x00000010
#define MSV1_0_VALIDATION_USER_FLAGS  0x00000020
#define MSV1_0_VALIDATION_USER_ID  0x00000040

#define MSV1_0_SUBAUTH_ACCOUNT_DISABLED  0x00000001
#define MSV1_0_SUBAUTH_PASSWORD  0x00000002
#define MSV1_0_SUBAUTH_WORKSTATIONS  0x00000004
#define MSV1_0_SUBAUTH_LOGON_HOURS  0x00000008
#define MSV1_0_SUBAUTH_ACCOUNT_EXPIRY  0x00000010
#define MSV1_0_SUBAUTH_PASSWORD_EXPIRY  0x00000020
#define MSV1_0_SUBAUTH_ACCOUNT_TYPE  0x00000040
#define MSV1_0_SUBAUTH_LOCKOUT  0x00000080

#define STATUS_SUCCESS  ((NTSTATUS)0x00000000L)
#define STATUS_INVALID_INFO_CLASS  ((NTSTATUS)0xC0000003L)
#define STATUS_NO_SUCH_USER  ((NTSTATUS)0xC0000064L)
#define STATUS_WRONG_PASSWORD  ((NTSTATUS)0xC000006AL)
#define STATUS_PASSWORD_RESTRICTION  ((NTSTATUS)0xC000006CL)
#define STATUS_LOGON_FAILURE  ((NTSTATUS)0xC000006DL)
#define STATUS_ACCOUNT_RESTRICTION  ((NTSTATUS)0xC000006EL)
#define STATUS_INVALID_LOGON_HOURS  ((NTSTATUS)0xC000006FL)
#define STATUS_INVALID_WORKSTATION  ((NTSTATUS)0xC0000070L)
#define STATUS_PASSWORD_EXPIRED  ((NTSTATUS)0xC0000071L)
#define STATUS_ACCOUNT_DISABLED  ((NTSTATUS)0xC0000072L)
#define STATUS_INSUFFICIENT_RESOURCES  ((NTSTATUS)0xC000009AL)
#define STATUS_ACCOUNT_EXPIRED  ((NTSTATUS)0xC0000193L)
#define STATUS_PASSWORD_MUST_CHANGE  ((NTSTATUS)0xC0000224L)
#define STATUS_ACCOUNT_LOCKED_OUT  ((NTSTATUS)0xC0000234L)

typedef struct _CLEAR_BLOCK {
    CHAR data[CLEAR_BLOCK_LENGTH];
} CLEAR_BLOCK;
typedef CLEAR_BLOCK *PCLEAR_BLOCK;

typedef struct _CYPHER_BLOCK {
    CHAR data[CYPHER_BLOCK_LENGTH];
} CYPHER_BLOCK;

typedef CYPHER_BLOCK *PCYPHER_BLOCK;

typedef struct _LM_OWF_PASSWORD {
    CYPHER_BLOCK data[2];
} LM_OWF_PASSWORD;

typedef LM_OWF_PASSWORD *PLM_OWF_PASSWORD;
typedef CLEAR_BLOCK LM_CHALLENGE;
typedef LM_CHALLENGE *PLM_CHALLENGE;
typedef LM_OWF_PASSWORD NT_OWF_PASSWORD;
typedef NT_OWF_PASSWORD *PNT_OWF_PASSWORD;
typedef LM_CHALLENGE NT_CHALLENGE;
typedef NT_CHALLENGE *PNT_CHALLENGE;

typedef struct _USER_SESSION_KEY {
    CYPHER_BLOCK data[2];
} USER_SESSION_KEY;
typedef USER_SESSION_KEY *PUSER_SESSION_KEY;

typedef enum _NETLOGON_LOGON_INFO_CLASS {
    NetlogonInteractiveInformation = 1,
    NetlogonNetworkInformation,
    NetlogonServiceInformation,
    NetlogonGenericInformation,
    NetlogonInteractiveTransitiveInformation,
    NetlogonNetworkTransitiveInformation,
    NetlogonServiceTransitiveInformation
} NETLOGON_LOGON_INFO_CLASS;

typedef struct _NETLOGON_LOGON_IDENTITY_INFO {
    UNICODE_STRING LogonDomainName;
    ULONG ParameterControl;
    OLD_LARGE_INTEGER LogonId;
    UNICODE_STRING UserName;
    UNICODE_STRING Workstation;
} NETLOGON_LOGON_IDENTITY_INFO, *PNETLOGON_LOGON_IDENTITY_INFO;

typedef struct _NETLOGON_INTERACTIVE_INFO {
    NETLOGON_LOGON_IDENTITY_INFO Identity;
    LM_OWF_PASSWORD LmOwfPassword;
    NT_OWF_PASSWORD NtOwfPassword;
} NETLOGON_INTERACTIVE_INFO, *PNETLOGON_INTERACTIVE_INFO;

typedef struct _NETLOGON_SERVICE_INFO {
    NETLOGON_LOGON_IDENTITY_INFO Identity;
    LM_OWF_PASSWORD LmOwfPassword;
    NT_OWF_PASSWORD NtOwfPassword;
} NETLOGON_SERVICE_INFO, *PNETLOGON_SERVICE_INFO;

typedef struct _NETLOGON_NETWORK_INFO {
    NETLOGON_LOGON_IDENTITY_INFO Identity;
    LM_CHALLENGE LmChallenge;
    STRING NtChallengeResponse;
    STRING LmChallengeResponse;
} NETLOGON_NETWORK_INFO, *PNETLOGON_NETWORK_INFO;

typedef struct _NETLOGON_GENERIC_INFO {
    NETLOGON_LOGON_IDENTITY_INFO Identity;
    UNICODE_STRING PackageName;
    ULONG DataLength;
    PUCHAR LogonData;
} NETLOGON_GENERIC_INFO, *PNETLOGON_GENERIC_INFO;

typedef struct _MSV1_0_VALIDATION_INFO {
    LARGE_INTEGER LogoffTime;
    LARGE_INTEGER KickoffTime;
    UNICODE_STRING LogonServer;
    UNICODE_STRING LogonDomainName;
    USER_SESSION_KEY SessionKey;
    BOOLEAN Authoritative;
    ULONG UserFlags;
    ULONG WhichFields;
    ULONG UserId;
} MSV1_0_VALIDATION_INFO, *PMSV1_0_VALIDATION_INFO;

NTSTATUS NTAPI Msv1_0SubAuthenticationRoutine(NETLOGON_LOGON_INFO_CLASS,PVOID,ULONG,PUSER_ALL_INFORMATION,PULONG,PULONG,PBOOLEAN,PLARGE_INTEGER,PLARGE_INTEGER);
NTSTATUS NTAPI Msv1_0SubAuthenticationRoutineEx(NETLOGON_LOGON_INFO_CLASS,PVOID,ULONG,PUSER_ALL_INFORMATION,SAM_HANDLE,PMSV1_0_VALIDATION_INFO,PULONG);
NTSTATUS NTAPI Msv1_0SubAuthenticationRoutineGeneric(PVOID,ULONG,PULONG,PVOID*);
NTSTATUS NTAPI Msv1_0SubAuthenticationFilter(NETLOGON_LOGON_INFO_CLASS,PVOID,ULONG,PUSER_ALL_INFORMATION,PULONG,PULONG,PBOOLEAN,PLARGE_INTEGER,PLARGE_INTEGER);

#ifdef __cplusplus
}
#endif

#endif /* _SUBAUTH_H */
