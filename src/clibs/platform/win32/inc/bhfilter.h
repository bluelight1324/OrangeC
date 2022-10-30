#ifndef _BHFILTER_H
#define _BHFILTER_H

#ifdef __ORANGEC__ 
#pragma once
#endif

/* Microsoft Network Monitor Display Filtering definitions */

#ifdef __cplusplus
extern "C" {
#endif

#if __POCC__ >= 290
#pragma warn(push)
#pragma warn(disable:2198)  /* Nameless field is not standard */
#endif

#define PROTOCOL_NUM_ANY  (-1)

#define FilterGetUIInstanceData(hfilt)  (((LPFILTERDESC)hfilt)->UIInstanceData)
#define FilterSetUIInstanceData(hfilt,inst)  (((LPFILTERDESC)hfilt)->UIInstanceData = (LPVOID)inst)

#define FILTERFREEPOOLSTART 20

#define INVALIDELEMENT -1
#define INVALIDVALUE ((VALUETYPE) -9999)

#define FILTER_FAIL_WITH_ERROR  -1
#define FILTER_PASSED TRUE
#define FILTER_FAILED FALSE

#define FILTERACTION_INVALID  0
#define FILTERACTION_PROPERTY  1
#define FILTERACTION_VALUE  2
#define FILTERACTION_STRING  3
#define FILTERACTION_ARRAY  4
#define FILTERACTION_AND  5
#define FILTERACTION_OR  6
#define FILTERACTION_XOR  7
#define FILTERACTION_PROPERTYEXIST  8
#define FILTERACTION_CONTAINSNC  9
#define FILTERACTION_CONTAINS  10
#define FILTERACTION_NOT  11
#define FILTERACTION_EQUALNC  12
#define FILTERACTION_EQUAL  13
#define FILTERACTION_NOTEQUALNC  14
#define FILTERACTION_NOTEQUAL  15
#define FILTERACTION_GREATERNC  16
#define FILTERACTION_GREATER  17
#define FILTERACTION_LESSNC  18
#define FILTERACTION_LESS  19
#define FILTERACTION_GREATEREQUALNC  20
#define FILTERACTION_GREATEREQUAL  21
#define FILTERACTION_LESSEQUALNC  22
#define FILTERACTION_LESSEQUAL  23
#define FILTERACTION_PLUS  24
#define FILTERACTION_MINUS  25
#define FILTERACTION_ADDRESS  26
#define FILTERACTION_ADDRESSANY  27
#define FILTERACTION_FROM  28
#define FILTERACTION_TO  29
#define FILTERACTION_FROMTO  30
#define FILTERACTION_AREBITSON  31
#define FILTERACTION_AREBITSOFF  32
#define FILTERACTION_PROTOCOLSEXIST  33
#define FILTERACTION_PROTOCOLEXIST  34
#define FILTERACTION_ARRAYEQUAL  35
#define FILTERACTION_DEREFPROPERTY  36
#define FILTERACTION_LARGEINT  37
#define FILTERACTION_TIME  38
#define FILTERACTION_ADDR_ETHER  39
#define FILTERACTION_ADDR_TOKEN  40
#define FILTERACTION_ADDR_FDDI  41
#define FILTERACTION_ADDR_IPX  42
#define FILTERACTION_ADDR_IP  43
#define FILTERACTION_OID  44
#define FILTERACTION_OID_CONTAINS  45
#define FILTERACTION_OID_BEGINS_WITH  46
#define FILTERACTION_OID_ENDS_WITH  47
#define FILTERACTION_ADDR_VINES  48

#define FILTERACTION_EXPRESSION  97
#define FILTERACTION_BOOL  98
#define FILTERACTION_NOEVAL  99

#define FILTER_NO_MORE_FRAMES  0xFFFFFFFF
#define FILTER_CANCELED  0xFFFFFFFE
#define FILTER_DIRECTION_NEXT  TRUE
#define FILTER_DIRECTION_PREV  FALSE

typedef HFILTER *LPHFILTER;

typedef DWORD FILTERACTIONTYPE;
typedef DWORD VALUETYPE;

typedef PROTOCOLTABLE PROTOCOLTABLETYPE;
typedef PROTOCOLTABLETYPE *LPPROTOCOLTABLETYPE;
typedef DWORD FILTERBITS, *LPFILTERBITS;
typedef SYSTEMTIME *LPTIME, UNALIGNED *ULPTIME;

typedef struct _FILTEROBJECT {
    FILTERACTIONTYPE Action;
    HPROPERTY hProperty;
    union {
        VALUETYPE Value;
        HPROTOCOL hProtocol;
        LPVOID lpArray;
        LPPROTOCOLTABLETYPE lpProtocolTable;
        LPADDRESS lpAddress;
        ULPLARGEINT lpLargeInt;
        ULPTIME lpTime;
        LPOBJECT_IDENTIFIER lpOID;
    };
    union {
        WORD ByteCount;
        WORD ByteOffset;
    };
    struct _FILTEROBJECT *pNext;
} FILTEROBJECT, *LPFILTEROBJECT;
#define FILTERINFO_SIZE  (sizeof(FILTEROBJECT))

typedef struct _FILTERDESC {
    WORD NumEntries;
    WORD Flags;
    LPFILTEROBJECT lpStack;
    LPFILTEROBJECT lpKeepLast;
    LPVOID UIInstanceData;
    LPFILTERBITS lpFilterBits;
    LPFILTERBITS lpCheckBits;
} FILTERDESC, *LPFILTERDESC;
#define FILTERDESC_SIZE  sizeof(FILTERDESC)

typedef BOOL(WINAPI *STATUSPROC)(DWORD,HCAPTURE,HFILTER,LPVOID);

extern HFILTER WINAPI CreateFilter(VOID);
extern DWORD WINAPI DestroyFilter(HFILTER);
extern HFILTER WINAPI FilterDuplicate(HFILTER);
extern DWORD WINAPI DisableParserFilter(HFILTER,HPARSER);
extern DWORD WINAPI EnableParserFilter(HFILTER,HPARSER);
extern DWORD WINAPI FilterAddObject(HFILTER,LPFILTEROBJECT);
extern VOID WINAPI FilterFlushBits(HFILTER);
extern DWORD WINAPI FilterFrame(HFRAME,HFILTER,HCAPTURE);
extern BOOL WINAPI FilterAttachesProperties(HFILTER);
DWORD WINAPI FilterFindFrame(HFILTER,HCAPTURE,DWORD,STATUSPROC,LPVOID,DWORD,BOOL);
HFRAME FilterFindPropertyInstance(HFRAME,HFILTER,HCAPTURE,HFILTER,LPPROPERTYINST*,STATUSPROC,LPVOID,DWORD,BOOL);
extern VOID WINAPI SetCurrentFilter(HFILTER);
extern HFILTER WINAPI GetCurrentFilter(VOID);

#if __POCC__ >= 290
#pragma warn(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _BHFILTER_H */
