#ifndef MSCAT_H
#define MSCAT_H

#ifdef __ORANGEC__ 
#pragma once
#endif

/* Microsoft Internet Security Catalog API */

/* #include <winapifamily.h> */

#include "mssip.h"

#define szOID_CATALOG_LIST  "1.3.6.1.4.1.311.12.1.1"
#define szOID_CATALOG_LIST_MEMBER  "1.3.6.1.4.1.311.12.1.2"
#define szOID_CATALOG_LIST_MEMBER2  "1.3.6.1.4.1.311.12.1.3"

#define CRYPTCAT_FILEEXT  L"CAT"

#define CRYPTCAT_MAX_MEMBERTAG  64

#define CRYPTCAT_VERSION_1  0x100
#define CRYPTCAT_VERSION_2  0x200

#define CRYPTCAT_OPEN_CREATENEW  0x00000001
#define CRYPTCAT_OPEN_ALWAYS  0x00000002
#define CRYPTCAT_OPEN_EXISTING  0x00000004

#define CRYPTCAT_OPEN_EXCLUDE_PAGE_HASHES  0x00010000

#define CRYPTCAT_OPEN_INCLUDE_PAGE_HASHES  0x00020000
#define CRYPTCAT_OPEN_VERIFYSIGHASH  0x10000000

#define CRYPTCAT_OPEN_NO_CONTENT_HCRYPTMSG  0x20000000
#define CRYPTCAT_OPEN_SORTED  0x40000000

#define CRYPTCAT_OPEN_FLAGS_MASK  0xffff0000

#define CRYPTCAT_MEMBER_SORTED  0x40000000

#define CRYPTCAT_ATTR_AUTHENTICATED  0x10000000
#define CRYPTCAT_ATTR_UNAUTHENTICATED  0x20000000

#define CRYPTCAT_ATTR_NAMEASCII  0x00000001
#define CRYPTCAT_ATTR_NAMEOBJID  0x00000002

#define CRYPTCAT_ATTR_DATAASCII  0x00010000
#define CRYPTCAT_ATTR_DATABASE64  0x00020000
#define CRYPTCAT_ATTR_DATAREPLACE  0x00040000

#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)
#define CRYPTCAT_ATTR_NO_AUTO_COMPAT_ENTRY  0x01000000
#endif /* NTDDI_VERSION >= NTDDI_WIN10_RS1 */

#define CRYPTCAT_E_AREA_HEADER  0x00000000
#define CRYPTCAT_E_AREA_MEMBER  0x00010000
#define CRYPTCAT_E_AREA_ATTRIBUTE  0x00020000

#define CRYPTCAT_E_CDF_UNSUPPORTED  0x00000001
#define CRYPTCAT_E_CDF_DUPLICATE  0x00000002
#define CRYPTCAT_E_CDF_TAGNOTFOUND  0x00000004

#define CRYPTCAT_E_CDF_MEMBER_FILE_PATH  0x00010001
#define CRYPTCAT_E_CDF_MEMBER_INDIRECTDATA  0x00010002
#define CRYPTCAT_E_CDF_MEMBER_FILENOTFOUND  0x00010004

#define CRYPTCAT_E_CDF_BAD_GUID_CONV  0x00020001
#define CRYPTCAT_E_CDF_ATTR_TOOFEWVALUES  0x00020002
#define CRYPTCAT_E_CDF_ATTR_TYPECOMBO  0x00020004

#if (NTDDI_VERSION >= NTDDI_WINBLUE)
#define CRYPTCAT_ADDCATALOG_NONE  0x00000000
#define CRYPTCAT_ADDCATALOG_HARDLINK  0x00000001
#endif /* NTDDI_VERSION >= NTDDI_WINBLUE */

#include <pshpack8.h>

typedef struct CRYPTCATSTORE_ {
    DWORD cbStruct;
    DWORD dwPublicVersion;
    LPWSTR pwszP7File;
    HCRYPTPROV hProv;
    DWORD dwEncodingType;
    DWORD fdwStoreFlags;
    HANDLE hReserved;
    HANDLE hAttrs;
    HCRYPTMSG hCryptMsg;
    HANDLE hSorted;
} CRYPTCATSTORE;

typedef struct CRYPTCATMEMBER_ {
    DWORD cbStruct;
    LPWSTR pwszReferenceTag;
    LPWSTR pwszFileName;
    GUID gSubjectType;
    DWORD fdwMemberFlags;
    struct SIP_INDIRECT_DATA_ *pIndirectData;
    DWORD dwCertVersion;
    DWORD dwReserved;
    HANDLE hReserved;
    CRYPT_ATTR_BLOB sEncodedIndirectData;
    CRYPT_ATTR_BLOB sEncodedMemberInfo;
} CRYPTCATMEMBER;

typedef struct CRYPTCATATTRIBUTE_ {
    DWORD cbStruct;
    LPWSTR pwszReferenceTag;
    DWORD dwAttrTypeAndAction;
    DWORD cbValue;
    BYTE *pbValue;
    DWORD dwReserved;
} CRYPTCATATTRIBUTE;

typedef struct CRYPTCATCDF_ {
    DWORD cbStruct;
    HANDLE hFile;
    DWORD dwCurFilePos;
    DWORD dwLastMemberOffset;
    BOOL fEOF;
    LPWSTR pwszResultDir;
    HANDLE hCATStore;
} CRYPTCATCDF;

typedef struct CATALOG_INFO_ {
    DWORD cbStruct;
    WCHAR wszCatalogFile[MAX_PATH];
} CATALOG_INFO;

typedef HANDLE HCATADMIN;
typedef HANDLE HCATINFO;

#include <poppack.h>

typedef void (WINAPI *PFN_CDF_PARSE_ERROR_CALLBACK)(DWORD, DWORD, WCHAR *);

extern HANDLE WINAPI CryptCATOpen(LPWSTR, DWORD, HCRYPTPROV, DWORD, DWORD);
extern void WINAPI CryptCATSetCreatePageHashesFlag(HANDLE, BOOL);
extern BOOL WINAPI CryptCATClose(HANDLE);
extern CRYPTCATSTORE *WINAPI CryptCATStoreFromHandle(HANDLE);
extern HANDLE WINAPI CryptCATHandleFromStore(CRYPTCATSTORE *);
extern BOOL WINAPI CryptCATPersistStore(HANDLE);
extern CRYPTCATATTRIBUTE *WINAPI CryptCATGetCatAttrInfo(HANDLE, LPWSTR);
extern CRYPTCATATTRIBUTE *WINAPI CryptCATPutCatAttrInfo(HANDLE, LPWSTR, DWORD, DWORD, BYTE *);
extern CRYPTCATATTRIBUTE *WINAPI CryptCATEnumerateCatAttr(HANDLE, CRYPTCATATTRIBUTE *);
extern CRYPTCATMEMBER *WINAPI CryptCATGetMemberInfo(HANDLE, LPWSTR);
extern CRYPTCATMEMBER *WINAPI CryptCATAllocSortedMemberInfo(HANDLE, LPWSTR);
extern void WINAPI CryptCATFreeSortedMemberInfo(HANDLE, CRYPTCATMEMBER *);
extern CRYPTCATATTRIBUTE *WINAPI CryptCATGetAttrInfo(HANDLE, CRYPTCATMEMBER *, LPWSTR);
extern CRYPTCATMEMBER *WINAPI CryptCATPutMemberInfo(HANDLE, LPWSTR, LPWSTR, GUID *, DWORD, DWORD, BYTE *);
extern CRYPTCATATTRIBUTE *WINAPI CryptCATPutAttrInfo(HANDLE, CRYPTCATMEMBER *, LPWSTR, DWORD, DWORD, BYTE *);
extern CRYPTCATMEMBER *WINAPI CryptCATEnumerateMember(HANDLE, CRYPTCATMEMBER *);
extern CRYPTCATATTRIBUTE *WINAPI CryptCATEnumerateAttr(HANDLE, CRYPTCATMEMBER *, CRYPTCATATTRIBUTE *);
extern CRYPTCATCDF *WINAPI CryptCATCDFOpen(LPWSTR, PFN_CDF_PARSE_ERROR_CALLBACK);
extern BOOL WINAPI CryptCATCDFClose(CRYPTCATCDF *);
extern CRYPTCATATTRIBUTE *WINAPI CryptCATCDFEnumCatAttributes(CRYPTCATCDF *, CRYPTCATATTRIBUTE *, PFN_CDF_PARSE_ERROR_CALLBACK);
extern CRYPTCATMEMBER *WINAPI CryptCATCDFEnumMembers(CRYPTCATCDF *, CRYPTCATMEMBER *, PFN_CDF_PARSE_ERROR_CALLBACK);
extern CRYPTCATATTRIBUTE *WINAPI CryptCATCDFEnumAttributes(CRYPTCATCDF *, CRYPTCATMEMBER *, CRYPTCATATTRIBUTE *, PFN_CDF_PARSE_ERROR_CALLBACK);
extern BOOL WINAPI IsCatalogFile(HANDLE, WCHAR *);
extern BOOL WINAPI CryptCATAdminAcquireContext(HCATADMIN *, const GUID *, DWORD);

#if (NTDDI_VERSION >= NTDDI_WIN8) 
extern BOOL WINAPI CryptCATAdminAcquireContext2(HCATADMIN *, const GUID *, PCWSTR , PCCERT_STRONG_SIGN_PARA , DWORD );
#endif /* NTDDI_VERSION >= NTDDI_WIN8 */

extern BOOL WINAPI CryptCATAdminReleaseContext(HCATADMIN, DWORD);
extern BOOL WINAPI CryptCATAdminReleaseCatalogContext(HCATADMIN, HCATINFO, DWORD);
extern HCATINFO WINAPI CryptCATAdminEnumCatalogFromHash(HCATADMIN, BYTE *, DWORD, DWORD, HCATINFO *);
extern BOOL WINAPI CryptCATAdminCalcHashFromFileHandle(HANDLE, DWORD *, BYTE *, DWORD);

#if (NTDDI_VERSION >= NTDDI_WIN8)
extern BOOL WINAPI CryptCATAdminCalcHashFromFileHandle2(HCATADMIN, HANDLE, DWORD *, BYTE *, DWORD);
#endif /* NTDDI_VERSION >= NTDDI_WIN8 */

extern HCATINFO WINAPI CryptCATAdminAddCatalog(HCATADMIN, PWSTR, PWSTR, DWORD);
extern BOOL WINAPI CryptCATAdminRemoveCatalog(HCATADMIN, LPCWSTR, DWORD);
extern BOOL WINAPI CryptCATCatalogInfoFromContext(HCATINFO, CATALOG_INFO *, DWORD);
extern BOOL WINAPI CryptCATAdminResolveCatalogPath(HCATADMIN, WCHAR *, CATALOG_INFO *, DWORD);
extern BOOL WINAPI CryptCATAdminPauseServiceForBackup(DWORD, BOOL);

#endif /* MSCAT_H */
