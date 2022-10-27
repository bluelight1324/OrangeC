#ifndef _AUTHZ_H
#define _AUTHZ_H

#ifdef __ORANGEC__ 
#pragma once
#endif

/* Authorization framework API definitions */

#ifdef __cplusplus
extern "C" {
#endif

#if __POCC__ >= 290
#pragma warn(push)
#pragma warn(disable:2198)  /* Nameless field is not standard */
#endif

#if !defined(_AUTHZ_)
#define AUTHZAPI DECLSPEC_IMPORT
#else
#define AUTHZAPI
#endif

#include <windows.h>
#include <adtgen.h>

#define AUTHZ_SKIP_TOKEN_GROUPS  0x2
#define AUTHZ_REQUIRE_S4U_LOGON  0x4
#define AUTHZ_COMPUTE_PRIVILEGES  0x8

#define AUTHZ_GENERATE_SUCCESS_AUDIT  0x1
#define AUTHZ_GENERATE_FAILURE_AUDIT  0x2

#define AUTHZ_SECURITY_ATTRIBUTE_TYPE_INVALID  0x00
#define AUTHZ_SECURITY_ATTRIBUTE_TYPE_INT64  0x01
#define AUTHZ_SECURITY_ATTRIBUTE_TYPE_UINT64  0x02
#define AUTHZ_SECURITY_ATTRIBUTE_TYPE_STRING  0x03
#define AUTHZ_SECURITY_ATTRIBUTE_TYPE_FQBN  0x04
#if(_WIN32_WINNT >= _WIN32_WINNT_WIN8)
#define AUTHZ_SECURITY_ATTRIBUTE_TYPE_SID  0x05
#define AUTHZ_SECURITY_ATTRIBUTE_TYPE_BOOLEAN  0x06
#endif /* _WIN32_WINNT >= _WIN32_WINNT_WIN8 */
#define AUTHZ_SECURITY_ATTRIBUTE_TYPE_OCTET_STRING  0x10

#define AUTHZ_SECURITY_ATTRIBUTE_NON_INHERITABLE  0x0001
#define AUTHZ_SECURITY_ATTRIBUTE_VALUE_CASE_SENSITIVE  0x0002
#define AUTHZ_SECURITY_ATTRIBUTE_VALID_FLAGS  (AUTHZ_SECURITY_ATTRIBUTE_NON_INHERITABLE|AUTHZ_SECURITY_ATTRIBUTE_VALUE_CASE_SENSITIVE)

#define AUTHZ_SECURITY_ATTRIBUTES_INFORMATION_VERSION_V1  1
#define AUTHZ_SECURITY_ATTRIBUTES_INFORMATION_VERSION  AUTHZ_SECURITY_ATTRIBUTES_INFORMATION_VERSION_V1

#define AUTHZ_ACCESS_CHECK_NO_DEEP_COPY_SD  0x00000001

#define AUTHZ_RM_FLAG_NO_AUDIT  0x1
#define AUTHZ_RM_FLAG_INITIALIZE_UNDER_IMPERSONATION  0x2
#define AUTHZ_VALID_RM_INIT_FLAGS  (AUTHZ_RM_FLAG_NO_AUDIT|AUTHZ_RM_FLAG_INITIALIZE_UNDER_IMPERSONATION)

#define AUTHZ_NO_SUCCESS_AUDIT  0x00000001
#define AUTHZ_NO_FAILURE_AUDIT  0x00000002
#define AUTHZ_NO_ALLOC_STRINGS  0x00000004

#define AUTHZ_VALID_OBJECT_ACCESS_AUDIT_FLAGS  (AUTHZ_NO_SUCCESS_AUDIT|AUTHZ_NO_FAILURE_AUDIT|AUTHZ_NO_ALLOC_STRINGS)

#define AUTHZ_FLAG_ALLOW_MULTIPLE_SOURCE_INSTANCES  0x1

DECLARE_HANDLE(AUTHZ_ACCESS_CHECK_RESULTS_HANDLE);
DECLARE_HANDLE(AUTHZ_CLIENT_CONTEXT_HANDLE);
DECLARE_HANDLE(AUTHZ_RESOURCE_MANAGER_HANDLE);
DECLARE_HANDLE(AUTHZ_AUDIT_EVENT_HANDLE);
DECLARE_HANDLE(AUTHZ_AUDIT_EVENT_TYPE_HANDLE);
DECLARE_HANDLE(AUTHZ_SECURITY_EVENT_PROVIDER_HANDLE);

#if(_WIN32_WINNT >= _WIN32_WINNT_WIN8)
DECLARE_HANDLE(AUTHZ_CAP_CHANGE_SUBSCRIPTION_HANDLE);
#endif /* _WIN32_WINNT >= _WIN32_WINNT_WIN8 */

typedef AUTHZ_ACCESS_CHECK_RESULTS_HANDLE *PAUTHZ_ACCESS_CHECK_RESULTS_HANDLE;
typedef AUTHZ_CLIENT_CONTEXT_HANDLE *PAUTHZ_CLIENT_CONTEXT_HANDLE;
typedef AUTHZ_RESOURCE_MANAGER_HANDLE *PAUTHZ_RESOURCE_MANAGER_HANDLE;
typedef AUTHZ_AUDIT_EVENT_HANDLE *PAUTHZ_AUDIT_EVENT_HANDLE;
typedef AUTHZ_AUDIT_EVENT_TYPE_HANDLE *PAUTHZ_AUDIT_EVENT_TYPE_HANDLE;
typedef AUTHZ_SECURITY_EVENT_PROVIDER_HANDLE *PAUTHZ_SECURITY_EVENT_PROVIDER_HANDLE;

#if(_WIN32_WINNT >= _WIN32_WINNT_WIN8)
typedef AUTHZ_CAP_CHANGE_SUBSCRIPTION_HANDLE *PAUTHZ_CAP_CHANGE_SUBSCRIPTION_HANDLE;
#endif /* _WIN32_WINNT >= _WIN32_WINNT_WIN8 */

typedef struct _AUTHZ_ACCESS_REQUEST {
    ACCESS_MASK DesiredAccess;
    PSID PrincipalSelfSid;
    POBJECT_TYPE_LIST ObjectTypeList;
    DWORD ObjectTypeListLength;
    PVOID OptionalArguments;
} AUTHZ_ACCESS_REQUEST, *PAUTHZ_ACCESS_REQUEST;

typedef struct _AUTHZ_ACCESS_REPLY {
    DWORD ResultListLength;
    PACCESS_MASK GrantedAccessMask;
    PDWORD SaclEvaluationResults;
    PDWORD Error;
} AUTHZ_ACCESS_REPLY, *PAUTHZ_ACCESS_REPLY;

typedef BOOL(CALLBACK *PFN_AUTHZ_DYNAMIC_ACCESS_CHECK)(AUTHZ_CLIENT_CONTEXT_HANDLE,PACE_HEADER,PVOID,PBOOL);
typedef BOOL(CALLBACK *PFN_AUTHZ_COMPUTE_DYNAMIC_GROUPS)(AUTHZ_CLIENT_CONTEXT_HANDLE,PVOID,PSID_AND_ATTRIBUTES*,PDWORD,PSID_AND_ATTRIBUTES*,PDWORD);
typedef VOID(CALLBACK *PFN_AUTHZ_FREE_DYNAMIC_GROUPS)(PSID_AND_ATTRIBUTES);

#if(_WIN32_WINNT >= _WIN32_WINNT_WIN8)
typedef BOOL (CALLBACK *PFN_AUTHZ_GET_CENTRAL_ACCESS_POLICY)(AUTHZ_CLIENT_CONTEXT_HANDLE, PSID, PVOID, PBOOL, PVOID *);
typedef VOID (CALLBACK *PFN_AUTHZ_FREE_CENTRAL_ACCESS_POLICY)(PVOID);
#endif /* _WIN32_WINNT >= _WIN32_WINNT_WIN8 */

typedef struct _AUTHZ_SECURITY_ATTRIBUTE_FQBN_VALUE {
    ULONG64 Version;
    PWSTR pName;
} AUTHZ_SECURITY_ATTRIBUTE_FQBN_VALUE, *PAUTHZ_SECURITY_ATTRIBUTE_FQBN_VALUE;

typedef struct _AUTHZ_SECURITY_ATTRIBUTE_OCTET_STRING_VALUE {
    PVOID pValue;
    ULONG ValueLength;
} AUTHZ_SECURITY_ATTRIBUTE_OCTET_STRING_VALUE, *PAUTHZ_SECURITY_ATTRIBUTE_OCTET_STRING_VALUE;

typedef enum {
    AUTHZ_SECURITY_ATTRIBUTE_OPERATION_NONE = 0,
    AUTHZ_SECURITY_ATTRIBUTE_OPERATION_REPLACE_ALL,
    AUTHZ_SECURITY_ATTRIBUTE_OPERATION_ADD,
    AUTHZ_SECURITY_ATTRIBUTE_OPERATION_DELETE,
    AUTHZ_SECURITY_ATTRIBUTE_OPERATION_REPLACE
} AUTHZ_SECURITY_ATTRIBUTE_OPERATION, *PAUTHZ_SECURITY_ATTRIBUTE_OPERATION;

#if(_WIN32_WINNT >= _WIN32_WINNT_WIN8)
typedef enum {
    AUTHZ_SID_OPERATION_NONE = 0,
    AUTHZ_SID_OPERATION_REPLACE_ALL,
    AUTHZ_SID_OPERATION_ADD,
    AUTHZ_SID_OPERATION_DELETE,
    AUTHZ_SID_OPERATION_REPLACE
} AUTHZ_SID_OPERATION, *PAUTHZ_SID_OPERATION;
#endif /* _WIN32_WINNT >= _WIN32_WINNT_WIN8 */

typedef struct _AUTHZ_SECURITY_ATTRIBUTE_V1 {
    PWSTR pName;
    USHORT ValueType;
    USHORT Reserved;
    ULONG Flags;
    ULONG ValueCount;
    union {
        PLONG64 pInt64;
        PULONG64 pUint64;
        PWSTR *ppString;
        PAUTHZ_SECURITY_ATTRIBUTE_FQBN_VALUE pFqbn;
        PAUTHZ_SECURITY_ATTRIBUTE_OCTET_STRING_VALUE pOctetString;
    } Values;
} AUTHZ_SECURITY_ATTRIBUTE_V1, *PAUTHZ_SECURITY_ATTRIBUTE_V1;

typedef struct _AUTHZ_SECURITY_ATTRIBUTES_INFORMATION {
    USHORT Version;
    USHORT Reserved;
    ULONG AttributeCount;
    union {
        PAUTHZ_SECURITY_ATTRIBUTE_V1 pAttributeV1;
    } Attribute;
} AUTHZ_SECURITY_ATTRIBUTES_INFORMATION, *PAUTHZ_SECURITY_ATTRIBUTES_INFORMATION;

typedef enum _AUTHZ_CONTEXT_INFORMATION_CLASS {
    AuthzContextInfoUserSid = 1,
    AuthzContextInfoGroupsSids,
    AuthzContextInfoRestrictedSids,
    AuthzContextInfoPrivileges,
    AuthzContextInfoExpirationTime,
    AuthzContextInfoServerContext,
    AuthzContextInfoIdentifier,
    AuthzContextInfoSource,
    AuthzContextInfoAll,
    AuthzContextInfoAuthenticationId,
    AuthzContextInfoSecurityAttributes,
} AUTHZ_CONTEXT_INFORMATION_CLASS;

typedef enum _AUTHZ_AUDIT_EVENT_INFORMATION_CLASS {
    AuthzAuditEventInfoFlags = 1,
    AuthzAuditEventInfoOperationType,
    AuthzAuditEventInfoObjectType,
    AuthzAuditEventInfoObjectName,
    AuthzAuditEventInfoAdditionalInfo,
} AUTHZ_AUDIT_EVENT_INFORMATION_CLASS;

typedef struct _AUTHZ_REGISTRATION_OBJECT_TYPE_NAME_OFFSET {
    PWSTR szObjectTypeName;
    DWORD dwOffset;
} AUTHZ_REGISTRATION_OBJECT_TYPE_NAME_OFFSET, *PAUTHZ_REGISTRATION_OBJECT_TYPE_NAME_OFFSET;

typedef struct _AUTHZ_SOURCE_SCHEMA_REGISTRATION {
    DWORD dwFlags;
    PWSTR szEventSourceName;
    PWSTR szEventMessageFile;
    PWSTR szEventSourceXmlSchemaFile;
    PWSTR szEventAccessStringsFile;
    PWSTR szExecutableImagePath;
    union {
        PVOID pReserved;
        GUID* pProviderGuid;
    };
    DWORD dwObjectTypeNameCount;
    AUTHZ_REGISTRATION_OBJECT_TYPE_NAME_OFFSET ObjectTypeNames[ANYSIZE_ARRAY];
} AUTHZ_SOURCE_SCHEMA_REGISTRATION, *PAUTHZ_SOURCE_SCHEMA_REGISTRATION;

#if(_WIN32_WINNT >= _WIN32_WINNT_WIN8)

#define AUTHZ_RPC_INIT_INFO_CLIENT_VERSION_V1  1

typedef struct _AUTHZ_RPC_INIT_INFO_CLIENT {
    USHORT version;
    PWSTR ObjectUuid;
    PWSTR ProtSeq;
    PWSTR NetworkAddr;
    PWSTR Endpoint;
    PWSTR Options;
    PWSTR ServerSpn;
} AUTHZ_RPC_INIT_INFO_CLIENT , *PAUTHZ_RPC_INIT_INFO_CLIENT;

#define AUTHZ_INIT_INFO_VERSION_V1  1

typedef struct _AUTHZ_INIT_INFO {
    USHORT version;
    PCWSTR szResourceManagerName;
    PFN_AUTHZ_DYNAMIC_ACCESS_CHECK pfnDynamicAccessCheck;
    PFN_AUTHZ_COMPUTE_DYNAMIC_GROUPS pfnComputeDynamicGroups;
    PFN_AUTHZ_FREE_DYNAMIC_GROUPS pfnFreeDynamicGroups;
    PFN_AUTHZ_GET_CENTRAL_ACCESS_POLICY pfnGetCentralAccessPolicy;
    PFN_AUTHZ_FREE_CENTRAL_ACCESS_POLICY pfnFreeCentralAccessPolicy;
} AUTHZ_INIT_INFO, *PAUTHZ_INIT_INFO;

#endif /* _WIN32_WINNT >= _WIN32_WINNT_WIN8 */


AUTHZAPI BOOL WINAPI AuthzAccessCheck(DWORD,AUTHZ_CLIENT_CONTEXT_HANDLE,PAUTHZ_ACCESS_REQUEST,AUTHZ_AUDIT_EVENT_HANDLE,PSECURITY_DESCRIPTOR,PSECURITY_DESCRIPTOR*,DWORD,PAUTHZ_ACCESS_REPLY,PAUTHZ_ACCESS_CHECK_RESULTS_HANDLE);
AUTHZAPI BOOL WINAPI AuthzCachedAccessCheck(DWORD,AUTHZ_ACCESS_CHECK_RESULTS_HANDLE,PAUTHZ_ACCESS_REQUEST,AUTHZ_AUDIT_EVENT_HANDLE,PAUTHZ_ACCESS_REPLY);
AUTHZAPI BOOL WINAPI AuthzOpenObjectAudit(DWORD,AUTHZ_CLIENT_CONTEXT_HANDLE,PAUTHZ_ACCESS_REQUEST,AUTHZ_AUDIT_EVENT_HANDLE,PSECURITY_DESCRIPTOR,PSECURITY_DESCRIPTOR*,DWORD,PAUTHZ_ACCESS_REPLY);
AUTHZAPI BOOL WINAPI AuthzFreeHandle(AUTHZ_ACCESS_CHECK_RESULTS_HANDLE);
AUTHZAPI BOOL WINAPI AuthzInitializeResourceManager(DWORD,PFN_AUTHZ_DYNAMIC_ACCESS_CHECK,PFN_AUTHZ_COMPUTE_DYNAMIC_GROUPS,PFN_AUTHZ_FREE_DYNAMIC_GROUPS,PCWSTR,PAUTHZ_RESOURCE_MANAGER_HANDLE);
#if(_WIN32_WINNT >= _WIN32_WINNT_WIN8)
AUTHZAPI BOOL WINAPI AuthzInitializeResourceManagerEx(DWORD, PAUTHZ_INIT_INFO, PAUTHZ_RESOURCE_MANAGER_HANDLE);
AUTHZAPI BOOL WINAPI AuthzInitializeRemoteResourceManager(PAUTHZ_RPC_INIT_INFO_CLIENT, PAUTHZ_RESOURCE_MANAGER_HANDLE);
#endif /* _WIN32_WINNT >= _WIN32_WINNT_WIN8 */
AUTHZAPI BOOL WINAPI AuthzFreeResourceManager(AUTHZ_RESOURCE_MANAGER_HANDLE);
AUTHZAPI BOOL WINAPI AuthzInitializeContextFromToken(DWORD,HANDLE,AUTHZ_RESOURCE_MANAGER_HANDLE,PLARGE_INTEGER,LUID,PVOID,PAUTHZ_CLIENT_CONTEXT_HANDLE);
AUTHZAPI BOOL WINAPI AuthzInitializeContextFromSid(DWORD,PSID,AUTHZ_RESOURCE_MANAGER_HANDLE,PLARGE_INTEGER,LUID,PVOID,PAUTHZ_CLIENT_CONTEXT_HANDLE);
AUTHZAPI BOOL WINAPI AuthzInitializeContextFromAuthzContext(DWORD,AUTHZ_CLIENT_CONTEXT_HANDLE,PLARGE_INTEGER,LUID,PVOID,PAUTHZ_CLIENT_CONTEXT_HANDLE);
#if(_WIN32_WINNT >= _WIN32_WINNT_WIN8)
AUTHZAPI BOOL WINAPI AuthzInitializeCompoundContext(AUTHZ_CLIENT_CONTEXT_HANDLE, AUTHZ_CLIENT_CONTEXT_HANDLE, PAUTHZ_CLIENT_CONTEXT_HANDLE);
#endif /* _WIN32_WINNT >= _WIN32_WINNT_WIN8 */
AUTHZAPI BOOL WINAPI AuthzAddSidsToContext(AUTHZ_CLIENT_CONTEXT_HANDLE,PSID_AND_ATTRIBUTES,DWORD,PSID_AND_ATTRIBUTES,DWORD,PAUTHZ_CLIENT_CONTEXT_HANDLE);
#if _WIN32_WINNT >= _WIN32_WINNT_WIN7
AUTHZAPI BOOL WINAPI AuthzModifySecurityAttributes(AUTHZ_CLIENT_CONTEXT_HANDLE,PAUTHZ_SECURITY_ATTRIBUTE_OPERATION,PAUTHZ_SECURITY_ATTRIBUTES_INFORMATION);
#endif /* _WIN32_WINNT >= _WIN32_WINNT_WIN7 */
#if(_WIN32_WINNT >= _WIN32_WINNT_WIN8)
AUTHZAPI BOOL WINAPI AuthzModifyClaims(AUTHZ_CLIENT_CONTEXT_HANDLE, AUTHZ_CONTEXT_INFORMATION_CLASS, PAUTHZ_SECURITY_ATTRIBUTE_OPERATION, PAUTHZ_SECURITY_ATTRIBUTES_INFORMATION);
AUTHZAPI BOOL WINAPI AuthzModifySids(AUTHZ_CLIENT_CONTEXT_HANDLE, AUTHZ_CONTEXT_INFORMATION_CLASS, PAUTHZ_SID_OPERATION, PTOKEN_GROUPS);
AUTHZAPI BOOL WINAPI AuthzSetAppContainerInformation(AUTHZ_CLIENT_CONTEXT_HANDLE, PSID, DWORD, PSID_AND_ATTRIBUTES);
#endif /* _WIN32_WINNT >= _WIN32_WINNT_WIN8 */
AUTHZAPI BOOL WINAPI AuthzGetInformationFromContext(AUTHZ_CLIENT_CONTEXT_HANDLE,AUTHZ_CONTEXT_INFORMATION_CLASS,DWORD,PDWORD,PVOID);
AUTHZAPI BOOL WINAPI AuthzFreeContext(AUTHZ_CLIENT_CONTEXT_HANDLE);
AUTHZAPI BOOL WINAPIV AuthzInitializeObjectAccessAuditEvent(DWORD,AUTHZ_AUDIT_EVENT_TYPE_HANDLE,PWSTR,PWSTR,PWSTR,PWSTR,PAUTHZ_AUDIT_EVENT_HANDLE,DWORD,...);
AUTHZAPI BOOL WINAPIV AuthzInitializeObjectAccessAuditEvent2(DWORD,AUTHZ_AUDIT_EVENT_TYPE_HANDLE,PWSTR,PWSTR,PWSTR,PWSTR,PWSTR,PAUTHZ_AUDIT_EVENT_HANDLE,DWORD,...);
AUTHZAPI BOOL WINAPI AuthzGetInformationFromAuditEvent(AUTHZ_AUDIT_EVENT_HANDLE,AUTHZ_AUDIT_EVENT_INFORMATION_CLASS,DWORD,PDWORD,PVOID);
AUTHZAPI BOOL WINAPI AuthzFreeAuditEvent(AUTHZ_AUDIT_EVENT_HANDLE);
AUTHZAPI BOOL WINAPI AuthzEvaluateSacl(AUTHZ_CLIENT_CONTEXT_HANDLE,PAUTHZ_ACCESS_REQUEST,PACL,ACCESS_MASK,BOOL,PBOOL);
AUTHZAPI BOOL WINAPI AuthzInstallSecurityEventSource(DWORD,PAUTHZ_SOURCE_SCHEMA_REGISTRATION);
AUTHZAPI BOOL WINAPI AuthzUninstallSecurityEventSource(DWORD,PCWSTR);
AUTHZAPI BOOL WINAPI AuthzEnumerateSecurityEventSources(DWORD,PAUTHZ_SOURCE_SCHEMA_REGISTRATION,PDWORD,PDWORD);
AUTHZAPI BOOL WINAPI AuthzRegisterSecurityEventSource(DWORD,PCWSTR,PAUTHZ_SECURITY_EVENT_PROVIDER_HANDLE);
AUTHZAPI BOOL WINAPI AuthzUnregisterSecurityEventSource(DWORD,PAUTHZ_SECURITY_EVENT_PROVIDER_HANDLE);
AUTHZAPI BOOL WINAPIV AuthzReportSecurityEvent(DWORD,AUTHZ_SECURITY_EVENT_PROVIDER_HANDLE,DWORD,PSID,DWORD,...);
AUTHZAPI BOOL WINAPI AuthzReportSecurityEventFromParams(DWORD,AUTHZ_SECURITY_EVENT_PROVIDER_HANDLE,DWORD,PSID,PAUDIT_PARAMS);
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
AUTHZAPI BOOL WINAPI AuthzRegisterCapChangeNotification(PAUTHZ_CAP_CHANGE_SUBSCRIPTION_HANDLE, LPTHREAD_START_ROUTINE, PVOID);
AUTHZAPI BOOL WINAPI AuthzUnregisterCapChangeNotification(AUTHZ_CAP_CHANGE_SUBSCRIPTION_HANDLE);
AUTHZAPI BOOL WINAPI AuthzFreeCentralAccessPolicyCache(void);
#endif /* _WIN32_WINNT >= _WIN32_WINNT_WIN8 */

#if __POCC__ >= 290
#pragma warn(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _AUTHZ_H */
