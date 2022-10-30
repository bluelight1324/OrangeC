#ifndef _VSSERROR_H
#define _VSSERROR_H

#define VSS_E_BAD_STATE ((HRESULT)0x80042301L)
#define VSS_E_UNEXPECTED ((HRESULT)0x80042302L)
#define VSS_E_PROVIDER_ALREADY_REGISTERED ((HRESULT)0x80042303L)
#define VSS_E_PROVIDER_NOT_REGISTERED ((HRESULT)0x80042304L)
#define VSS_E_PROVIDER_VETO ((HRESULT)0x80042306L)
#define VSS_E_PROVIDER_IN_USE ((HRESULT)0x80042307L)
#define VSS_E_OBJECT_NOT_FOUND ((HRESULT)0x80042308L)
#define VSS_S_ASYNC_PENDING ((HRESULT)0x00042309L)
#define VSS_S_ASYNC_FINISHED ((HRESULT)0x0004230AL)
#define VSS_S_ASYNC_CANCELLED ((HRESULT)0x0004230BL)
#define VSS_E_VOLUME_NOT_SUPPORTED ((HRESULT)0x8004230CL)
#define VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER ((HRESULT)0x8004230EL)
#define VSS_E_OBJECT_ALREADY_EXISTS ((HRESULT)0x8004230DL)
#define VSS_E_UNEXPECTED_PROVIDER_ERROR ((HRESULT)0x8004230FL)
#define VSS_E_CORRUPT_XML_DOCUMENT ((HRESULT)0x80042310L)
#define VSS_E_INVALID_XML_DOCUMENT ((HRESULT)0x80042311L)
#define VSS_E_MAXIMUM_NUMBER_OF_VOLUMES_REACHED ((HRESULT)0x80042312L)
#define VSS_E_FLUSH_WRITES_TIMEOUT ((HRESULT)0x80042313L)
#define VSS_E_HOLD_WRITES_TIMEOUT ((HRESULT)0x80042314L)
#define VSS_E_UNEXPECTED_WRITER_ERROR ((HRESULT)0x80042315L)
#define VSS_E_SNAPSHOT_SET_IN_PROGRESS ((HRESULT)0x80042316L)
#define VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED ((HRESULT)0x80042317L)
#define VSS_E_WRITER_INFRASTRUCTURE ((HRESULT)0x80042318L)
#define VSS_E_WRITER_NOT_RESPONDING ((HRESULT)0x80042319L)
#define VSS_E_WRITER_ALREADY_SUBSCRIBED ((HRESULT)0x8004231AL)
#define VSS_E_UNSUPPORTED_CONTEXT ((HRESULT)0x8004231BL)
#define VSS_E_VOLUME_IN_USE ((HRESULT)0x8004231DL)
#define VSS_E_MAXIMUM_DIFFAREA_ASSOCIATIONS_REACHED ((HRESULT)0x8004231EL)
#define VSS_E_INSUFFICIENT_STORAGE ((HRESULT)0x8004231FL)
#define VSS_E_NO_SNAPSHOTS_IMPORTED ((HRESULT)0x80042320L)
#define VSS_S_SOME_SNAPSHOTS_NOT_IMPORTED ((HRESULT)0x00042321L)
#define VSS_E_SOME_SNAPSHOTS_NOT_IMPORTED ((HRESULT)0x80042321L)
#define VSS_E_MAXIMUM_NUMBER_OF_REMOTE_MACHINES_REACHED ((HRESULT)0x80042322L)
#define VSS_E_REMOTE_SERVER_UNAVAILABLE ((HRESULT)0x80042323L)
#define VSS_E_REMOTE_SERVER_UNSUPPORTED ((HRESULT)0x80042324L)
#define VSS_E_REVERT_IN_PROGRESS ((HRESULT)0x80042325L)
#define VSS_E_REVERT_VOLUME_LOST ((HRESULT)0x80042326L)
#define VSS_E_REBOOT_REQUIRED ((HRESULT)0x80042327L)
#define VSS_E_TRANSACTION_FREEZE_TIMEOUT ((HRESULT)0x80042328L)
#define VSS_E_TRANSACTION_THAW_TIMEOUT ((HRESULT)0x80042329L)
#define VSS_E_VOLUME_NOT_LOCAL ((HRESULT)0x8004232DL)
#define VSS_E_CLUSTER_TIMEOUT ((HRESULT)0x8004232EL)
#define VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT ((HRESULT)0x800423F0L)
#define VSS_E_WRITERERROR_OUTOFRESOURCES ((HRESULT)0x800423F1L)
#define VSS_E_WRITERERROR_TIMEOUT ((HRESULT)0x800423F2L)
#define VSS_E_WRITERERROR_RETRYABLE ((HRESULT)0x800423F3L)
#define VSS_E_WRITERERROR_NONRETRYABLE ((HRESULT)0x800423F4L)
#define VSS_E_WRITERERROR_RECOVERY_FAILED ((HRESULT)0x800423F5L)
#define VSS_E_BREAK_REVERT_ID_FAILED ((HRESULT)0x800423F6L)
#define VSS_E_LEGACY_PROVIDER ((HRESULT)0x800423F7L)
#define VSS_E_MISSING_DISK ((HRESULT)0x800423F8L)
#define VSS_E_MISSING_HIDDEN_VOLUME ((HRESULT)0x800423F9L)
#define VSS_E_MISSING_VOLUME ((HRESULT)0x800423FAL)
#define VSS_E_AUTORECOVERY_FAILED ((HRESULT)0x800423FBL)
#define VSS_E_DYNAMIC_DISK_ERROR ((HRESULT)0x800423FCL)
#define VSS_E_NONTRANSPORTABLE_BCD ((HRESULT)0x800423FDL)
#define VSS_E_CANNOT_REVERT_DISKID ((HRESULT)0x800423FEL)
#define VSS_E_RESYNC_IN_PROGRESS ((HRESULT)0x800423FFL)
#define VSS_E_CLUSTER_ERROR ((HRESULT)0x80042400L)
#define VSS_E_UNSELECTED_VOLUME ((HRESULT)0x8004232AL)
#define VSS_E_SNAPSHOT_NOT_IN_SET ((HRESULT)0x8004232BL)
#define VSS_E_NESTED_VOLUME_LIMIT ((HRESULT)0x8004232CL)
#define VSS_E_NOT_SUPPORTED ((HRESULT)0x8004232FL)
#define VSS_E_WRITERERROR_PARTIAL_FAILURE ((HRESULT)0x80042336L)
#define VSS_E_ASRERROR_DISK_ASSIGNMENT_FAILED ((HRESULT)0x80042401L)
#define VSS_E_ASRERROR_DISK_RECREATION_FAILED ((HRESULT)0x80042402L)
#define VSS_E_ASRERROR_NO_ARCPATH ((HRESULT)0x80042403L)
#define VSS_E_ASRERROR_MISSING_DYNDISK ((HRESULT)0x80042404L)
#define VSS_E_ASRERROR_SHARED_CRIDISK ((HRESULT)0x80042405L)
#define VSS_E_ASRERROR_DATADISK_RDISK0 ((HRESULT)0x80042406L)
#define VSS_E_ASRERROR_RDISK0_TOOSMALL ((HRESULT)0x80042407L)
#define VSS_E_ASRERROR_CRITICAL_DISKS_TOO_SMALL ((HRESULT)0x80042408L)
#define VSS_E_WRITER_STATUS_NOT_AVAILABLE ((HRESULT)0x80042409L)
#define VSS_E_ASRERROR_DYNAMIC_VHD_NOT_SUPPORTED ((HRESULT)0x8004240AL)
#define VSS_E_CRITICAL_VOLUME_ON_INVALID_DISK ((HRESULT)0x80042411L)
#define VSS_E_ASRERROR_RDISK_FOR_SYSTEM_DISK_NOT_FOUND ((HRESULT)0x80042412L)
#define VSS_E_ASRERROR_NO_PHYSICAL_DISK_AVAILABLE ((HRESULT)0x80042413L)
#define VSS_E_ASRERROR_FIXED_PHYSICAL_DISK_AVAILABLE_AFTER_DISK_EXCLUSION ((HRESULT)0x80042414L)
#define VSS_E_ASRERROR_CRITICAL_DISK_CANNOT_BE_EXCLUDED ((HRESULT)0x80042415L)
#define VSS_E_ASRERROR_SYSTEM_PARTITION_HIDDEN ((HRESULT)0x80042416L)

#endif /* _VSSERROR_H */
