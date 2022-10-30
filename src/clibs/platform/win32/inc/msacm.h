#ifndef _MSACM_H
#define _MSACM_H

#ifdef __ORANGEC__ 
#pragma once
#endif

/* Windows Audio Compression Manager definitions */

#ifdef __cplusplus
extern "C" {
#endif

#if __POCC__ >= 290
#pragma warn(push)
#pragma warn(disable:2185)  /* Alignment of field 'x' can be less than the natural alignment */
#endif

#include "pshpack1.h"

#ifndef DRV_MAPPER_PREFERRED_INPUT_GET
#define DRV_MAPPER_PREFERRED_INPUT_GET  (DRV_USER+0)
#endif
#ifndef DRV_MAPPER_PREFERRED_OUTPUT_GET
#define DRV_MAPPER_PREFERRED_OUTPUT_GET  (DRV_USER+2)
#endif

#ifndef DRVM_MAPPER_STATUS
#define DRVM_MAPPER  0x2000
#define DRVM_MAPPER_STATUS  (DRVM_MAPPER+0)
#endif

#ifndef WIDM_MAPPER_STATUS
#define WIDM_MAPPER_STATUS  (DRVM_MAPPER_STATUS+0)
#define WAVEIN_MAPPER_STATUS_DEVICE  0
#define WAVEIN_MAPPER_STATUS_MAPPED  1
#define WAVEIN_MAPPER_STATUS_FORMAT  2
#endif

#ifndef WODM_MAPPER_STATUS
#define WODM_MAPPER_STATUS  (DRVM_MAPPER_STATUS+0)
#define WAVEOUT_MAPPER_STATUS_DEVICE  0
#define WAVEOUT_MAPPER_STATUS_MAPPED  1
#define WAVEOUT_MAPPER_STATUS_FORMAT  2
#endif

#define ACMAPI WINAPI

DECLARE_HANDLE(HACMDRIVERID);
typedef HACMDRIVERID *PHACMDRIVERID;
typedef HACMDRIVERID *LPHACMDRIVERID;

DECLARE_HANDLE(HACMDRIVER);
typedef HACMDRIVER *PHACMDRIVER;
typedef HACMDRIVER *LPHACMDRIVER;

DECLARE_HANDLE(HACMSTREAM);
typedef HACMSTREAM *PHACMSTREAM;
typedef HACMSTREAM *LPHACMSTREAM;

DECLARE_HANDLE(HACMOBJ);
typedef HACMOBJ *PHACMOBJ;
typedef HACMOBJ *LPHACMOBJ;

#ifndef _MMRESULT_
#define _MMRESULT_
typedef UINT MMRESULT;
#endif

#define ACMERR_BASE  512
#define ACMERR_NOTPOSSIBLE  (ACMERR_BASE+0)
#define ACMERR_BUSY  (ACMERR_BASE+1)
#define ACMERR_UNPREPARED  (ACMERR_BASE+2)
#define ACMERR_CANCELED  (ACMERR_BASE+3)

#define MM_ACM_OPEN  (MM_STREAM_OPEN)
#define MM_ACM_CLOSE  (MM_STREAM_CLOSE)
#define MM_ACM_DONE  (MM_STREAM_DONE)

#define ACM_METRIC_COUNT_DRIVERS  1
#define ACM_METRIC_COUNT_CODECS  2
#define ACM_METRIC_COUNT_CONVERTERS  3
#define ACM_METRIC_COUNT_FILTERS  4
#define ACM_METRIC_COUNT_DISABLED  5
#define ACM_METRIC_COUNT_HARDWARE  6
#define ACM_METRIC_COUNT_LOCAL_DRIVERS  20
#define ACM_METRIC_COUNT_LOCAL_CODECS  21
#define ACM_METRIC_COUNT_LOCAL_CONVERTERS  22
#define ACM_METRIC_COUNT_LOCAL_FILTERS  23
#define ACM_METRIC_COUNT_LOCAL_DISABLED  24
#define ACM_METRIC_HARDWARE_WAVE_INPUT  30
#define ACM_METRIC_HARDWARE_WAVE_OUTPUT  31
#define ACM_METRIC_MAX_SIZE_FORMAT  50
#define ACM_METRIC_MAX_SIZE_FILTER  51
#define ACM_METRIC_DRIVER_SUPPORT  100
#define ACM_METRIC_DRIVER_PRIORITY  101

#define ACM_DRIVERENUMF_NOLOCAL  0x40000000L
#define ACM_DRIVERENUMF_DISABLED  0x80000000L

#define ACM_DRIVERADDF_FUNCTION  0x00000003L
#define ACM_DRIVERADDF_NOTIFYHWND  0x00000004L
#define ACM_DRIVERADDF_TYPEMASK  0x00000007L
#define ACM_DRIVERADDF_LOCAL  0x00000000L
#define ACM_DRIVERADDF_GLOBAL  0x00000008L

#define ACMDM_USER  (DRV_USER+0x0000)
#define ACMDM_RESERVED_LOW  (DRV_USER+0x2000)
#define ACMDM_RESERVED_HIGH  (DRV_USER+0x2FFF)

#define ACMDM_BASE  ACMDM_RESERVED_LOW

#define ACMDM_DRIVER_ABOUT  (ACMDM_BASE+11)

#define ACM_DRIVERPRIORITYF_ENABLE  0x00000001L
#define ACM_DRIVERPRIORITYF_DISABLE  0x00000002L
#define ACM_DRIVERPRIORITYF_ABLEMASK  0x00000003L
#define ACM_DRIVERPRIORITYF_BEGIN  0x00010000L
#define ACM_DRIVERPRIORITYF_END  0x00020000L
#define ACM_DRIVERPRIORITYF_DEFERMASK  0x00030000L

#define ACMDRIVERDETAILS_SHORTNAME_CHARS  32
#define ACMDRIVERDETAILS_LONGNAME_CHARS  128
#define ACMDRIVERDETAILS_COPYRIGHT_CHARS  80
#define ACMDRIVERDETAILS_LICENSING_CHARS  128
#define ACMDRIVERDETAILS_FEATURES_CHARS  512

#define ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC mmioFOURCC('a','u','d','c')
#define ACMDRIVERDETAILS_FCCCOMP_UNDEFINED mmioFOURCC('\0','\0','\0','\0')

#define ACMDRIVERDETAILS_SUPPORTF_CODEC  0x00000001L
#define ACMDRIVERDETAILS_SUPPORTF_CONVERTER  0x00000002L
#define ACMDRIVERDETAILS_SUPPORTF_FILTER  0x00000004L
#define ACMDRIVERDETAILS_SUPPORTF_HARDWARE  0x00000008L
#define ACMDRIVERDETAILS_SUPPORTF_ASYNC  0x00000010L
#define ACMDRIVERDETAILS_SUPPORTF_LOCAL  0x40000000L
#define ACMDRIVERDETAILS_SUPPORTF_DISABLED  0x80000000L

#define ACMFORMATTAGDETAILS_FORMATTAG_CHARS  48

#define ACM_FORMATTAGDETAILSF_INDEX  0x00000000L
#define ACM_FORMATTAGDETAILSF_FORMATTAG  0x00000001L
#define ACM_FORMATTAGDETAILSF_LARGESTSIZE  0x00000002L
#define ACM_FORMATTAGDETAILSF_QUERYMASK  0x0000000FL

#define ACMFORMATDETAILS_FORMAT_CHARS  128

#define ACM_FORMATDETAILSF_INDEX  0x00000000L
#define ACM_FORMATDETAILSF_FORMAT  0x00000001L
#define ACM_FORMATDETAILSF_QUERYMASK  0x0000000FL

#define ACM_FORMATENUMF_WFORMATTAG  0x00010000L
#define ACM_FORMATENUMF_NCHANNELS  0x00020000L
#define ACM_FORMATENUMF_NSAMPLESPERSEC  0x00040000L
#define ACM_FORMATENUMF_WBITSPERSAMPLE  0x00080000L
#define ACM_FORMATENUMF_CONVERT  0x00100000L
#define ACM_FORMATENUMF_SUGGEST  0x00200000L
#define ACM_FORMATENUMF_HARDWARE  0x00400000L
#define ACM_FORMATENUMF_INPUT  0x00800000L
#define ACM_FORMATENUMF_OUTPUT  0x01000000L

#define ACM_FORMATSUGGESTF_WFORMATTAG  0x00010000L
#define ACM_FORMATSUGGESTF_NCHANNELS  0x00020000L
#define ACM_FORMATSUGGESTF_NSAMPLESPERSEC  0x00040000L
#define ACM_FORMATSUGGESTF_WBITSPERSAMPLE  0x00080000L
#define ACM_FORMATSUGGESTF_TYPEMASK  0x00FF0000L

#define ACMHELPMSGSTRINGA  "acmchoose_help"
#define ACMHELPMSGSTRINGW  L"acmchoose_help"
#define ACMHELPMSGCONTEXTMENUA  "acmchoose_contextmenu"
#define ACMHELPMSGCONTEXTMENUW  L"acmchoose_contextmenu"
#define ACMHELPMSGCONTEXTHELPA  "acmchoose_contexthelp"
#define ACMHELPMSGCONTEXTHELPW  L"acmchoose_contexthelp"

#define MM_ACM_FORMATCHOOSE  0x8000

#define FORMATCHOOSE_MESSAGE  0
#define FORMATCHOOSE_FORMATTAG_VERIFY  (FORMATCHOOSE_MESSAGE+0)
#define FORMATCHOOSE_FORMAT_VERIFY  (FORMATCHOOSE_MESSAGE+1)
#define FORMATCHOOSE_CUSTOM_VERIFY  (FORMATCHOOSE_MESSAGE+2)

#define ACMFORMATCHOOSE_STYLEF_SHOWHELP  0x00000004L
#define ACMFORMATCHOOSE_STYLEF_ENABLEHOOK  0x00000008L
#define ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATE  0x00000010L
#define ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATEHANDLE  0x00000020L
#define ACMFORMATCHOOSE_STYLEF_INITTOWFXSTRUCT  0x00000040L
#define ACMFORMATCHOOSE_STYLEF_CONTEXTHELP  0x00000080L

#define ACMFILTERTAGDETAILS_FILTERTAG_CHARS  48

#define ACM_FILTERTAGDETAILSF_INDEX  0x00000000L
#define ACM_FILTERTAGDETAILSF_FILTERTAG  0x00000001L
#define ACM_FILTERTAGDETAILSF_LARGESTSIZE  0x00000002L
#define ACM_FILTERTAGDETAILSF_QUERYMASK  0x0000000FL

#define ACMFILTERDETAILS_FILTER_CHARS  128

#define ACM_FILTERDETAILSF_INDEX  0x00000000L
#define ACM_FILTERDETAILSF_FILTER  0x00000001L
#define ACM_FILTERDETAILSF_QUERYMASK  0x0000000FL

#define ACM_FILTERENUMF_DWFILTERTAG  0x00010000L

#define MM_ACM_FILTERCHOOSE  0x8000

#define FILTERCHOOSE_MESSAGE  0
#define FILTERCHOOSE_FILTERTAG_VERIFY  (FILTERCHOOSE_MESSAGE+0)
#define FILTERCHOOSE_FILTER_VERIFY  (FILTERCHOOSE_MESSAGE+1)
#define FILTERCHOOSE_CUSTOM_VERIFY  (FILTERCHOOSE_MESSAGE+2)

#define ACMFILTERCHOOSE_STYLEF_SHOWHELP  0x00000004L
#define ACMFILTERCHOOSE_STYLEF_ENABLEHOOK  0x00000008L
#define ACMFILTERCHOOSE_STYLEF_ENABLETEMPLATE  0x00000010L
#define ACMFILTERCHOOSE_STYLEF_ENABLETEMPLATEHANDLE  0x00000020L
#define ACMFILTERCHOOSE_STYLEF_INITTOFILTERSTRUCT  0x00000040L
#define ACMFILTERCHOOSE_STYLEF_CONTEXTHELP  0x00000080L

#define ACMSTREAMHEADER_STATUSF_DONE  0x00010000L
#define ACMSTREAMHEADER_STATUSF_PREPARED  0x00020000L
#define ACMSTREAMHEADER_STATUSF_INQUEUE  0x00100000L

#define ACM_STREAMOPENF_QUERY  0x00000001
#define ACM_STREAMOPENF_ASYNC  0x00000002
#define ACM_STREAMOPENF_NONREALTIME  0x00000004

#define ACM_STREAMSIZEF_SOURCE  0x00000000L
#define ACM_STREAMSIZEF_DESTINATION  0x00000001L
#define ACM_STREAMSIZEF_QUERYMASK  0x0000000FL

#define ACM_STREAMCONVERTF_BLOCKALIGN  0x00000004
#define ACM_STREAMCONVERTF_START  0x00000010
#define ACM_STREAMCONVERTF_END  0x00000020

typedef BOOL(CALLBACK *ACMDRIVERENUMCB)(HACMDRIVERID,DWORD_PTR,DWORD);

typedef LRESULT(CALLBACK *ACMDRIVERPROC)(DWORD_PTR,HACMDRIVERID,UINT,LPARAM,LPARAM);
typedef ACMDRIVERPROC *LPACMDRIVERPROC;

typedef struct tACMDRIVERDETAILSA {
    DWORD cbStruct;
    FOURCC fccType;
    FOURCC fccComp;
    WORD wMid;
    WORD wPid;
    DWORD vdwACM;
    DWORD vdwDriver;
    DWORD fdwSupport;
    DWORD cFormatTags;
    DWORD cFilterTags;
    HICON hicon;
    char szShortName[ACMDRIVERDETAILS_SHORTNAME_CHARS];
    char szLongName[ACMDRIVERDETAILS_LONGNAME_CHARS];
    char szCopyright[ACMDRIVERDETAILS_COPYRIGHT_CHARS];
    char szLicensing[ACMDRIVERDETAILS_LICENSING_CHARS];
    char szFeatures[ACMDRIVERDETAILS_FEATURES_CHARS];
} ACMDRIVERDETAILSA, *PACMDRIVERDETAILSA, *LPACMDRIVERDETAILSA;

typedef struct tACMDRIVERDETAILSW {
    DWORD cbStruct;
    FOURCC fccType;
    FOURCC fccComp;
    WORD wMid;
    WORD wPid;
    DWORD vdwACM;
    DWORD vdwDriver;
    DWORD fdwSupport;
    DWORD cFormatTags;
    DWORD cFilterTags;
    HICON hicon;
    WCHAR szShortName[ACMDRIVERDETAILS_SHORTNAME_CHARS];
    WCHAR szLongName[ACMDRIVERDETAILS_LONGNAME_CHARS];
    WCHAR szCopyright[ACMDRIVERDETAILS_COPYRIGHT_CHARS];
    WCHAR szLicensing[ACMDRIVERDETAILS_LICENSING_CHARS];
    WCHAR szFeatures[ACMDRIVERDETAILS_FEATURES_CHARS];
} ACMDRIVERDETAILSW, *PACMDRIVERDETAILSW, *LPACMDRIVERDETAILSW;

typedef struct tACMFORMATTAGDETAILSA {
    DWORD cbStruct;
    DWORD dwFormatTagIndex;
    DWORD dwFormatTag;
    DWORD cbFormatSize;
    DWORD fdwSupport;
    DWORD cStandardFormats;
    char szFormatTag[ACMFORMATTAGDETAILS_FORMATTAG_CHARS];
} ACMFORMATTAGDETAILSA, *PACMFORMATTAGDETAILSA, *LPACMFORMATTAGDETAILSA;

typedef struct tACMFORMATTAGDETAILSW {
    DWORD cbStruct;
    DWORD dwFormatTagIndex;
    DWORD dwFormatTag;
    DWORD cbFormatSize;
    DWORD fdwSupport;
    DWORD cStandardFormats;
    WCHAR szFormatTag[ACMFORMATTAGDETAILS_FORMATTAG_CHARS];
} ACMFORMATTAGDETAILSW, *PACMFORMATTAGDETAILSW, *LPACMFORMATTAGDETAILSW;

typedef BOOL(CALLBACK *ACMFORMATTAGENUMCBA)(HACMDRIVERID,LPACMFORMATTAGDETAILSA,DWORD_PTR,DWORD);
typedef BOOL(CALLBACK *ACMFORMATTAGENUMCBW)(HACMDRIVERID,LPACMFORMATTAGDETAILSW,DWORD_PTR,DWORD);

typedef struct tACMFORMATDETAILSA {
    DWORD cbStruct;
    DWORD dwFormatIndex;
    DWORD dwFormatTag;
    DWORD fdwSupport;
    LPWAVEFORMATEX pwfx;
    DWORD cbwfx;
    char szFormat[ACMFORMATDETAILS_FORMAT_CHARS];
} ACMFORMATDETAILSA, *PACMFORMATDETAILSA, *LPACMFORMATDETAILSA;

typedef struct tACMFORMATDETAILSW {
    DWORD cbStruct;
    DWORD dwFormatIndex;
    DWORD dwFormatTag;
    DWORD fdwSupport;
    LPWAVEFORMATEX pwfx;
    DWORD cbwfx;
    WCHAR szFormat[ACMFORMATDETAILS_FORMAT_CHARS];
} ACMFORMATDETAILSW, *PACMFORMATDETAILSW, *LPACMFORMATDETAILSW;

typedef BOOL(CALLBACK *ACMFORMATENUMCBA)(HACMDRIVERID,LPACMFORMATDETAILSA,DWORD_PTR,DWORD);
typedef BOOL(CALLBACK *ACMFORMATENUMCBW)(HACMDRIVERID,LPACMFORMATDETAILSW,DWORD_PTR,DWORD);

typedef UINT(CALLBACK *ACMFORMATCHOOSEHOOKPROCA)(HWND,UINT,WPARAM,LPARAM);
typedef UINT(CALLBACK *ACMFORMATCHOOSEHOOKPROCW)(HWND,UINT,WPARAM,LPARAM);

typedef struct tACMFORMATCHOOSEA {
    DWORD cbStruct;
    DWORD fdwStyle;
    HWND hwndOwner;
    LPWAVEFORMATEX pwfx;
    DWORD cbwfx;
    LPCSTR pszTitle;
    char szFormatTag[ACMFORMATTAGDETAILS_FORMATTAG_CHARS];
    char szFormat[ACMFORMATDETAILS_FORMAT_CHARS];
    LPSTR pszName;
    DWORD cchName;
    DWORD fdwEnum;
    LPWAVEFORMATEX pwfxEnum;
    HINSTANCE hInstance;
    LPCSTR pszTemplateName;
    LPARAM lCustData;
    ACMFORMATCHOOSEHOOKPROCA pfnHook;
} ACMFORMATCHOOSEA, *PACMFORMATCHOOSEA, *LPACMFORMATCHOOSEA;

typedef struct tACMFORMATCHOOSEW {
    DWORD cbStruct;
    DWORD fdwStyle;
    HWND hwndOwner;
    LPWAVEFORMATEX pwfx;
    DWORD cbwfx;
    LPCWSTR pszTitle;
    WCHAR szFormatTag[ACMFORMATTAGDETAILS_FORMATTAG_CHARS];
    WCHAR szFormat[ACMFORMATDETAILS_FORMAT_CHARS];
    LPWSTR pszName;
    DWORD cchName;
    DWORD fdwEnum;
    LPWAVEFORMATEX pwfxEnum;
    HINSTANCE hInstance;
    LPCWSTR pszTemplateName;
    LPARAM lCustData;
    ACMFORMATCHOOSEHOOKPROCW pfnHook;
} ACMFORMATCHOOSEW, *PACMFORMATCHOOSEW, *LPACMFORMATCHOOSEW;

typedef struct tACMFILTERTAGDETAILSA {
    DWORD cbStruct;
    DWORD dwFilterTagIndex;
    DWORD dwFilterTag;
    DWORD cbFilterSize;
    DWORD fdwSupport;
    DWORD cStandardFilters;
    char szFilterTag[ACMFILTERTAGDETAILS_FILTERTAG_CHARS];
} ACMFILTERTAGDETAILSA, *PACMFILTERTAGDETAILSA, *LPACMFILTERTAGDETAILSA;

typedef struct tACMFILTERTAGDETAILSW {
    DWORD cbStruct;
    DWORD dwFilterTagIndex;
    DWORD dwFilterTag;
    DWORD cbFilterSize;
    DWORD fdwSupport;
    DWORD cStandardFilters;
    WCHAR szFilterTag[ACMFILTERTAGDETAILS_FILTERTAG_CHARS];
} ACMFILTERTAGDETAILSW, *PACMFILTERTAGDETAILSW, *LPACMFILTERTAGDETAILSW;

typedef BOOL(CALLBACK *ACMFILTERTAGENUMCBA)(HACMDRIVERID,LPACMFILTERTAGDETAILSA,DWORD_PTR,DWORD);
typedef BOOL(CALLBACK *ACMFILTERTAGENUMCBW)(HACMDRIVERID,LPACMFILTERTAGDETAILSW,DWORD_PTR,DWORD);

typedef struct tACMFILTERDETAILSA {
    DWORD cbStruct;
    DWORD dwFilterIndex;
    DWORD dwFilterTag;
    DWORD fdwSupport;
    LPWAVEFILTER pwfltr;
    DWORD cbwfltr;
    char szFilter[ACMFILTERDETAILS_FILTER_CHARS];
} ACMFILTERDETAILSA, *PACMFILTERDETAILSA, *LPACMFILTERDETAILSA;

typedef struct tACMFILTERDETAILSW {
    DWORD cbStruct;
    DWORD dwFilterIndex;
    DWORD dwFilterTag;
    DWORD fdwSupport;
    LPWAVEFILTER pwfltr;
    DWORD cbwfltr;
    WCHAR szFilter[ACMFILTERDETAILS_FILTER_CHARS];
} ACMFILTERDETAILSW, *PACMFILTERDETAILSW, *LPACMFILTERDETAILSW;

typedef BOOL(CALLBACK *ACMFILTERENUMCBA)(HACMDRIVERID,LPACMFILTERDETAILSA,DWORD_PTR,DWORD);
typedef BOOL(CALLBACK *ACMFILTERENUMCBW)(HACMDRIVERID,LPACMFILTERDETAILSW,DWORD_PTR,DWORD);

typedef UINT(CALLBACK *ACMFILTERCHOOSEHOOKPROCA)(HWND,UINT,WPARAM,LPARAM);
typedef UINT(CALLBACK *ACMFILTERCHOOSEHOOKPROCW)(HWND,UINT,WPARAM,LPARAM);

typedef struct tACMFILTERCHOOSEA {
    DWORD cbStruct;
    DWORD fdwStyle;
    HWND hwndOwner;
    LPWAVEFILTER pwfltr;
    DWORD cbwfltr;
    LPCSTR pszTitle;
    char szFilterTag[ACMFILTERTAGDETAILS_FILTERTAG_CHARS];
    char szFilter[ACMFILTERDETAILS_FILTER_CHARS];
    LPSTR pszName;
    DWORD cchName;
    DWORD fdwEnum;
    LPWAVEFILTER pwfltrEnum;
    HINSTANCE hInstance;
    LPCSTR pszTemplateName;
    LPARAM lCustData;
    ACMFILTERCHOOSEHOOKPROCA pfnHook;
} ACMFILTERCHOOSEA, *PACMFILTERCHOOSEA, *LPACMFILTERCHOOSEA;

typedef struct tACMFILTERCHOOSEW {
    DWORD cbStruct;
    DWORD fdwStyle;
    HWND hwndOwner;
    LPWAVEFILTER pwfltr;
    DWORD cbwfltr;
    LPCWSTR pszTitle;
    WCHAR szFilterTag[ACMFILTERTAGDETAILS_FILTERTAG_CHARS];
    WCHAR szFilter[ACMFILTERDETAILS_FILTER_CHARS];
    LPWSTR pszName;
    DWORD cchName;
    DWORD fdwEnum;
    LPWAVEFILTER pwfltrEnum;
    HINSTANCE hInstance;
    LPCWSTR pszTemplateName;
    LPARAM lCustData;
    ACMFILTERCHOOSEHOOKPROCW pfnHook;
} ACMFILTERCHOOSEW, *PACMFILTERCHOOSEW, *LPACMFILTERCHOOSEW;

typedef struct tACMSTREAMHEADER {
    DWORD cbStruct;
    DWORD fdwStatus;
    DWORD_PTR dwUser;
    LPBYTE pbSrc;
    DWORD cbSrcLength;
    DWORD cbSrcLengthUsed;
    DWORD_PTR dwSrcUser;
    LPBYTE pbDst;
    DWORD cbDstLength;
    DWORD cbDstLengthUsed;
    DWORD_PTR dwDstUser;
    DWORD dwReservedDriver[10];
} ACMSTREAMHEADER, *PACMSTREAMHEADER, *LPACMSTREAMHEADER;

DWORD ACMAPI acmGetVersion(void);
MMRESULT ACMAPI acmMetrics(HACMOBJ,UINT,LPVOID);
MMRESULT ACMAPI acmDriverEnum(ACMDRIVERENUMCB,DWORD_PTR,DWORD);
MMRESULT ACMAPI acmDriverID(HACMOBJ,LPHACMDRIVERID,DWORD);
MMRESULT ACMAPI acmDriverAddA(LPHACMDRIVERID,HINSTANCE,LPARAM,DWORD,DWORD);
MMRESULT ACMAPI acmDriverAddW(LPHACMDRIVERID,HINSTANCE,LPARAM,DWORD,DWORD);
MMRESULT ACMAPI acmDriverRemove(HACMDRIVERID,DWORD);
MMRESULT ACMAPI acmDriverOpen(LPHACMDRIVER,HACMDRIVERID,DWORD);
MMRESULT ACMAPI acmDriverClose(HACMDRIVER,DWORD);
LRESULT ACMAPI acmDriverMessage(HACMDRIVER,UINT,LPARAM,LPARAM);
MMRESULT ACMAPI acmDriverPriority(HACMDRIVERID,DWORD,DWORD);
MMRESULT ACMAPI acmDriverDetailsA(HACMDRIVERID,LPACMDRIVERDETAILSA,DWORD);
MMRESULT ACMAPI acmDriverDetailsW(HACMDRIVERID,LPACMDRIVERDETAILSW,DWORD);
MMRESULT ACMAPI acmFormatTagDetailsA(HACMDRIVER,LPACMFORMATTAGDETAILSA,DWORD);
MMRESULT ACMAPI acmFormatTagDetailsW(HACMDRIVER,LPACMFORMATTAGDETAILSW,DWORD);
MMRESULT ACMAPI acmFormatTagEnumA(HACMDRIVER,LPACMFORMATTAGDETAILSA,ACMFORMATTAGENUMCBA,DWORD_PTR,DWORD);
MMRESULT ACMAPI acmFormatTagEnumW(HACMDRIVER,LPACMFORMATTAGDETAILSW,ACMFORMATTAGENUMCBW,DWORD_PTR,DWORD);
MMRESULT ACMAPI acmFormatDetailsA(HACMDRIVER,LPACMFORMATDETAILSA,DWORD);
MMRESULT ACMAPI acmFormatDetailsW(HACMDRIVER,LPACMFORMATDETAILSW,DWORD);
MMRESULT ACMAPI acmFormatEnumA(HACMDRIVER,LPACMFORMATDETAILSA,ACMFORMATENUMCBA,DWORD_PTR,DWORD);
MMRESULT ACMAPI acmFormatEnumW(HACMDRIVER,LPACMFORMATDETAILSW,ACMFORMATENUMCBW,DWORD_PTR,DWORD);
MMRESULT ACMAPI acmFormatSuggest(HACMDRIVER,LPWAVEFORMATEX,LPWAVEFORMATEX,DWORD,DWORD);
MMRESULT ACMAPI acmFormatChooseA(LPACMFORMATCHOOSEA);
MMRESULT ACMAPI acmFormatChooseW(LPACMFORMATCHOOSEW);
MMRESULT ACMAPI acmFilterTagDetailsA(HACMDRIVER,LPACMFILTERTAGDETAILSA,DWORD);
MMRESULT ACMAPI acmFilterTagDetailsW(HACMDRIVER,LPACMFILTERTAGDETAILSW,DWORD);
MMRESULT ACMAPI acmFilterTagEnumA(HACMDRIVER,LPACMFILTERTAGDETAILSA,ACMFILTERTAGENUMCBA,DWORD_PTR,DWORD);
MMRESULT ACMAPI acmFilterTagEnumW(HACMDRIVER,LPACMFILTERTAGDETAILSW,ACMFILTERTAGENUMCBW,DWORD_PTR,DWORD);
MMRESULT ACMAPI acmFilterDetailsA(HACMDRIVER,LPACMFILTERDETAILSA,DWORD);
MMRESULT ACMAPI acmFilterDetailsW(HACMDRIVER,LPACMFILTERDETAILSW,DWORD);
MMRESULT ACMAPI acmFilterEnumA(HACMDRIVER,LPACMFILTERDETAILSA,ACMFILTERENUMCBA,DWORD_PTR,DWORD);
MMRESULT ACMAPI acmFilterEnumW(HACMDRIVER,LPACMFILTERDETAILSW,ACMFILTERENUMCBW,DWORD_PTR,DWORD);
MMRESULT ACMAPI acmFilterChooseA(LPACMFILTERCHOOSEA);
MMRESULT ACMAPI acmFilterChooseW(LPACMFILTERCHOOSEW);
MMRESULT ACMAPI acmStreamOpen(LPHACMSTREAM,HACMDRIVER,LPWAVEFORMATEX,LPWAVEFORMATEX,LPWAVEFILTER,DWORD_PTR,DWORD_PTR,DWORD);
MMRESULT ACMAPI acmStreamClose(HACMSTREAM,DWORD);
MMRESULT ACMAPI acmStreamSize(HACMSTREAM,DWORD,LPDWORD,DWORD);
MMRESULT ACMAPI acmStreamReset(HACMSTREAM,DWORD);
MMRESULT ACMAPI acmStreamMessage(HACMSTREAM,UINT,LPARAM,LPARAM);
MMRESULT ACMAPI acmStreamConvert(HACMSTREAM,LPACMSTREAMHEADER,DWORD);
MMRESULT ACMAPI acmStreamPrepareHeader(HACMSTREAM,LPACMSTREAMHEADER,DWORD);
MMRESULT ACMAPI acmStreamUnprepareHeader(HACMSTREAM,LPACMSTREAMHEADER,DWORD);

#ifdef UNICODE
#define ACMDRIVERDETAILS ACMDRIVERDETAILSW
#define PACMDRIVERDETAILS PACMDRIVERDETAILSW
#define LPACMDRIVERDETAILS LPACMDRIVERDETAILSW
#define ACMFORMATTAGDETAILS ACMFORMATTAGDETAILSW
#define PACMFORMATTAGDETAILS PACMFORMATTAGDETAILSW
#define LPACMFORMATTAGDETAILS LPACMFORMATTAGDETAILSW
#define ACMFORMATTAGENUMCB ACMFORMATTAGENUMCBW
#define ACMFORMATDETAILS ACMFORMATDETAILSW
#define PACMFORMATDETAILS PACMFORMATDETAILSW
#define LPACMFORMATDETAILS LPACMFORMATDETAILSW
#define ACMFORMATENUMCB ACMFORMATENUMCBW
#define ACMHELPMSGSTRING ACMHELPMSGSTRINGW
#define ACMHELPMSGCONTEXTMENU ACMHELPMSGCONTEXTMENUW
#define ACMHELPMSGCONTEXTHELP ACMHELPMSGCONTEXTHELPW
#define ACMFORMATCHOOSEHOOKPROC ACMFORMATCHOOSEHOOKPROCW
#define ACMFORMATCHOOSE ACMFORMATCHOOSEW
#define PACMFORMATCHOOSE PACMFORMATCHOOSEW
#define LPACMFORMATCHOOSE LPACMFORMATCHOOSEW
#define ACMFILTERTAGDETAILS ACMFILTERTAGDETAILSW
#define PACMFILTERTAGDETAILS PACMFILTERTAGDETAILSW
#define LPACMFILTERTAGDETAILS LPACMFILTERTAGDETAILSW
#define ACMFILTERTAGENUMCB ACMFILTERTAGENUMCBW
#define ACMFILTERDETAILS ACMFILTERDETAILSW
#define PACMFILTERDETAILS PACMFILTERDETAILSW
#define LPACMFILTERDETAILS LPACMFILTERDETAILSW
#define ACMFILTERENUMCB ACMFILTERENUMCBW
#define ACMFILTERCHOOSEHOOKPROC ACMFILTERCHOOSEHOOKPROCW
#define ACMFILTERCHOOSE ACMFILTERCHOOSEW
#define PACMFILTERCHOOSE PACMFILTERCHOOSEW
#define LPACMFILTERCHOOSE LPACMFILTERCHOOSEW
#define acmDriverAdd acmDriverAddW
#define acmDriverDetails  acmDriverDetailsW
#define acmFormatTagDetails acmFormatTagDetailsW
#define acmFormatTagEnum acmFormatTagEnumW
#define acmFormatDetails acmFormatDetailsW
#define acmFormatEnum acmFormatEnumW
#define acmFormatChoose acmFormatChooseW
#define acmFilterTagDetails acmFilterTagDetailsW
#define acmFilterTagEnum acmFilterTagEnumW
#define acmFilterDetails acmFilterDetailsW
#define acmFilterEnum acmFilterEnumW
#define acmFilterChoose acmFilterChooseW
#else /* UNICODE */
#define ACMDRIVERDETAILS ACMDRIVERDETAILSA
#define PACMDRIVERDETAILS PACMDRIVERDETAILSA
#define LPACMDRIVERDETAILS LPACMDRIVERDETAILSA
#define ACMFORMATTAGDETAILS ACMFORMATTAGDETAILSA
#define PACMFORMATTAGDETAILS PACMFORMATTAGDETAILSA
#define LPACMFORMATTAGDETAILS LPACMFORMATTAGDETAILSA
#define ACMFORMATTAGENUMCB ACMFORMATTAGENUMCBA
#define ACMFORMATDETAILS ACMFORMATDETAILSA
#define PACMFORMATDETAILS PACMFORMATDETAILSA
#define LPACMFORMATDETAILS LPACMFORMATDETAILSA
#define ACMFORMATENUMCB ACMFORMATENUMCBA
#define ACMHELPMSGSTRING ACMHELPMSGSTRINGA
#define ACMHELPMSGCONTEXTMENU ACMHELPMSGCONTEXTMENUA
#define ACMHELPMSGCONTEXTHELP ACMHELPMSGCONTEXTHELPA
#define ACMFORMATCHOOSEHOOKPROC ACMFORMATCHOOSEHOOKPROCA
#define ACMFORMATCHOOSE ACMFORMATCHOOSEA
#define PACMFORMATCHOOSE PACMFORMATCHOOSEA
#define LPACMFORMATCHOOSE LPACMFORMATCHOOSEA
#define ACMFILTERTAGDETAILS ACMFILTERTAGDETAILSA
#define PACMFILTERTAGDETAILS PACMFILTERTAGDETAILSA
#define LPACMFILTERTAGDETAILS LPACMFILTERTAGDETAILSA
#define ACMFILTERTAGENUMCB ACMFILTERTAGENUMCBA
#define ACMFILTERDETAILS ACMFILTERDETAILSA
#define PACMFILTERDETAILS PACMFILTERDETAILSA
#define LPACMFILTERDETAILS LPACMFILTERDETAILSA
#define ACMFILTERENUMCB ACMFILTERENUMCBA
#define ACMFILTERCHOOSEHOOKPROC ACMFILTERCHOOSEHOOKPROCA
#define ACMFILTERCHOOSE ACMFILTERCHOOSEA
#define PACMFILTERCHOOSE PACMFILTERCHOOSEA
#define LPACMFILTERCHOOSE LPACMFILTERCHOOSEA
#define acmDriverAdd acmDriverAddA
#define acmDriverDetails  acmDriverDetailsA
#define acmFormatTagDetails acmFormatTagDetailsA
#define acmFormatTagEnum acmFormatTagEnumA
#define acmFormatDetails acmFormatDetailsA
#define acmFormatEnum acmFormatEnumA
#define acmFormatChoose acmFormatChooseA
#define acmFilterTagDetails acmFilterTagDetailsA
#define acmFilterTagEnum acmFilterTagEnumA
#define acmFilterDetails acmFilterDetailsA
#define acmFilterEnum acmFilterEnumA
#define acmFilterChoose acmFilterChooseA
#endif /* UNICODE */

#include "poppack.h"

#if __POCC__ >= 290
#pragma warn(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _MSACM_H */
