#ifndef _MGM_H
#define _MGM_H

#ifdef __ORANGEC__ 
#pragma once
#endif

/* MGM API definitions */

typedef struct _MGM_IF_ENTRY {
    DWORD dwIfIndex;
    DWORD dwIfNextHopAddr;
    BOOL bIGMP;
    BOOL bIsEnabled;
} MGM_IF_ENTRY, *PMGM_IF_ENTRY;

typedef DWORD(*PMGM_RPF_CALLBACK)(DWORD,DWORD,DWORD,DWORD,PDWORD,PDWORD,PDWORD,DWORD,PBYTE,PBYTE);
typedef DWORD(*PMGM_CREATION_ALERT_CALLBACK)(DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,PMGM_IF_ENTRY);
typedef DWORD(*PMGM_PRUNE_ALERT_CALLBACK)(DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,BOOL,PDWORD);
typedef DWORD(*PMGM_JOIN_ALERT_CALLBACK)(DWORD,DWORD,DWORD,DWORD,BOOL);
typedef DWORD(*PMGM_WRONG_IF_CALLBACK)(DWORD,DWORD,DWORD,DWORD,DWORD,PBYTE);
typedef DWORD(*PMGM_LOCAL_JOIN_CALLBACK)(DWORD,DWORD,DWORD,DWORD,DWORD,DWORD);
typedef DWORD(*PMGM_LOCAL_LEAVE_CALLBACK)(DWORD,DWORD,DWORD,DWORD,DWORD,DWORD);
typedef DWORD(*PMGM_DISABLE_IGMP_CALLBACK)(DWORD,DWORD);
typedef DWORD(*PMGM_ENABLE_IGMP_CALLBACK)(DWORD,DWORD);

typedef struct _ROUTING_PROTOCOL_CONFIG {
    DWORD dwCallbackFlags;
    PMGM_RPF_CALLBACK pfnRpfCallback;
    PMGM_CREATION_ALERT_CALLBACK pfnCreationAlertCallback;
    PMGM_PRUNE_ALERT_CALLBACK pfnPruneAlertCallback;
    PMGM_JOIN_ALERT_CALLBACK pfnJoinAlertCallback;
    PMGM_WRONG_IF_CALLBACK pfnWrongIfCallback;
    PMGM_LOCAL_JOIN_CALLBACK pfnLocalJoinCallback;
    PMGM_LOCAL_LEAVE_CALLBACK pfnLocalLeaveCallback;
    PMGM_DISABLE_IGMP_CALLBACK pfnDisableIgmpCallback;
    PMGM_ENABLE_IGMP_CALLBACK pfnEnableIgmpCallback;
} ROUTING_PROTOCOL_CONFIG, *PROUTING_PROTOCOL_CONFIG;

typedef enum _MGM_ENUM_TYPES {
    ANY_SOURCE = 0,
    ALL_SOURCES
} MGM_ENUM_TYPES;

typedef struct _SOURCE_GROUP_ENTRY {
    DWORD dwSourceAddr;
    DWORD dwSourceMask;
    DWORD dwGroupAddr;
    DWORD dwGroupMask;
} SOURCE_GROUP_ENTRY, *PSOURCE_GROUP_ENTRY;

DWORD MgmRegisterMProtocol(PROUTING_PROTOCOL_CONFIG,DWORD,DWORD,HANDLE*);
DWORD MgmDeRegisterMProtocol(HANDLE);
DWORD MgmTakeInterfaceOwnership(HANDLE,DWORD,DWORD);
DWORD MgmReleaseInterfaceOwnership(HANDLE,DWORD,DWORD);
DWORD MgmGetProtocolOnInterface(DWORD,DWORD,PDWORD,PDWORD);
DWORD MgmAddGroupMembershipEntry(HANDLE,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD);
DWORD MgmDeleteGroupMembershipEntry(HANDLE,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD);
DWORD MgmGetMfe(PMIB_IPMCAST_MFE,PDWORD,PBYTE);
DWORD MgmGetFirstMfe(PDWORD,PBYTE,PDWORD);
DWORD MgmGetNextMfe(PMIB_IPMCAST_MFE,PDWORD,PBYTE,PDWORD);
DWORD MgmGetMfeStats(PMIB_IPMCAST_MFE,PDWORD,PBYTE);
DWORD MgmGetFirstMfeStats(PDWORD,PBYTE,PDWORD);
DWORD MgmGetNextMfeStats(PMIB_IPMCAST_MFE,PDWORD,PBYTE,PDWORD);
DWORD MgmGroupEnumerationStart(HANDLE,MGM_ENUM_TYPES,HANDLE*);
DWORD MgmGroupEnumerationGetNext(HANDLE,PDWORD,PBYTE,PDWORD);
DWORD MgmGroupEnumerationEnd(HANDLE);
DWORD MgmSetMfe(HANDLE,PMIB_IPMCAST_MFE);

#endif /* _MGM_H */
