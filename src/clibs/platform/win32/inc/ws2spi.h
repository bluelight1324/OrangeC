#ifndef _WS2SPI_H
#define _WS2SPI_H

#ifdef __ORANGEC__ 
#pragma once
#endif

/* Winsock service provider definitions */

#if !defined(_WIN64)
#include <pshpack4.h>
#endif

#ifndef _WINSOCK2_H
#include <winsock2.h>
#endif

#define WSPDESCRIPTION_LEN  255

#define WSS_OPERATION_IN_PROGRESS  0x00000103L

typedef struct WSPData {
    WORD wVersion;
    WORD wHighVersion;
    WCHAR szDescription[WSPDESCRIPTION_LEN+1];
} WSPDATA, *LPWSPDATA;

typedef struct _WSATHREADID {
    HANDLE ThreadHandle;
    DWORD_PTR Reserved;
} WSATHREADID, *LPWSATHREADID;

#define WSPAPI WSAAPI

#ifdef __cplusplus
extern "C" {
#endif

typedef BOOL (CALLBACK *LPBLOCKINGCALLBACK)(DWORD_PTR);
typedef VOID (CALLBACK *LPWSAUSERAPC)(DWORD_PTR);
typedef SOCKET (WSPAPI *LPWSPACCEPT)(SOCKET,struct sockaddr*,LPINT,LPCONDITIONPROC,DWORD_PTR,LPINT);
typedef INT (WSPAPI *LPWSPADDRESSTOSTRING)(LPSOCKADDR,DWORD,LPWSAPROTOCOL_INFOW,LPWSTR,LPDWORD,LPINT);
typedef int (WSPAPI *LPWSPASYNCSELECT)(SOCKET,HWND,unsigned int,long,LPINT);
typedef int (WSPAPI *LPWSPBIND)(SOCKET,const struct sockaddr*,int,LPINT);
typedef int (WSPAPI *LPWSPCANCELBLOCKINGCALL)(LPINT);
typedef int (WSPAPI *LPWSPCLEANUP)(LPINT);
typedef int (WSPAPI *LPWSPCLOSESOCKET)(SOCKET,LPINT);
typedef int (WSPAPI *LPWSPCONNECT)(SOCKET,const struct sockaddr*,int,LPWSABUF,LPWSABUF,LPQOS,LPQOS,LPINT);
typedef int (WSPAPI *LPWSPDUPLICATESOCKET)(SOCKET,DWORD,LPWSAPROTOCOL_INFOW,LPINT);
typedef int (WSPAPI *LPWSPENUMNETWORKEVENTS)(SOCKET,WSAEVENT,LPWSANETWORKEVENTS,LPINT);
typedef int (WSPAPI *LPWSPEVENTSELECT)(SOCKET,WSAEVENT,long,LPINT);
typedef BOOL (WSPAPI *LPWSPGETOVERLAPPEDRESULT)(SOCKET,LPWSAOVERLAPPED,LPDWORD,BOOL,LPDWORD,LPINT);
typedef int (WSPAPI *LPWSPGETPEERNAME)(SOCKET,struct sockaddr*,LPINT,LPINT);
typedef int (WSPAPI *LPWSPGETSOCKNAME)(SOCKET,struct sockaddr*,LPINT,LPINT);
typedef int (WSPAPI *LPWSPGETSOCKOPT)(SOCKET,int,int,char*,LPINT,LPINT);
typedef BOOL (WSPAPI *LPWSPGETQOSBYNAME)(SOCKET,LPWSABUF,LPQOS,LPINT);
typedef int (WSPAPI *LPWSPIOCTL)(SOCKET,DWORD,LPVOID,DWORD,LPVOID,DWORD,LPDWORD,LPWSAOVERLAPPED,LPWSAOVERLAPPED_COMPLETION_ROUTINE,LPWSATHREADID,LPINT);
typedef SOCKET (WSPAPI *LPWSPJOINLEAF)(SOCKET,const struct sockaddr*,int,LPWSABUF,LPWSABUF,LPQOS,LPQOS,DWORD,LPINT);
typedef int (WSPAPI *LPWSPLISTEN)(SOCKET,int,LPINT);
typedef int (WSPAPI *LPWSPRECV)(SOCKET,LPWSABUF,DWORD,LPDWORD,LPDWORD,LPWSAOVERLAPPED,LPWSAOVERLAPPED_COMPLETION_ROUTINE,LPWSATHREADID,LPINT);
typedef int (WSPAPI *LPWSPRECVDISCONNECT)(SOCKET,LPWSABUF,LPINT);
typedef int (WSPAPI *LPWSPRECVFROM)(SOCKET,LPWSABUF,DWORD,LPDWORD,LPDWORD,struct sockaddr*,LPINT,LPWSAOVERLAPPED,LPWSAOVERLAPPED_COMPLETION_ROUTINE,LPWSATHREADID,LPINT);
typedef int (WSPAPI *LPWSPSELECT)(int,fd_set*,fd_set*,fd_set*,const struct timeval*,LPINT);
typedef int (WSPAPI *LPWSPSEND)(SOCKET,LPWSABUF,DWORD,LPDWORD,DWORD,LPWSAOVERLAPPED,LPWSAOVERLAPPED_COMPLETION_ROUTINE,LPWSATHREADID,LPINT);
typedef int (WSPAPI *LPWSPSENDDISCONNECT)(SOCKET,LPWSABUF,LPINT);
typedef int (WSPAPI *LPWSPSENDTO)(SOCKET,LPWSABUF,DWORD,LPDWORD,DWORD,const struct sockaddr*,int,LPWSAOVERLAPPED,LPWSAOVERLAPPED_COMPLETION_ROUTINE,LPWSATHREADID,LPINT);
typedef int (WSPAPI *LPWSPSETSOCKOPT)(SOCKET,int,int,const char*,int,LPINT);
typedef int (WSPAPI *LPWSPSHUTDOWN)(SOCKET,int,LPINT);
typedef SOCKET (WSPAPI *LPWSPSOCKET)(int,int,int,LPWSAPROTOCOL_INFOW,GROUP,DWORD,LPINT);
typedef INT (WSPAPI *LPWSPSTRINGTOADDRESS)(LPWSTR,INT,LPWSAPROTOCOL_INFOW,LPSOCKADDR,LPINT,LPINT);

typedef struct _WSPPROC_TABLE {
    LPWSPACCEPT lpWSPAccept;
    LPWSPADDRESSTOSTRING lpWSPAddressToString;
    LPWSPASYNCSELECT lpWSPAsyncSelect;
    LPWSPBIND lpWSPBind;
    LPWSPCANCELBLOCKINGCALL lpWSPCancelBlockingCall;
    LPWSPCLEANUP lpWSPCleanup;
    LPWSPCLOSESOCKET lpWSPCloseSocket;
    LPWSPCONNECT lpWSPConnect;
    LPWSPDUPLICATESOCKET lpWSPDuplicateSocket;
    LPWSPENUMNETWORKEVENTS lpWSPEnumNetworkEvents;
    LPWSPEVENTSELECT lpWSPEventSelect;
    LPWSPGETOVERLAPPEDRESULT lpWSPGetOverlappedResult;
    LPWSPGETPEERNAME lpWSPGetPeerName;
    LPWSPGETSOCKNAME lpWSPGetSockName;
    LPWSPGETSOCKOPT lpWSPGetSockOpt;
    LPWSPGETQOSBYNAME lpWSPGetQOSByName;
    LPWSPIOCTL lpWSPIoctl;
    LPWSPJOINLEAF lpWSPJoinLeaf;
    LPWSPLISTEN lpWSPListen;
    LPWSPRECV lpWSPRecv;
    LPWSPRECVDISCONNECT lpWSPRecvDisconnect;
    LPWSPRECVFROM lpWSPRecvFrom;
    LPWSPSELECT lpWSPSelect;
    LPWSPSEND lpWSPSend;
    LPWSPSENDDISCONNECT lpWSPSendDisconnect;
    LPWSPSENDTO lpWSPSendTo;
    LPWSPSETSOCKOPT lpWSPSetSockOpt;
    LPWSPSHUTDOWN lpWSPShutdown;
    LPWSPSOCKET lpWSPSocket;
    LPWSPSTRINGTOADDRESS lpWSPStringToAddress;
} WSPPROC_TABLE, *LPWSPPROC_TABLE;

typedef BOOL (WSPAPI *LPWPUCLOSEEVENT)(WSAEVENT,LPINT);
typedef int (WSPAPI *LPWPUCLOSESOCKETHANDLE)(SOCKET,LPINT);
typedef WSAEVENT (WSPAPI *LPWPUCREATEEVENT)(LPINT);
typedef SOCKET (WSPAPI *LPWPUCREATESOCKETHANDLE)(DWORD,DWORD_PTR,LPINT);
typedef int (WSPAPI *LPWPUFDISSET)(SOCKET,fd_set*);
typedef int (WSPAPI *LPWPUGETPROVIDERPATH)(LPGUID,WCHAR*,LPINT,LPINT);
typedef SOCKET (WSPAPI *LPWPUMODIFYIFSHANDLE)(DWORD,SOCKET,LPINT);
typedef BOOL (WSPAPI *LPWPUPOSTMESSAGE)(HWND,UINT,WPARAM,LPARAM);
typedef int (WSPAPI *LPWPUQUERYBLOCKINGCALLBACK)(DWORD,LPBLOCKINGCALLBACK*,PDWORD_PTR,LPINT);
typedef int (WSPAPI *LPWPUQUERYSOCKETHANDLECONTEXT)(SOCKET,PDWORD_PTR,LPINT);
typedef int (WSPAPI *LPWPUQUEUEAPC)(LPWSATHREADID,LPWSAUSERAPC,DWORD_PTR,LPINT);
typedef BOOL (WSPAPI *LPWPURESETEVENT)(WSAEVENT,LPINT);
typedef BOOL (WSPAPI *LPWPUSETEVENT)(WSAEVENT,LPINT);
typedef int (WSPAPI *LPWPUOPENCURRENTTHREAD)(LPWSATHREADID,LPINT);
typedef int (WSPAPI *LPWPUCLOSETHREAD)(LPWSATHREADID,LPINT);
typedef int (WSPAPI *LPWPUCOMPLETEOVERLAPPEDREQUEST)(SOCKET,LPWSAOVERLAPPED,DWORD,DWORD,LPINT);

typedef struct _WSPUPCALLTABLE {
    LPWPUCLOSEEVENT lpWPUCloseEvent;
    LPWPUCLOSESOCKETHANDLE lpWPUCloseSocketHandle;
    LPWPUCREATEEVENT lpWPUCreateEvent;
    LPWPUCREATESOCKETHANDLE lpWPUCreateSocketHandle;
    LPWPUFDISSET lpWPUFDIsSet;
    LPWPUGETPROVIDERPATH lpWPUGetProviderPath;
    LPWPUMODIFYIFSHANDLE lpWPUModifyIFSHandle;
    LPWPUPOSTMESSAGE lpWPUPostMessage;
    LPWPUQUERYBLOCKINGCALLBACK lpWPUQueryBlockingCallback;
    LPWPUQUERYSOCKETHANDLECONTEXT lpWPUQuerySocketHandleContext;
    LPWPUQUEUEAPC lpWPUQueueApc;
    LPWPURESETEVENT lpWPUResetEvent;
    LPWPUSETEVENT lpWPUSetEvent;
    LPWPUOPENCURRENTTHREAD lpWPUOpenCurrentThread;
    LPWPUCLOSETHREAD lpWPUCloseThread;
} WSPUPCALLTABLE, *LPWSPUPCALLTABLE;

typedef int (WSPAPI *LPWSPSTARTUP)(WORD,LPWSPDATA,LPWSAPROTOCOL_INFOW,WSPUPCALLTABLE,LPWSPPROC_TABLE);
typedef int (WSPAPI *LPWSCENUMPROTOCOLS)(LPINT,LPWSAPROTOCOL_INFOW,LPDWORD,LPINT);
typedef int (WSPAPI *LPWSCDEINSTALLPROVIDER)(LPGUID,LPINT);
typedef int (WSPAPI *LPWSCINSTALLPROVIDER)(LPGUID,const WCHAR*,const LPWSAPROTOCOL_INFOW,DWORD,LPINT);
typedef int (WSPAPI *LPWSCGETPROVIDERPATH)(LPGUID,WCHAR*,LPINT,LPINT);
typedef int (WSPAPI * LPWSCUPDATEPROVIDER)(LPGUID,const WCHAR*,const LPWSAPROTOCOL_INFOW,DWORD,LPINT);
typedef int (WSPAPI *LPWSCINSTALLQOSTEMPLATE)(const LPGUID,LPWSABUF,LPQOS);
typedef int (WSPAPI *LPWSCREMOVEQOSTEMPLATE)(const LPGUID,LPWSABUF);
typedef INT (WSPAPI *LPWSCINSTALLNAMESPACE)(LPWSTR,LPWSTR,DWORD,DWORD,LPGUID);
typedef INT (WSPAPI *LPWSCUNINSTALLNAMESPACE)(LPGUID);
typedef INT (WSPAPI *LPWSCENABLENSPROVIDER)(LPGUID,BOOL);

typedef INT (WSAAPI *LPNSPCLEANUP)(LPGUID);
typedef INT (WSAAPI *LPNSPLOOKUPSERVICEBEGIN)(LPGUID,LPWSAQUERYSETW,LPWSASERVICECLASSINFOW,DWORD,LPHANDLE);
typedef INT (WSAAPI *LPNSPLOOKUPSERVICENEXT)(HANDLE,DWORD,LPDWORD,LPWSAQUERYSETW);
typedef INT (WSAAPI *LPNSPLOOKUPSERVICEEND)(HANDLE);
typedef INT (WSAAPI *LPNSPIOCTL)(HANDLE,DWORD,LPVOID,DWORD,LPVOID,DWORD,LPDWORD,LPWSACOMPLETION,LPWSATHREADID);
typedef INT (WSAAPI *LPNSPSETSERVICE)(LPGUID,LPWSASERVICECLASSINFOW,LPWSAQUERYSETW,WSAESETSERVICEOP,DWORD);
typedef INT (WSAAPI *LPNSPINSTALLSERVICECLASS)(LPGUID,LPWSASERVICECLASSINFOW);
typedef INT (WSAAPI *LPNSPREMOVESERVICECLASS)(LPGUID,LPGUID);
typedef INT (WSAAPI *LPNSPGETSERVICECLASSINFO)(LPGUID,LPDWORD,LPWSASERVICECLASSINFOW);

typedef struct _NSP_ROUTINE {
    DWORD cbSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    LPNSPCLEANUP NSPCleanup;
    LPNSPLOOKUPSERVICEBEGIN NSPLookupServiceBegin;
    LPNSPLOOKUPSERVICENEXT NSPLookupServiceNext;
    LPNSPLOOKUPSERVICEEND NSPLookupServiceEnd;
    LPNSPSETSERVICE NSPSetService;
    LPNSPINSTALLSERVICECLASS NSPInstallServiceClass;
    LPNSPREMOVESERVICECLASS NSPRemoveServiceClass;
    LPNSPGETSERVICECLASSINFO NSPGetServiceClassInfo;
    LPNSPIOCTL NSPIoctl;
} NSP_ROUTINE, *LPNSP_ROUTINE;

typedef INT (WSAAPI *LPNSPSTARTUP)(LPGUID,LPNSP_ROUTINE);

#define WSCEnumNameSpaceProviders WSAEnumNameSpaceProvidersW
#define LPFN_WSCENUMNAMESPACEPROVIDERS LPFN_WSAENUMNAMESPACEPROVIDERSW

int WSPAPI WSPStartup(WORD,LPWSPDATA,LPWSAPROTOCOL_INFOW,WSPUPCALLTABLE,LPWSPPROC_TABLE);
int WSPAPI WSCEnumProtocols(LPINT,LPWSAPROTOCOL_INFOW,LPDWORD,LPINT);
int WSPAPI WSCDeinstallProvider(LPGUID,LPINT);
int WSPAPI WSCInstallProvider(LPGUID,const WCHAR*,const LPWSAPROTOCOL_INFOW,DWORD,LPINT);
int WSPAPI WSCGetProviderPath(LPGUID,WCHAR*,LPINT,LPINT);
int WSPAPI WSCUpdateProvider(LPGUID,const WCHAR*,const LPWSAPROTOCOL_INFOW,DWORD,LPINT);
int WSPAPI WSCInstallQOSTemplate(const LPGUID,LPWSABUF,LPQOS);
int WSPAPI WSCRemoveQOSTemplate(const LPGUID,LPWSABUF);
BOOL WSPAPI WPUCloseEvent(WSAEVENT,LPINT);
int WSPAPI WPUCloseSocketHandle(SOCKET,LPINT);
WSAEVENT WSPAPI WPUCreateEvent(LPINT);
SOCKET WSPAPI WPUCreateSocketHandle(DWORD,DWORD_PTR,LPINT);
int WSPAPI WPUFDIsSet(SOCKET,fd_set*);
int WSPAPI WPUGetProviderPath(LPGUID,WCHAR*,LPINT,LPINT);
SOCKET WSPAPI WPUModifyIFSHandle(DWORD,SOCKET,LPINT);
BOOL WSPAPI WPUPostMessage(HWND,UINT,WPARAM,LPARAM);
int WSPAPI WPUQueryBlockingCallback(DWORD,LPBLOCKINGCALLBACK*,PDWORD_PTR,LPINT);
int WSPAPI WPUQuerySocketHandleContext(SOCKET,PDWORD_PTR,LPINT);
int WSPAPI WPUQueueApc(LPWSATHREADID,LPWSAUSERAPC,DWORD_PTR,LPINT);
BOOL WSPAPI WPUResetEvent(WSAEVENT,LPINT);
BOOL WSPAPI WPUSetEvent(WSAEVENT,LPINT);
int WSPAPI WPUOpenCurrentThread(LPWSATHREADID,LPINT);
int WSPAPI WPUCloseThread(LPWSATHREADID,LPINT);
int WSPAPI WPUCompleteOverlappedRequest(SOCKET,LPWSAOVERLAPPED,DWORD,DWORD,LPINT);
INT WSPAPI WSCInstallNameSpace(LPWSTR,LPWSTR,DWORD,DWORD,LPGUID);
INT WSPAPI WSCUnInstallNameSpace(LPGUID);
INT WSPAPI WSCEnableNSProvider(LPGUID,BOOL);
INT WSAAPI NSPStartup(LPGUID,LPNSP_ROUTINE);

#if defined(_WIN64)
int WSPAPI WSCEnumProtocols32(LPINT,LPWSAPROTOCOL_INFOW,LPDWORD,LPINT);
int WSPAPI WSCDeinstallProvider32(LPGUID,LPINT);
int WSPAPI WSCInstallProvider64_32(LPGUID,const WCHAR*,const LPWSAPROTOCOL_INFOW,DWORD,LPINT);
int WSPAPI WSCGetProviderPath32(LPGUID,WCHAR*,LPINT,LPINT);
int WSPAPI WSCUpdateProvider32(LPGUID,const WCHAR*,const LPWSAPROTOCOL_INFOW,DWORD,LPINT);
INT WSAAPI WSCEnumNameSpaceProviders32(LPDWORD,LPWSANAMESPACE_INFOW);
INT WSPAPI WSCInstallNameSpace32(LPWSTR,LPWSTR,DWORD,DWORD,LPGUID);
INT WSPAPI WSCUnInstallNameSpace32(LPGUID);
INT WSPAPI WSCEnableNSProvider32(LPGUID,BOOL);
#endif /* _WIN64 */

#ifdef __cplusplus
}
#endif

#if !defined(_WIN64)
#include <poppack.h>
#endif

#endif /* _WS2SPI_H */
