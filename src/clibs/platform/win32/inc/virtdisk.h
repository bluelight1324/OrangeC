#ifndef _VIRTDISK_H
#define _VIRTDISK_H

#ifdef __ORANGEC__ 
#pragma once
#endif

/* Virtual disk user mode interface */

#if ((WINVER >= _WIN32_WINNT_WIN7) || defined(VIRTDISK_DEFINE_FLAGS))

#ifdef __cplusplus
extern "C" {
#endif

#if __POCC__ >= 290
#pragma warn(push)
#pragma warn(disable:2014)  /* Empty declaration */
#pragma warn(disable:2198)  /* Nameless field is not standard */
#endif

#ifndef _VIRTUAL_STORAGE_TYPE_DEFINED
#define _VIRTUAL_STORAGE_TYPE_DEFINED
typedef struct _VIRTUAL_STORAGE_TYPE {
    ULONG DeviceId;
    GUID VendorId;
} VIRTUAL_STORAGE_TYPE, *PVIRTUAL_STORAGE_TYPE;
#endif /* _VIRTUAL_STORAGE_TYPE_DEFINED */

#ifdef DEFINE_GUID
DEFINE_GUID(VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN, 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
DEFINE_GUID(VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT, 0xec984aec, 0xa0f9, 0x47e9, 0x90, 0x1f, 0x71, 0x41, 0x5a, 0x66, 0x34, 0x5b);
#endif /* DEFINE_GUID */

#define VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN  0
#define VIRTUAL_STORAGE_TYPE_DEVICE_ISO  1
#define VIRTUAL_STORAGE_TYPE_DEVICE_VHD  2
#define VIRTUAL_STORAGE_TYPE_DEVICE_VHDX  3
#define VIRTUAL_STORAGE_TYPE_DEVICE_VHDSET  4

#ifndef VIRTDISK_DEFINE_FLAGS

#define OPEN_VIRTUAL_DISK_RW_DEPTH_DEFAULT  1

typedef enum _OPEN_VIRTUAL_DISK_VERSION {
    OPEN_VIRTUAL_DISK_VERSION_UNSPECIFIED = 0,
    OPEN_VIRTUAL_DISK_VERSION_1 = 1,
    OPEN_VIRTUAL_DISK_VERSION_2 = 2,
    OPEN_VIRTUAL_DISK_VERSION_3 = 3,
} OPEN_VIRTUAL_DISK_VERSION;

typedef struct _OPEN_VIRTUAL_DISK_PARAMETERS {
    OPEN_VIRTUAL_DISK_VERSION Version;
    union {
        struct {
            ULONG RWDepth;
        } Version1;
        struct {
            BOOL GetInfoOnly;
            BOOL ReadOnly;
            GUID ResiliencyGuid;
        } Version2;
        struct {
            BOOL GetInfoOnly;
            BOOL ReadOnly;
            GUID ResiliencyGuid;
            GUID SnapshotId;
        } Version3;
    };
} OPEN_VIRTUAL_DISK_PARAMETERS, *POPEN_VIRTUAL_DISK_PARAMETERS;

#endif /* VIRTDISK_DEFINE_FLAGS */

typedef enum _VIRTUAL_DISK_ACCESS_MASK {
    VIRTUAL_DISK_ACCESS_ATTACH_RO = 0x00010000,
    VIRTUAL_DISK_ACCESS_ATTACH_RW = 0x00020000,
    VIRTUAL_DISK_ACCESS_DETACH = 0x00040000,
    VIRTUAL_DISK_ACCESS_GET_INFO = 0x00080000,
    VIRTUAL_DISK_ACCESS_CREATE = 0x00100000,
    VIRTUAL_DISK_ACCESS_METAOPS = 0x00200000,
    VIRTUAL_DISK_ACCESS_READ = 0x000d0000,
    VIRTUAL_DISK_ACCESS_ALL = 0x003f0000,
    VIRTUAL_DISK_ACCESS_WRITABLE = 0x00320000
} VIRTUAL_DISK_ACCESS_MASK;

/* DEFINE_ENUM_FLAG_OPERATORS(VIRTUAL_DISK_ACCESS_MASK); */

typedef enum _OPEN_VIRTUAL_DISK_FLAG {
    OPEN_VIRTUAL_DISK_FLAG_NONE = 0x00000000,
    OPEN_VIRTUAL_DISK_FLAG_NO_PARENTS = 0x00000001,
    OPEN_VIRTUAL_DISK_FLAG_BLANK_FILE = 0x00000002,
    OPEN_VIRTUAL_DISK_FLAG_BOOT_DRIVE = 0x00000004,
    OPEN_VIRTUAL_DISK_FLAG_CACHED_IO = 0x00000008,
    OPEN_VIRTUAL_DISK_FLAG_CUSTOM_DIFF_CHAIN = 0x00000010,
    OPEN_VIRTUAL_DISK_FLAG_PARENT_CACHED_IO = 0x00000020,
    OPEN_VIRTUAL_DISK_FLAG_VHDSET_FILE_ONLY = 0x00000040,
    OPEN_VIRTUAL_DISK_FLAG_IGNORE_RELATIVE_PARENT_LOCATOR = 0x00000080,
    OPEN_VIRTUAL_DISK_FLAG_NO_WRITE_HARDENING = 0x00000100,
} OPEN_VIRTUAL_DISK_FLAG;

#ifndef VIRTDISK_DEFINE_FLAGS

/* DEFINE_ENUM_FLAG_OPERATORS(OPEN_VIRTUAL_DISK_FLAG); */

#define CREATE_VIRTUAL_DISK_PARAMETERS_DEFAULT_BLOCK_SIZE  0
#define CREATE_VIRTUAL_DISK_PARAMETERS_DEFAULT_SECTOR_SIZE  0

typedef enum _CREATE_VIRTUAL_DISK_VERSION {
    CREATE_VIRTUAL_DISK_VERSION_UNSPECIFIED = 0,
    CREATE_VIRTUAL_DISK_VERSION_1 = 1,
    CREATE_VIRTUAL_DISK_VERSION_2 = 2,
    CREATE_VIRTUAL_DISK_VERSION_3 = 3,
} CREATE_VIRTUAL_DISK_VERSION;

typedef struct _CREATE_VIRTUAL_DISK_PARAMETERS {
    CREATE_VIRTUAL_DISK_VERSION Version;
    union {
        struct {
            GUID UniqueId;
            ULONGLONG MaximumSize;
            ULONG BlockSizeInBytes;
            ULONG SectorSizeInBytes;
            PCWSTR ParentPath;
            PCWSTR SourcePath;
        } Version1;
        struct {
            GUID UniqueId;
            ULONGLONG MaximumSize;
            ULONG BlockSizeInBytes;
            ULONG SectorSizeInBytes;
            ULONG PhysicalSectorSizeInBytes;
            PCWSTR ParentPath;
            PCWSTR SourcePath;
            OPEN_VIRTUAL_DISK_FLAG OpenFlags;
            VIRTUAL_STORAGE_TYPE ParentVirtualStorageType;
            VIRTUAL_STORAGE_TYPE SourceVirtualStorageType;
            GUID ResiliencyGuid;
        } Version2;
        struct {
            GUID UniqueId;
            ULONGLONG MaximumSize;
            ULONG BlockSizeInBytes;
            ULONG SectorSizeInBytes;
            ULONG PhysicalSectorSizeInBytes;
            PCWSTR ParentPath;
            PCWSTR SourcePath;
            OPEN_VIRTUAL_DISK_FLAG OpenFlags;
            VIRTUAL_STORAGE_TYPE ParentVirtualStorageType;
            VIRTUAL_STORAGE_TYPE SourceVirtualStorageType;
            GUID ResiliencyGuid;
            PCWSTR SourceLimitPath;
            VIRTUAL_STORAGE_TYPE BackingStorageType;
        } Version3;
    };
} CREATE_VIRTUAL_DISK_PARAMETERS, *PCREATE_VIRTUAL_DISK_PARAMETERS;

typedef enum _CREATE_VIRTUAL_DISK_FLAG {
    CREATE_VIRTUAL_DISK_FLAG_NONE = 0x0,
    CREATE_VIRTUAL_DISK_FLAG_FULL_PHYSICAL_ALLOCATION = 0x1,
    CREATE_VIRTUAL_DISK_FLAG_PREVENT_WRITES_TO_SOURCE_DISK = 0x2,
    CREATE_VIRTUAL_DISK_FLAG_DO_NOT_COPY_METADATA_FROM_PARENT = 0x4,
    CREATE_VIRTUAL_DISK_FLAG_CREATE_BACKING_STORAGE = 0x8,
    CREATE_VIRTUAL_DISK_FLAG_USE_CHANGE_TRACKING_SOURCE_LIMIT = 0x10,
    CREATE_VIRTUAL_DISK_FLAG_PRESERVE_PARENT_CHANGE_TRACKING_STATE = 0x20,
    CREATE_VIRTUAL_DISK_FLAG_VHD_SET_USE_ORIGINAL_BACKING_STORAGE = 0x40,
} CREATE_VIRTUAL_DISK_FLAG;

#define CREATE_VIRTUAL_DISK_FLAG_USE_RCT_SOURCE_LIMIT CREATE_VIRTUAL_DISK_FLAG_USE_CHANGE_TRACKING_SOURCE_LIMIT

/* DEFINE_ENUM_FLAG_OPERATORS(CREATE_VIRTUAL_DISK_FLAG); */

typedef enum _ATTACH_VIRTUAL_DISK_VERSION {
    ATTACH_VIRTUAL_DISK_VERSION_UNSPECIFIED = 0,
    ATTACH_VIRTUAL_DISK_VERSION_1 = 1,
} ATTACH_VIRTUAL_DISK_VERSION;

typedef struct _ATTACH_VIRTUAL_DISK_PARAMETERS {
    ATTACH_VIRTUAL_DISK_VERSION Version;
    union {
        struct {
            ULONG Reserved;
        } Version1;
    };
} ATTACH_VIRTUAL_DISK_PARAMETERS, *PATTACH_VIRTUAL_DISK_PARAMETERS;

DWORD WINAPI OpenVirtualDisk(PVIRTUAL_STORAGE_TYPE, PCWSTR, VIRTUAL_DISK_ACCESS_MASK, OPEN_VIRTUAL_DISK_FLAG, POPEN_VIRTUAL_DISK_PARAMETERS, PHANDLE);
DWORD WINAPI CreateVirtualDisk(PVIRTUAL_STORAGE_TYPE, PCWSTR, VIRTUAL_DISK_ACCESS_MASK, PSECURITY_DESCRIPTOR, CREATE_VIRTUAL_DISK_FLAG, ULONG, PCREATE_VIRTUAL_DISK_PARAMETERS, LPOVERLAPPED, PHANDLE);

#endif /* !VIRTDISK_DEFINE_FLAGS */

typedef enum _ATTACH_VIRTUAL_DISK_FLAG {
    ATTACH_VIRTUAL_DISK_FLAG_NONE = 0x00000000,
    ATTACH_VIRTUAL_DISK_FLAG_READ_ONLY = 0x00000001,
    ATTACH_VIRTUAL_DISK_FLAG_NO_DRIVE_LETTER = 0x00000002,
    ATTACH_VIRTUAL_DISK_FLAG_PERMANENT_LIFETIME = 0x00000004,
    ATTACH_VIRTUAL_DISK_FLAG_NO_LOCAL_HOST = 0x00000008,
    ATTACH_VIRTUAL_DISK_FLAG_NO_SECURITY_DESCRIPTOR = 0x00000010,
} ATTACH_VIRTUAL_DISK_FLAG;

#ifndef VIRTDISK_DEFINE_FLAGS

/* DEFINE_ENUM_FLAG_OPERATORS(ATTACH_VIRTUAL_DISK_FLAG); */

DWORD WINAPI AttachVirtualDisk(HANDLE, PSECURITY_DESCRIPTOR, ATTACH_VIRTUAL_DISK_FLAG, ULONG, PATTACH_VIRTUAL_DISK_PARAMETERS, LPOVERLAPPED);

#endif /* VIRTDISK_DEFINE_FLAGS */

typedef enum _DETACH_VIRTUAL_DISK_FLAG {
    DETACH_VIRTUAL_DISK_FLAG_NONE = 0x00000000,
} DETACH_VIRTUAL_DISK_FLAG;

#ifndef VIRTDISK_DEFINE_FLAGS

/* DEFINE_ENUM_FLAG_OPERATORS(DETACH_VIRTUAL_DISK_FLAG); */

DWORD WINAPI DetachVirtualDisk(HANDLE, DETACH_VIRTUAL_DISK_FLAG, ULONG);
DWORD WINAPI GetVirtualDiskPhysicalPath(HANDLE, PULONG, PWSTR);
DWORD WINAPI GetAllAttachedVirtualDiskPhysicalPaths(PULONG,PWSTR);
#endif /* !VIRTDISK_DEFINE_FLAGS */

typedef enum _DEPENDENT_DISK_FLAG {
    DEPENDENT_DISK_FLAG_NONE = 0x00000000,
    DEPENDENT_DISK_FLAG_MULT_BACKING_FILES = 0x00000001,
    DEPENDENT_DISK_FLAG_FULLY_ALLOCATED = 0x00000002,
    DEPENDENT_DISK_FLAG_READ_ONLY = 0x00000004,
    DEPENDENT_DISK_FLAG_REMOTE = 0x00000008,
    DEPENDENT_DISK_FLAG_SYSTEM_VOLUME = 0x00000010,
    DEPENDENT_DISK_FLAG_SYSTEM_VOLUME_PARENT = 0x00000020,
    DEPENDENT_DISK_FLAG_REMOVABLE = 0x00000040,
    DEPENDENT_DISK_FLAG_NO_DRIVE_LETTER = 0x00000080,
    DEPENDENT_DISK_FLAG_PARENT = 0x00000100,
    DEPENDENT_DISK_FLAG_NO_HOST_DISK = 0x00000200,
    DEPENDENT_DISK_FLAG_PERMANENT_LIFETIME = 0x00000400,
} DEPENDENT_DISK_FLAG;

/* DEFINE_ENUM_FLAG_OPERATORS(DEPENDENT_DISK_FLAG); */

#ifndef VIRTDISK_DEFINE_FLAGS

typedef enum _STORAGE_DEPENDENCY_INFO_VERSION {
    STORAGE_DEPENDENCY_INFO_VERSION_UNSPECIFIED = 0,
    STORAGE_DEPENDENCY_INFO_VERSION_1 = 1,
    STORAGE_DEPENDENCY_INFO_VERSION_2 = 2,
} STORAGE_DEPENDENCY_INFO_VERSION;

typedef struct _STORAGE_DEPENDENCY_INFO_TYPE_1 {
    DEPENDENT_DISK_FLAG DependencyTypeFlags;
    ULONG ProviderSpecificFlags;
    VIRTUAL_STORAGE_TYPE VirtualStorageType;
} STORAGE_DEPENDENCY_INFO_TYPE_1, *PSTORAGE_DEPENDENCY_INFO_TYPE_1;

typedef struct _STORAGE_DEPENDENCY_INFO_TYPE_2 {
    DEPENDENT_DISK_FLAG DependencyTypeFlags;
    ULONG ProviderSpecificFlags;
    VIRTUAL_STORAGE_TYPE VirtualStorageType;
    ULONG AncestorLevel;
    PWSTR DependencyDeviceName;
    PWSTR HostVolumeName;
    PWSTR DependentVolumeName;
    PWSTR DependentVolumeRelativePath;
} STORAGE_DEPENDENCY_INFO_TYPE_2, *PSTORAGE_DEPENDENCY_INFO_TYPE_2;

typedef struct _STORAGE_DEPENDENCY_INFO {
    STORAGE_DEPENDENCY_INFO_VERSION Version;
    ULONG NumberEntries;
    union {
        STORAGE_DEPENDENCY_INFO_TYPE_1 Version1Entries[];
        STORAGE_DEPENDENCY_INFO_TYPE_2 Version2Entries[];
    };
} STORAGE_DEPENDENCY_INFO, *PSTORAGE_DEPENDENCY_INFO;

#endif /* !VIRTDISK_DEFINE_FLAGS */

typedef enum _GET_STORAGE_DEPENDENCY_FLAG {
    GET_STORAGE_DEPENDENCY_FLAG_NONE = 0x00000000,
    GET_STORAGE_DEPENDENCY_FLAG_HOST_VOLUMES = 0x00000001,
    GET_STORAGE_DEPENDENCY_FLAG_DISK_HANDLE = 0x00000002,
} GET_STORAGE_DEPENDENCY_FLAG;

#define GET_STORAGE_DEPENDENCY_FLAG_PARENTS  GET_STORAGE_DEPENDENCY_FLAG_HOST_VOLUMES

/* DEFINE_ENUM_FLAG_OPERATORS(GET_STORAGE_DEPENDENCY_FLAG); */

#ifndef VIRTDISK_DEFINE_FLAGS

DWORD WINAPI GetStorageDependencyInformation(HANDLE, GET_STORAGE_DEPENDENCY_FLAG, ULONG, PSTORAGE_DEPENDENCY_INFO, PULONG);

typedef enum _GET_VIRTUAL_DISK_INFO_VERSION {
    GET_VIRTUAL_DISK_INFO_UNSPECIFIED = 0,
    GET_VIRTUAL_DISK_INFO_SIZE = 1,
    GET_VIRTUAL_DISK_INFO_IDENTIFIER = 2,
    GET_VIRTUAL_DISK_INFO_PARENT_LOCATION = 3,
    GET_VIRTUAL_DISK_INFO_PARENT_IDENTIFIER = 4,
    GET_VIRTUAL_DISK_INFO_PARENT_TIMESTAMP = 5,
    GET_VIRTUAL_DISK_INFO_VIRTUAL_STORAGE_TYPE = 6,
    GET_VIRTUAL_DISK_INFO_PROVIDER_SUBTYPE = 7,
    GET_VIRTUAL_DISK_INFO_IS_4K_ALIGNED = 8,
    GET_VIRTUAL_DISK_INFO_PHYSICAL_DISK = 9,
    GET_VIRTUAL_DISK_INFO_VHD_PHYSICAL_SECTOR_SIZE = 10,
    GET_VIRTUAL_DISK_INFO_SMALLEST_SAFE_VIRTUAL_SIZE = 11,
    GET_VIRTUAL_DISK_INFO_FRAGMENTATION = 12,
    GET_VIRTUAL_DISK_INFO_IS_LOADED = 13,
    GET_VIRTUAL_DISK_INFO_VIRTUAL_DISK_ID = 14,
    GET_VIRTUAL_DISK_INFO_CHANGE_TRACKING_STATE = 15,
} GET_VIRTUAL_DISK_INFO_VERSION;

typedef struct _GET_VIRTUAL_DISK_INFO {
    GET_VIRTUAL_DISK_INFO_VERSION Version;
    union {
        struct {
            ULONGLONG VirtualSize;
            ULONGLONG PhysicalSize;
            ULONG BlockSize;
            ULONG SectorSize;
        } Size;
        GUID Identifier;
        struct {
            BOOL ParentResolved;
            WCHAR ParentLocationBuffer[1];
        } ParentLocation;
        GUID ParentIdentifier;
        ULONG ParentTimestamp;
        VIRTUAL_STORAGE_TYPE VirtualStorageType;
        ULONG ProviderSubtype;
        BOOL Is4kAligned;
        BOOL IsLoaded;
        struct {
            ULONG LogicalSectorSize;
            ULONG PhysicalSectorSize;
            BOOL IsRemote;
        }
        PhysicalDisk;
        ULONG VhdPhysicalSectorSize;
        ULONGLONG SmallestSafeVirtualSize;
        ULONG FragmentationPercentage;
        GUID VirtualDiskId;
        struct {
            BOOL Enabled;
            BOOL NewerChanges;
            WCHAR MostRecentId[1];
        }
        ChangeTrackingState;
    };
} GET_VIRTUAL_DISK_INFO, *PGET_VIRTUAL_DISK_INFO;

#define VIRTUAL_DISK_MAXIMUM_CHANGE_TRACKING_ID_LENGTH  256

DWORD WINAPI GetVirtualDiskInformation(HANDLE, PULONG, PGET_VIRTUAL_DISK_INFO, PULONG);

typedef enum _SET_VIRTUAL_DISK_INFO_VERSION {
    SET_VIRTUAL_DISK_INFO_UNSPECIFIED = 0,
    SET_VIRTUAL_DISK_INFO_PARENT_PATH = 1,
    SET_VIRTUAL_DISK_INFO_IDENTIFIER = 2,
    SET_VIRTUAL_DISK_INFO_PARENT_PATH_WITH_DEPTH = 3,
    SET_VIRTUAL_DISK_INFO_PHYSICAL_SECTOR_SIZE = 4,
    SET_VIRTUAL_DISK_INFO_VIRTUAL_DISK_ID = 5,
    SET_VIRTUAL_DISK_INFO_CHANGE_TRACKING_STATE = 6,
    SET_VIRTUAL_DISK_INFO_PARENT_LOCATOR = 7,
} SET_VIRTUAL_DISK_INFO_VERSION;

typedef struct _SET_VIRTUAL_DISK_INFO {
    SET_VIRTUAL_DISK_INFO_VERSION Version;
    union {
        PCWSTR ParentFilePath;
        GUID UniqueIdentifier;
        struct {
            ULONG ChildDepth;
            PCWSTR ParentFilePath;
        }
        ParentPathWithDepthInfo;
        ULONG VhdPhysicalSectorSize;
        GUID VirtualDiskId;
        BOOL ChangeTrackingEnabled;
        struct {
            GUID LinkageId;
            PCWSTR ParentFilePath;
        }
        ParentLocator;
    };
} SET_VIRTUAL_DISK_INFO, *PSET_VIRTUAL_DISK_INFO;


DWORD WINAPI SetVirtualDiskInformation(HANDLE, PSET_VIRTUAL_DISK_INFO);

#if (NTDDI_VERSION >= NTDDI_WIN8)
DWORD WINAPI EnumerateVirtualDiskMetadata(HANDLE, PULONG, GUID *);
DWORD WINAPI GetVirtualDiskMetadata(HANDLE, const GUID *, PULONG, PVOID);
DWORD WINAPI SetVirtualDiskMetadata(HANDLE, const GUID *, ULONG, const void *);
DWORD WINAPI DeleteVirtualDiskMetadata(HANDLE, const GUID *);
#endif /* NTDDI_VERSION >= NTDDI_WIN8 */

typedef struct _VIRTUAL_DISK_PROGRESS {
    DWORD OperationStatus;
    ULONGLONG CurrentValue;
    ULONGLONG CompletionValue;
} VIRTUAL_DISK_PROGRESS, *PVIRTUAL_DISK_PROGRESS;

DWORD WINAPI GetVirtualDiskOperationProgress(HANDLE, LPOVERLAPPED, PVIRTUAL_DISK_PROGRESS);

typedef enum _COMPACT_VIRTUAL_DISK_VERSION {
    COMPACT_VIRTUAL_DISK_VERSION_UNSPECIFIED = 0,
    COMPACT_VIRTUAL_DISK_VERSION_1 = 1,
} COMPACT_VIRTUAL_DISK_VERSION;

typedef struct _COMPACT_VIRTUAL_DISK_PARAMETERS {
    COMPACT_VIRTUAL_DISK_VERSION Version;
    union {
        struct {
            ULONG Reserved;
        } Version1;
    };
} COMPACT_VIRTUAL_DISK_PARAMETERS, *PCOMPACT_VIRTUAL_DISK_PARAMETERS;

typedef enum _COMPACT_VIRTUAL_DISK_FLAG {
    COMPACT_VIRTUAL_DISK_FLAG_NONE = 0x00000000,
    COMPACT_VIRTUAL_DISK_FLAG_NO_ZERO_SCAN = 0x00000001,
    COMPACT_VIRTUAL_DISK_FLAG_NO_BLOCK_MOVES = 0x00000002,
} COMPACT_VIRTUAL_DISK_FLAG;

/* DEFINE_ENUM_FLAG_OPERATORS(COMPACT_VIRTUAL_DISK_FLAG); */

DWORD WINAPI CompactVirtualDisk(HANDLE, COMPACT_VIRTUAL_DISK_FLAG, PCOMPACT_VIRTUAL_DISK_PARAMETERS, LPOVERLAPPED);

typedef enum _MERGE_VIRTUAL_DISK_VERSION {
    MERGE_VIRTUAL_DISK_VERSION_UNSPECIFIED = 0,
    MERGE_VIRTUAL_DISK_VERSION_1 = 1,
    MERGE_VIRTUAL_DISK_VERSION_2 = 2,
} MERGE_VIRTUAL_DISK_VERSION;

#define MERGE_VIRTUAL_DISK_DEFAULT_MERGE_DEPTH  1

typedef struct _MERGE_VIRTUAL_DISK_PARAMETERS {
    MERGE_VIRTUAL_DISK_VERSION Version;
    union {
        struct {
            ULONG MergeDepth;
        } Version1;
        struct {
            ULONG MergeSourceDepth;
            ULONG MergeTargetDepth;
        } Version2;
    };
} MERGE_VIRTUAL_DISK_PARAMETERS, *PMERGE_VIRTUAL_DISK_PARAMETERS;

typedef enum _MERGE_VIRTUAL_DISK_FLAG {
    MERGE_VIRTUAL_DISK_FLAG_NONE = 0x00000000,
} MERGE_VIRTUAL_DISK_FLAG;

/* DEFINE_ENUM_FLAG_OPERATORS(MERGE_VIRTUAL_DISK_FLAG); */

DWORD WINAPI MergeVirtualDisk(HANDLE, MERGE_VIRTUAL_DISK_FLAG, PMERGE_VIRTUAL_DISK_PARAMETERS, LPOVERLAPPED);

typedef enum _EXPAND_VIRTUAL_DISK_VERSION {
    EXPAND_VIRTUAL_DISK_VERSION_UNSPECIFIED = 0,
    EXPAND_VIRTUAL_DISK_VERSION_1 = 1,
} EXPAND_VIRTUAL_DISK_VERSION;

typedef struct _EXPAND_VIRTUAL_DISK_PARAMETERS {
    EXPAND_VIRTUAL_DISK_VERSION Version;
    union {
        struct {
            ULONGLONG NewSize;
        } Version1;
    };
} EXPAND_VIRTUAL_DISK_PARAMETERS, *PEXPAND_VIRTUAL_DISK_PARAMETERS;

typedef enum _EXPAND_VIRTUAL_DISK_FLAG {
    EXPAND_VIRTUAL_DISK_FLAG_NONE = 0x00000000,
} EXPAND_VIRTUAL_DISK_FLAG;

/* DEFINE_ENUM_FLAG_OPERATORS(EXPAND_VIRTUAL_DISK_FLAG); */

DWORD WINAPI ExpandVirtualDisk(HANDLE, EXPAND_VIRTUAL_DISK_FLAG, PEXPAND_VIRTUAL_DISK_PARAMETERS, LPOVERLAPPED);

typedef enum _RESIZE_VIRTUAL_DISK_VERSION {
    RESIZE_VIRTUAL_DISK_VERSION_UNSPECIFIED = 0,
    RESIZE_VIRTUAL_DISK_VERSION_1 = 1,
} RESIZE_VIRTUAL_DISK_VERSION;

typedef struct _RESIZE_VIRTUAL_DISK_PARAMETERS {
    RESIZE_VIRTUAL_DISK_VERSION Version;
    union {
        struct {
            ULONGLONG NewSize;
        } Version1;
    };
} RESIZE_VIRTUAL_DISK_PARAMETERS , *PRESIZE_VIRTUAL_DISK_PARAMETERS;

typedef enum _RESIZE_VIRTUAL_DISK_FLAG {
    RESIZE_VIRTUAL_DISK_FLAG_NONE = 0x0,
    RESIZE_VIRTUAL_DISK_FLAG_ALLOW_UNSAFE_VIRTUAL_SIZE = 0x1,
    RESIZE_VIRTUAL_DISK_FLAG_RESIZE_TO_SMALLEST_SAFE_VIRTUAL_SIZE = 0x2,
} RESIZE_VIRTUAL_DISK_FLAG;

/* DEFINE_ENUM_FLAG_OPERATORS(RESIZE_VIRTUAL_DISK_FLAG); */

#if (NTDDI_VERSION >= NTDDI_WIN8)
DWORD WINAPI ResizeVirtualDisk(HANDLE, RESIZE_VIRTUAL_DISK_FLAG, PRESIZE_VIRTUAL_DISK_PARAMETERS, LPOVERLAPPED);
#endif /* NTDDI_VERSION >= NTDDI_WIN8 */

typedef enum _MIRROR_VIRTUAL_DISK_VERSION {
    MIRROR_VIRTUAL_DISK_VERSION_UNSPECIFIED = 0,
    MIRROR_VIRTUAL_DISK_VERSION_1 = 1,
} MIRROR_VIRTUAL_DISK_VERSION;

typedef struct _MIRROR_VIRTUAL_DISK_PARAMETERS {
    MIRROR_VIRTUAL_DISK_VERSION Version;
    union {
        struct {
            PCWSTR MirrorVirtualDiskPath;
        } Version1;
    };
} MIRROR_VIRTUAL_DISK_PARAMETERS , *PMIRROR_VIRTUAL_DISK_PARAMETERS;

typedef enum _MIRROR_VIRTUAL_DISK_FLAG {
    MIRROR_VIRTUAL_DISK_FLAG_NONE = 0x00000000,
    MIRROR_VIRTUAL_DISK_FLAG_EXISTING_FILE = 0x00000001,
} MIRROR_VIRTUAL_DISK_FLAG;

/* DEFINE_ENUM_FLAG_OPERATORS(MIRROR_VIRTUAL_DISK_FLAG); */

#if (NTDDI_VERSION >= NTDDI_WIN8)
DWORD WINAPI MirrorVirtualDisk(HANDLE, MIRROR_VIRTUAL_DISK_FLAG, PMIRROR_VIRTUAL_DISK_PARAMETERS, LPOVERLAPPED);
DWORD WINAPI BreakMirrorVirtualDisk(HANDLE);
DWORD WINAPI AddVirtualDiskParent(HANDLE, PCWSTR);
#endif // NTDDI_VERSION >= NTDDI_WIN8

typedef struct _QUERY_CHANGES_VIRTUAL_DISK_RANGE {
    ULONG64 ByteOffset;
    ULONG64 ByteLength;
    ULONG64 Reserved;
} QUERY_CHANGES_VIRTUAL_DISK_RANGE, *PQUERY_CHANGES_VIRTUAL_DISK_RANGE;

typedef enum _QUERY_CHANGES_VIRTUAL_DISK_FLAG {
    QUERY_CHANGES_VIRTUAL_DISK_FLAG_NONE = 0x00000000,
} QUERY_CHANGES_VIRTUAL_DISK_FLAG;

/* DEFINE_ENUM_FLAG_OPERATORS(QUERY_CHANGES_VIRTUAL_DISK_FLAG); */

typedef enum _TAKE_SNAPSHOT_VHDSET_FLAG {
    TAKE_SNAPSHOT_VHDSET_FLAG_NONE = 0x00000000,
    TAKE_SNAPSHOT_VHDSET_FLAG_WRITEABLE = 0x00000001,
} TAKE_SNAPSHOT_VHDSET_FLAG, *PTAKE_SNAPSHOT_VHDSET_FLAG;

/* DEFINE_ENUM_FLAG_OPERATORS(TAKE_SNAPSHOT_VHDSET_FLAG); */

typedef enum _TAKE_SNAPSHOT_VHDSET_VERSION {
    TAKE_SNAPSHOT_VHDSET_VERSION_UNSPECIFIED = 0,
    TAKE_SNAPSHOT_VHDSET_VERSION_1 = 1,
} TAKE_SNAPSHOT_VHDSET_VERSION;

typedef struct _TAKE_SNAPSHOT_VHDSET_PARAMETERS {
    TAKE_SNAPSHOT_VHDSET_VERSION Version;
    union {
        struct {
            GUID SnapshotId;
        } Version1;
    };
} TAKE_SNAPSHOT_VHDSET_PARAMETERS, *PTAKE_SNAPSHOT_VHDSET_PARAMETERS;

typedef enum _DELETE_SNAPSHOT_VHDSET_FLAG {
    DELETE_SNAPSHOT_VHDSET_FLAG_NONE = 0x00000000,
    DELETE_SNAPSHOT_VHDSET_FLAG_PERSIST_RCT = 0x00000001,
} DELETE_SNAPSHOT_VHDSET_FLAG, *PDELETE_SNAPSHOT_VHDSET_FLAG;

/* DEFINE_ENUM_FLAG_OPERATORS(DELETE_SNAPSHOT_VHDSET_FLAG); */

typedef enum _DELETE_SNAPSHOT_VHDSET_VERSION {
    DELETE_SNAPSHOT_VHDSET_VERSION_UNSPECIFIED = 0,
    DELETE_SNAPSHOT_VHDSET_VERSION_1 = 1,
} DELETE_SNAPSHOT_VHDSET_VERSION;

typedef struct _DELETE_SNAPSHOT_VHDSET_PARAMETERS {
    DELETE_SNAPSHOT_VHDSET_VERSION Version;
    union {
        struct {
            GUID SnapshotId;
        } Version1;
    };
} DELETE_SNAPSHOT_VHDSET_PARAMETERS, *PDELETE_SNAPSHOT_VHDSET_PARAMETERS;

typedef enum _MODIFY_VHDSET_VERSION {
    MODIFY_VHDSET_UNSPECIFIED = 0,
    MODIFY_VHDSET_SNAPSHOT_PATH = 1,
    MODIFY_VHDSET_REMOVE_SNAPSHOT = 2,
    MODIFY_VHDSET_DEFAULT_SNAPSHOT_PATH = 3,
} MODIFY_VHDSET_VERSION, *PMODIFY_VHDSET_VERSION;

typedef enum _MODIFY_VHDSET_FLAG {
    MODIFY_VHDSET_FLAG_NONE = 0x00000000,
    MODIFY_VHDSET_FLAG_WRITEABLE_SNAPSHOT = 0x00000001,
} MODIFY_VHDSET_FLAG, *PMODIFY_VHDSET_FLAG;

/* DEFINE_ENUM_FLAG_OPERATORS(MODIFY_VHDSET_FLAG); */

typedef struct _MODIFY_VHDSET_PARAMETERS {
    MODIFY_VHDSET_VERSION Version;
    union {
        struct{
            GUID SnapshotId;
            PCWSTR SnapshotFilePath;
        } SnapshotPath;
        GUID SnapshotId;
        PCWSTR DefaultFilePath;
    };
} MODIFY_VHDSET_PARAMETERS, *PMODIFY_VHDSET_PARAMETERS;

typedef enum _APPLY_SNAPSHOT_VHDSET_FLAG {
    APPLY_SNAPSHOT_VHDSET_FLAG_NONE = 0x00000000,
    APPLY_SNAPSHOT_VHDSET_FLAG_WRITEABLE = 0x00000001,
} APPLY_SNAPSHOT_VHDSET_FLAG, *PAPPLY_SNAPSHOT_VHDSET_FLAG;

/* DEFINE_ENUM_FLAG_OPERATORS(APPLY_SNAPSHOT_VHDSET_FLAG); */

typedef enum _APPLY_SNAPSHOT_VHDSET_VERSION {
    APPLY_SNAPSHOT_VHDSET_VERSION_UNSPECIFIED = 0,
    APPLY_SNAPSHOT_VHDSET_VERSION_1 = 1,
} APPLY_SNAPSHOT_VHDSET_VERSION;

typedef struct _APPLY_SNAPSHOT_VHDSET_PARAMETERS {
    APPLY_SNAPSHOT_VHDSET_VERSION Version;
    union {
        struct {
            GUID SnapshotId;
            GUID LeafSnapshotId;
        } Version1;
    };
} APPLY_SNAPSHOT_VHDSET_PARAMETERS, *PAPPLY_SNAPSHOT_VHDSET_PARAMETERS;

typedef enum _RAW_SCSI_VIRTUAL_DISK_FLAG {
    RAW_SCSI_VIRTUAL_DISK_FLAG_NONE   = 0X00000000
} RAW_SCSI_VIRTUAL_DISK_FLAG, *PRAW_SCSI_VIRTUAL_DISK_FLAG;

/* DEFINE_ENUM_FLAG_OPERATORS(RAW_SCSI_VIRTUAL_DISK_FLAG); */

typedef enum _RAW_SCSI_VIRTUAL_DISK_VERSION {
    RAW_SCSI_VIRTUAL_DISK_VERSION_UNSPECIFIED = 0,
    RAW_SCSI_VIRTUAL_DISK_VERSION_1 = 1,
} RAW_SCSI_VIRTUAL_DISK_VERSION;

typedef struct _RAW_SCSI_VIRTUAL_DISK_PARAMETERS {
    RAW_SCSI_VIRTUAL_DISK_VERSION Version;
    union {
        struct {
            BOOL RSVDHandle;
            UCHAR DataIn;
            UCHAR CdbLength;
            UCHAR SenseInfoLength;
            ULONG SrbFlags;
            ULONG DataTransferLength;
            PVOID DataBuffer;
            UCHAR *SenseInfo;
            UCHAR *Cdb;
        } Version1;
    };
} RAW_SCSI_VIRTUAL_DISK_PARAMETERS, *PRAW_SCSI_VIRTUAL_DISK_PARAMETERS;

typedef struct _RAW_SCSI_VIRTUAL_DISK_RESPONSE {
    RAW_SCSI_VIRTUAL_DISK_VERSION Version;
    union {
        struct {
            UCHAR ScsiStatus;
            UCHAR SenseInfoLength;
            ULONG DataTransferLength;
        } Version1;
    };
} RAW_SCSI_VIRTUAL_DISK_RESPONSE, *PRAW_SCSI_VIRTUAL_DISK_RESPONSE;

#if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
DWORD WINAPI QueryChangesVirtualDisk(HANDLE, PCWSTR, ULONG64, ULONG64, QUERY_CHANGES_VIRTUAL_DISK_FLAG, PQUERY_CHANGES_VIRTUAL_DISK_RANGE, PULONG, PULONG64);
DWORD WINAPI TakeSnapshotVhdSet(HANDLE, const PTAKE_SNAPSHOT_VHDSET_PARAMETERS, TAKE_SNAPSHOT_VHDSET_FLAG);
DWORD WINAPI DeleteSnapshotVhdSet(HANDLE, const PDELETE_SNAPSHOT_VHDSET_PARAMETERS, DELETE_SNAPSHOT_VHDSET_FLAG);
DWORD WINAPI ModifyVhdSet(HANDLE, const PMODIFY_VHDSET_PARAMETERS, MODIFY_VHDSET_FLAG);
DWORD WINAPI ApplySnapshotVhdSet(HANDLE, const PAPPLY_SNAPSHOT_VHDSET_PARAMETERS, APPLY_SNAPSHOT_VHDSET_FLAG);
DWORD WINAPI RawSCSIVirtualDisk(HANDLE, const PRAW_SCSI_VIRTUAL_DISK_PARAMETERS, RAW_SCSI_VIRTUAL_DISK_FLAG, PRAW_SCSI_VIRTUAL_DISK_RESPONSE);
#endif /* NTDDI_VERSION >= NTDDI_WINTHRESHOLD */

#endif /* VIRTDISK_DEFINE_FLAGS */

#define SurfaceVirtualDisk  AttachVirtualDisk
#define UnsurfaceVirtualDisk  DetachVirtualDisk
#define VIRTUAL_DISK_ACCESS_SURFACE_RO  VIRTUAL_DISK_ACCESS_ATTACH_RO
#define VIRTUAL_DISK_ACCESS_SURFACE_RW  VIRTUAL_DISK_ACCESS_ATTACH_RW
#define VIRTUAL_DISK_ACCESS_UNSURFACE  VIRTUAL_DISK_ACCESS_DETACH
#define SURFACE_VIRTUAL_DISK_VERSION_UNSPECIFIED  ATTACH_VIRTUAL_DISK_VERSION_UNSPECIFIED
#define SURFACE_VIRTUAL_DISK_VERSION_1  ATTACH_VIRTUAL_DISK_VERSION_1
#define SURFACE_VIRTUAL_DISK_VERSION  ATTACH_VIRTUAL_DISK_VERSION
#define _SURFACE_VIRTUAL_DISK_VERSION  _ATTACH_VIRTUAL_DISK_VERSION
#define SURFACE_VIRTUAL_DISK_PARAMETERS  ATTACH_VIRTUAL_DISK_PARAMETERS
#define PSURFACE_VIRTUAL_DISK_PARAMETERS  PATTACH_VIRTUAL_DISK_PARAMETERS
#define _SURFACE_VIRTUAL_DISK_PARAMETERS  _ATTACH_VIRTUAL_DISK_PARAMETERS

#define _SURFACE_VIRTUAL_DISK_FLAG  _ATTACH_VIRTUAL_DISK_FLAG
#define SURFACE_VIRTUAL_DISK_FLAG_NONE  ATTACH_VIRTUAL_DISK_FLAG_NONE
#define SURFACE_VIRTUAL_DISK_FLAG_READ_ONLY  ATTACH_VIRTUAL_DISK_FLAG_READ_ONLY
#define SURFACE_VIRTUAL_DISK_FLAG_NO_DRIVE_LETTER  ATTACH_VIRTUAL_DISK_FLAG_NO_DRIVE_LETTER
#define SURFACE_VIRTUAL_DISK_FLAG_PERMANENT_LIFETIME  ATTACH_VIRTUAL_DISK_FLAG_PERMANENT_LIFETIME
#define SURFACE_VIRTUAL_DISK_FLAG_NO_LOCAL_HOST  ATTACH_VIRTUAL_DISK_FLAG_NO_LOCAL_HOST
#define SURFACE_VIRTUAL_DISK_FLAG_NO_SECURITY_DESCRIPTOR  ATTACH_VIRTUAL_DISK_FLAG_NO_SECURITY_DESCRIPTOR
#define SURFACE_VIRTUAL_DISK_FLAG  ATTACH_VIRTUAL_DISK_FLAG
#define _UNSURFACE_VIRTUAL_DISK_FLAG  _DETACH_VIRTUAL_DISK_FLAG
#define UNSURFACE_VIRTUAL_DISK_FLAG_NONE  DETACH_VIRTUAL_DISK_FLAG_NONE
#define UNSURFACE_VIRTUAL_DISK_FLAG  DETACH_VIRTUAL_DISK_FLAG

#ifndef VIRTDISK_DEFINE_FLAGS

/* DEFINE_ENUM_FLAG_OPERATORS(SURFACE_VIRTUAL_DISK_FLAG); */
/* DEFINE_ENUM_FLAG_OPERATORS(UNSURFACE_VIRTUAL_DISK_FLAG); */

#endif /* VIRTDISK_DEFINE_FLAGS */

#if __POCC__ >= 290
#pragma warn(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* WIN32_WINNT_WIN7 */

#endif /* _VIRTDISK_H */
