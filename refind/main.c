/*
 * refind/main.c
 * Main code for the boot menu
 *
 * Copyright (c) 2006-2010 Christoph Pfisterer
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *  * Neither the name of Christoph Pfisterer nor the names of the
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Modifications copyright (c) 2012-2017 Roderick W. Smith
 *
 * Modifications distributed under the terms of the GNU General Public
 * License (GPL) version 3 (GPLv3), or (at your option) any later version.
 *
 */
/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "global.h"
#include "config.h"
#include "screen.h"
#include "legacy.h"
#include "lib.h"
#include "icns.h"
#include "menu.h"
#include "pointer.h"
#include "mok.h"
#include "gpt.h"
#include "apple.h"
#include "mystrings.h"
#include "security_policy.h"
#include "driver_support.h"
#include "../include/Handle.h"
#include "../include/refit_call_wrapper.h"
#include "../include/version.h"
#include "../EfiLib/BdsHelper.h"
#include "../EfiLib/legacy.h"

#ifdef __MAKEWITH_GNUEFI
#ifndef EFI_SECURITY_VIOLATION
#define EFI_SECURITY_VIOLATION    EFIERR (26)
#endif
#endif

#ifndef EFI_OS_INDICATIONS_BOOT_TO_FW_UI
#define EFI_OS_INDICATIONS_BOOT_TO_FW_UI 0x0000000000000001ULL
#endif

#ifdef __MAKEWITH_TIANO
#define LibLocateHandle gBS->LocateHandleBuffer
#endif

//
// constants

#define MACOSX_LOADER_DIR      L"System\\Library\\CoreServices"
#define MACOSX_LOADER_PATH      ( MACOSX_LOADER_DIR L"\\boot.efi" )
#if defined (EFIX64)
#define SHELL_NAMES             L"\\EFI\\tools\\shell.efi,\\EFI\\tools\\shellx64.efi,\\shell.efi,\\shellx64.efi"
#define GPTSYNC_NAMES           L"\\EFI\\tools\\gptsync.efi,\\EFI\\tools\\gptsync_x64.efi"
#define GDISK_NAMES             L"\\EFI\\tools\\gdisk.efi,\\EFI\\tools\\gdisk_x64.efi"
#define NETBOOT_NAMES           L"\\EFI\\tools\\ipxe.efi"
#define MEMTEST_NAMES           L"memtest86.efi,memtest86_x64.efi,memtest86x64.efi,bootx64.efi"
#define FALLBACK_FULLNAME       L"EFI\\BOOT\\bootx64.efi"
#define FALLBACK_BASENAME       L"bootx64.efi"
#define EFI_STUB_ARCH           0x8664
EFI_GUID gFreedesktopRootGuid = { 0x4f68bce3, 0xe8cd, 0x4db1, { 0x96, 0xe7, 0xfb, 0xca, 0xf9, 0x84, 0xb7, 0x09 }};
#elif defined (EFI32)
#define SHELL_NAMES             L"\\EFI\\tools\\shell.efi,\\EFI\\tools\\shellia32.efi,\\shell.efi,\\shellia32.efi"
#define GPTSYNC_NAMES           L"\\EFI\\tools\\gptsync.efi,\\EFI\\tools\\gptsync_ia32.efi"
#define GDISK_NAMES             L"\\EFI\\tools\\gdisk.efi,\\EFI\\tools\\gdisk_ia32.efi"
#define NETBOOT_NAMES           L"\\EFI\\tools\\ipxe.efi"
#define MEMTEST_NAMES           L"memtest86.efi,memtest86_ia32.efi,memtest86ia32.efi,bootia32.efi"
#define FALLBACK_FULLNAME       L"EFI\\BOOT\\bootia32.efi"
#define FALLBACK_BASENAME       L"bootia32.efi"
#define EFI_STUB_ARCH           0x014c
EFI_GUID gFreedesktopRootGuid = { 0x44479540, 0xf297, 0x41b2, { 0x9a, 0xf7, 0xd1, 0x31, 0xd5, 0xf0, 0x45, 0x8a }};
#elif defined (EFIAARCH64)
#define SHELL_NAMES             L"\\EFI\\tools\\shell.efi,\\EFI\\tools\\shellaa64.efi,\\shell.efi,\\shellaa64.efi"
#define GPTSYNC_NAMES           L"\\EFI\\tools\\gptsync.efi,\\EFI\\tools\\gptsync_aa64.efi"
#define GDISK_NAMES             L"\\EFI\\tools\\gdisk.efi,\\EFI\\tools\\gdisk_aa64.efi"
#define NETBOOT_NAMES           L"\\EFI\\tools\\ipxe.efi"
#define MEMTEST_NAMES           L"memtest86.efi,memtest86_aa64.efi,memtest86aa64.efi,bootaa64.efi"
#define FALLBACK_FULLNAME       L"EFI\\BOOT\\bootaa64.efi"
#define FALLBACK_BASENAME       L"bootaa64.efi"
#define EFI_STUB_ARCH           0xaa64
EFI_GUID gFreedesktopRootGuid = { 0xb921b045, 0x1df0, 0x41c3, { 0xaf, 0x44, 0x4c, 0x6f, 0x28, 0x0d, 0x3f, 0xae }};
#else
#define SHELL_NAMES             L"\\EFI\\tools\\shell.efi,\\shell.efi"
#define GPTSYNC_NAMES           L"\\EFI\\tools\\gptsync.efi"
#define GDISK_NAMES             L"\\EFI\\tools\\gdisk.efi"
#define NETBOOT_NAMES           L"\\EFI\\tools\\ipxe.efi"
#define MEMTEST_NAMES           L"memtest86.efi"
#define DRIVER_DIRS             L"drivers"
#define FALLBACK_FULLNAME       L"EFI\\BOOT\\boot.efi" /* Not really correct */
#define FALLBACK_BASENAME       L"boot.efi"            /* Not really correct */
// Below is GUID for ARM32
EFI_GUID gFreedesktopRootGuid = { 0x69dad710, 0x2ce4, 0x4e3c, { 0xb1, 0x6c, 0x21, 0xa1, 0xd4, 0x9a, 0xbe, 0xd3 }};
#endif
#define FAT_ARCH                0x0ef1fab9 /* ID for Apple "fat" binary */

#define IPXE_DISCOVER_NAME      L"\\efi\\tools\\ipxe_discover.efi"
#define IPXE_NAME               L"\\efi\\tools\\ipxe.efi"

// Patterns that identify Linux kernels. Added to the loader match pattern when the
// scan_all_linux_kernels option is set in the configuration file. Causes kernels WITHOUT
// a ".efi" extension to be found when scanning for boot loaders.
#define LINUX_MATCH_PATTERNS    L"vmlinuz*,bzImage*,kernel*"

// Maximum length of a text string in certain menus
#define MAX_LINE_LENGTH 65

static REFIT_MENU_ENTRY MenuEntryAbout    = { L"About rEFInd", TAG_ABOUT, 1, 0, 'A', NULL, NULL, NULL };
static REFIT_MENU_ENTRY MenuEntryReset    = { L"Reboot Computer", TAG_REBOOT, 1, 0, 'R', NULL, NULL, NULL };
static REFIT_MENU_ENTRY MenuEntryShutdown = { L"Shut Down Computer", TAG_SHUTDOWN, 1, 0, 'U', NULL, NULL, NULL };
REFIT_MENU_ENTRY MenuEntryReturn   = { L"Return to Main Menu", TAG_RETURN, 1, 0, 0, NULL, NULL, NULL };
static REFIT_MENU_ENTRY MenuEntryExit     = { L"Exit rEFInd", TAG_EXIT, 1, 0, 0, NULL, NULL, NULL };
static REFIT_MENU_ENTRY MenuEntryManageHidden = { L"Manage Hidden Tags Menu", TAG_HIDDEN, 1, 0, 0, NULL, NULL, NULL };
static REFIT_MENU_ENTRY MenuEntryFirmware = { L"Reboot to Computer Setup Utility", TAG_FIRMWARE, 1, 0, 0, NULL, NULL, NULL };
static REFIT_MENU_ENTRY MenuEntryRotateCsr = { L"Change SIP Policy", TAG_CSR_ROTATE, 1, 0, 0, NULL, NULL, NULL };

REFIT_MENU_SCREEN MainMenu       = { L"Main Menu", NULL, 0, NULL, 0, NULL, 0, L"Automatic boot",
                                     L"Use arrow keys to move cursor; Enter to boot;",
                                     L"Insert, Tab, or F2 for more options; Esc or Backspace to refresh" };
static REFIT_MENU_SCREEN AboutMenu      = { L"About", NULL, 0, NULL, 0, NULL, 0, NULL, L"Press Enter to return to main menu", L"" };

REFIT_CONFIG GlobalConfig = { /* TextOnly = */ FALSE,
                              /* ScanAllLinux = */ TRUE,
                              /* DeepLegacyScan = */ FALSE,
                              /* EnableAndLockVMX = */ FALSE,
                              /* FoldLinuxKernels = */ TRUE,
                              /* EnableMouse = */ FALSE,
                              /* EnableTouch = */ FALSE,
                              /* HiddenTags = */ TRUE,
                              /* UseNvram = */ TRUE,
                              /* ShutdownAfterTimeout = */ FALSE,
                              /* RequestedScreenWidth = */ 0,
                              /* RequestedScreenHeight = */ 0,
                              /* BannerBottomEdge = */ 0,
                              /* RequestedTextMode = */ DONT_CHANGE_TEXT_MODE,
                              /* Timeout = */ 20,
                              /* HideUIFlags = */ 0,
                              /* MaxTags = */ 0,
                              /* GraphicsFor = */ GRAPHICS_FOR_OSX,
                              /* LegacyType = */ LEGACY_TYPE_MAC,
                              /* ScanDelay = */ 0,
                              /* ScreensaverTime = */ 0,
                              /* MouseSpeed = */ 4,
                              /* IconSizes = */ { DEFAULT_BIG_ICON_SIZE / 4,
                                                  DEFAULT_SMALL_ICON_SIZE,
                                                  DEFAULT_BIG_ICON_SIZE,
                                                  DEFAULT_MOUSE_SIZE },
                              /* BannerScale = */ BANNER_NOSCALE,
                              /* *DiscoveredRoot = */ NULL,
                              /* *SelfDevicePath = */ NULL,
                              /* *BannerFileName = */ NULL,
                              /* *ScreenBackground = */ NULL,
                              /* *ConfigFilename = */ CONFIG_FILE_NAME,
                              /* *SelectionSmallFileName = */ NULL,
                              /* *SelectionBigFileName = */ NULL,
                              /* *DefaultSelection = */ NULL,
                              /* *AlsoScan = */ NULL,
                              /* *DontScanVolumes = */ NULL,
                              /* *DontScanDirs = */ NULL,
                              /* *DontScanFiles = */ NULL,
                              /* *DontScanTools = */ NULL,
                              /* *WindowsRecoveryFiles = */ NULL,
                              /* *MacOSRecoveryFiles = */ NULL,
                              /* *DriverDirs = */ NULL,
                              /* *IconsDir = */ NULL,
                              /* *ExtraKernelVersionStrings = */ NULL,
                              /* *SpoofOSXVersion = */ NULL,
                              /* CsrValues = */ NULL,
                              /* ShowTools = */ { TAG_SHELL, TAG_MEMTEST, TAG_GDISK, TAG_APPLE_RECOVERY, TAG_WINDOWS_RECOVERY,
                                                  TAG_MOK_TOOL, TAG_ABOUT, TAG_HIDDEN, TAG_SHUTDOWN, TAG_REBOOT, TAG_FIRMWARE,
                                                  TAG_FWUPDATE_TOOL, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
                            };

EFI_GUID GlobalGuid = EFI_GLOBAL_VARIABLE;
EFI_GUID RefindGuid = REFIND_GUID_VALUE;

GPT_DATA *gPartitions = NULL;

// Structure used to hold boot loader filenames and time stamps in
// a linked list; used to sort entries within a directory.
struct LOADER_LIST {
    CHAR16              *FileName;
    EFI_TIME            TimeStamp;
    struct LOADER_LIST  *NextEntry;
};

//
// misc functions
//

static VOID AboutrEFInd(VOID)
{
    CHAR16     *FirmwareVendor;
    UINT32     CsrStatus;

    if (AboutMenu.EntryCount == 0) {
        AboutMenu.TitleImage = BuiltinIcon(BUILTIN_ICON_FUNC_ABOUT);
        AddMenuInfoLine(&AboutMenu, PoolPrint(L"rEFInd Version %s", REFIND_VERSION));
        AddMenuInfoLine(&AboutMenu, L"");
        AddMenuInfoLine(&AboutMenu, L"Copyright (c) 2006-2010 Christoph Pfisterer");
        AddMenuInfoLine(&AboutMenu, L"Copyright (c) 2012-2017 Roderick W. Smith");
        AddMenuInfoLine(&AboutMenu, L"Portions Copyright (c) Intel Corporation and others");
        AddMenuInfoLine(&AboutMenu, L"Distributed under the terms of the GNU GPLv3 license");
        AddMenuInfoLine(&AboutMenu, L"");
        AddMenuInfoLine(&AboutMenu, L"Running on:");
        AddMenuInfoLine(&AboutMenu, PoolPrint(L" EFI Revision %d.%02d", ST->Hdr.Revision >> 16, ST->Hdr.Revision & ((1 << 16) - 1)));
#if defined(EFI32)
        AddMenuInfoLine(&AboutMenu, PoolPrint(L" Platform: x86 (32 bit); Secure Boot %s",
                                              secure_mode() ? L"active" : L"inactive"));
#elif defined(EFIX64)
        AddMenuInfoLine(&AboutMenu, PoolPrint(L" Platform: x86_64 (64 bit); Secure Boot %s",
                                              secure_mode() ? L"active" : L"inactive"));
#elif defined(EFIAARCH64)
        AddMenuInfoLine(&AboutMenu, PoolPrint(L" Platform: ARM (64 bit); Secure Boot %s",
                                              secure_mode() ? L"active" : L"inactive"));
#else
        AddMenuInfoLine(&AboutMenu, L" Platform: unknown");
#endif
        if (GetCsrStatus(&CsrStatus) == EFI_SUCCESS) {
            RecordgCsrStatus(CsrStatus, FALSE);
            AddMenuInfoLine(&AboutMenu, gCsrStatus);
        }
        FirmwareVendor = StrDuplicate(ST->FirmwareVendor);
        LimitStringLength(FirmwareVendor, MAX_LINE_LENGTH); // More than ~65 causes empty info page on 800x600 display
        AddMenuInfoLine(&AboutMenu, PoolPrint(L" Firmware: %s %d.%02d", FirmwareVendor, ST->FirmwareRevision >> 16,
                                              ST->FirmwareRevision & ((1 << 16) - 1)));
        AddMenuInfoLine(&AboutMenu, PoolPrint(L" Screen Output: %s", egScreenDescription()));
        AddMenuInfoLine(&AboutMenu, L"");
#if defined(__MAKEWITH_GNUEFI)
        AddMenuInfoLine(&AboutMenu, L"Built with GNU-EFI");
#else
        AddMenuInfoLine(&AboutMenu, L"Built with TianoCore EDK2");
#endif
        AddMenuInfoLine(&AboutMenu, L"");
        AddMenuInfoLine(&AboutMenu, L"For more information, see the rEFInd Web site:");
        AddMenuInfoLine(&AboutMenu, L"http://www.rodsbooks.com/refind/");
        AddMenuEntry(&AboutMenu, &MenuEntryReturn);
    }

    RunMenu(&AboutMenu, NULL);
} /* VOID AboutrEFInd() */

static VOID WarnSecureBootError(CHAR16 *Name, BOOLEAN Verbose) {
    if (Name == NULL)
        Name = L"the loader";

    refit_call2_wrapper(ST->ConOut->SetAttribute, ST->ConOut, ATTR_ERROR);
    Print(L"Secure Boot validation failure loading %s!\n", Name);
    refit_call2_wrapper(ST->ConOut->SetAttribute, ST->ConOut, ATTR_BASIC);
    if (Verbose && secure_mode()) {
        Print(L"\nThis computer is configured with Secure Boot active, but\n%s has failed validation.\n", Name);
        Print(L"\nYou can:\n * Launch another boot loader\n");
        Print(L" * Disable Secure Boot in your firmware\n");
        Print(L" * Sign %s with a machine owner key (MOK)\n", Name);
        Print(L" * Use a MOK utility (often present on the second row) to add a MOK with which\n");
        Print(L"   %s has already been signed.\n", Name);
        Print(L" * Use a MOK utility to register %s (\"enroll its hash\") without\n", Name);
        Print(L"   signing it.\n");
        Print(L"\nSee http://www.rodsbooks.com/refind/secureboot.html for more information\n");
        PauseForKey();
    } // if
} // VOID WarnSecureBootError()

// Returns TRUE if this file is a valid EFI loader file, and is proper ARCH
static BOOLEAN IsValidLoader(EFI_FILE *RootDir, CHAR16 *FileName) {
    BOOLEAN         IsValid = TRUE;
#if defined (EFIX64) | defined (EFI32) | defined (EFIAARCH64)
    EFI_STATUS      Status;
    EFI_FILE_HANDLE FileHandle;
    CHAR8           Header[512];
    UINTN           Size = sizeof(Header);

    if ((RootDir == NULL) || (FileName == NULL)) {
        // Assume valid here, because Macs produce NULL RootDir (& maybe FileName)
        // when launching from a Firewire drive. This should be handled better, but
        // fix would have to be in StartEFIImage() and/or in FindVolumeAndFilename().
        return TRUE;
    } // if

    Status = refit_call5_wrapper(RootDir->Open, RootDir, &FileHandle, FileName, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(Status))
        return FALSE;

    Status = refit_call3_wrapper(FileHandle->Read, FileHandle, &Size, Header);
    refit_call1_wrapper(FileHandle->Close, FileHandle);

    IsValid = !EFI_ERROR(Status) &&
              Size == sizeof(Header) &&
              ((Header[0] == 'M' && Header[1] == 'Z' &&
               (Size = *(UINT32 *)&Header[0x3c]) < 0x180 &&
               Header[Size] == 'P' && Header[Size+1] == 'E' &&
               Header[Size+2] == 0 && Header[Size+3] == 0 &&
               *(UINT16 *)&Header[Size+4] == EFI_STUB_ARCH) ||
              (*(UINT32 *)&Header == FAT_ARCH));
#endif
    return IsValid;
} // BOOLEAN IsValidLoader()

// Launch an EFI binary.
EFI_STATUS StartEFIImage(IN REFIT_VOLUME *Volume,
                         IN CHAR16 *Filename,
                         IN CHAR16 *LoadOptions,
                         IN CHAR16 *ImageTitle,
                         IN CHAR8 OSType,
                         IN BOOLEAN Verbose,
                         IN BOOLEAN IsDriver)
{
    EFI_STATUS              Status, ReturnStatus;
    EFI_HANDLE              ChildImageHandle, ChildImageHandle2;
    EFI_DEVICE_PATH         *DevicePath;
    EFI_LOADED_IMAGE        *ChildLoadedImage = NULL;
    CHAR16                  ErrorInfo[256];
    CHAR16                  *FullLoadOptions = NULL;
    CHAR16                  *Temp;

    // set load options
    if (LoadOptions != NULL) {
        FullLoadOptions = StrDuplicate(LoadOptions);
        if (OSType == 'M') {
            MergeStrings(&FullLoadOptions, L" ", 0);
            // NOTE: That last space is also added by the EFI shell and seems to be significant
            // when passing options to Apple's boot.efi...
        } // if
    } // if (LoadOptions != NULL)
    if (Verbose)
        Print(L"Starting %s\nUsing load options '%s'\n", ImageTitle, FullLoadOptions ? FullLoadOptions : L"");

    // load the image into memory
    ReturnStatus = Status = EFI_NOT_FOUND;  // in case the list is empty
    // Some EFIs crash if attempting to load driver for invalid architecture, so
    // protect for this condition; but sometimes Volume comes back NULL, so provide
    // an exception. (TODO: Handle this special condition better.)
    if (IsValidLoader(Volume->RootDir, Filename)) {
        if (Filename) {
            Temp = PoolPrint(L"\\%s %s", Filename, FullLoadOptions ? FullLoadOptions : L"");
            if (Temp != NULL) {
                MyFreePool(FullLoadOptions);
                FullLoadOptions = Temp;
            }
        } // if (Filename)

        DevicePath = FileDevicePath(Volume->DeviceHandle, Filename);
        // NOTE: Below commented-out line could be more efficient if file were read ahead of
        // time and passed as a pre-loaded image to LoadImage(), but it doesn't work on my
        // 32-bit Mac Mini or my 64-bit Intel box when launching a Linux kernel; the
        // kernel returns a "Failed to handle fs_proto" error message.
        // TODO: Track down the cause of this error and fix it, if possible.
        // ReturnStatus = Status = refit_call6_wrapper(BS->LoadImage, FALSE, SelfImageHandle, DevicePath,
        //                                            ImageData, ImageSize, &ChildImageHandle);
        ReturnStatus = Status = refit_call6_wrapper(BS->LoadImage, FALSE, SelfImageHandle, DevicePath,
                                                    NULL, 0, &ChildImageHandle);
        if (secure_mode() && ShimLoaded()) {
            // Load ourself into memory. This is a trick to work around a bug in Shim 0.8,
            // which ties itself into the BS->LoadImage() and BS->StartImage() functions and
            // then unregisters itself from the EFI system table when its replacement
            // StartImage() function is called *IF* the previous LoadImage() was for the same
            // program. The result is that rEFInd can validate only the first program it
            // launches (often a filesystem driver). Loading a second program (rEFInd itself,
            // here, to keep it smaller than a kernel) works around this problem. See the
            // replacements.c file in Shim, and especially its start_image() function, for
            // the source of the problem.
            // NOTE: This doesn't check the return status or handle errors. It could
            // conceivably do weird things if, say, rEFInd were on a USB drive that the
            // user pulls before launching a program.
            refit_call6_wrapper(BS->LoadImage, FALSE, SelfImageHandle, GlobalConfig.SelfDevicePath,
                                NULL, 0, &ChildImageHandle2);
        }
    } else {
        Print(L"Invalid loader file!\n");
        ReturnStatus = EFI_LOAD_ERROR;
    }
    if ((Status == EFI_ACCESS_DENIED) || (Status == EFI_SECURITY_VIOLATION)) {
        WarnSecureBootError(ImageTitle, Verbose);
        goto bailout;
    }
    SPrint(ErrorInfo, 255, L"while loading %s", ImageTitle);
    if (CheckError(Status, ErrorInfo)) {
        goto bailout;
    }

    ReturnStatus = Status = refit_call3_wrapper(BS->HandleProtocol, ChildImageHandle, &LoadedImageProtocol,
                                                (VOID **) &ChildLoadedImage);
    if (CheckError(Status, L"while getting a LoadedImageProtocol handle")) {
        goto bailout_unload;
    }
    ChildLoadedImage->LoadOptions = (VOID *)FullLoadOptions;
    ChildLoadedImage->LoadOptionsSize = FullLoadOptions ? ((UINT32)StrLen(FullLoadOptions) + 1) * sizeof(CHAR16) : 0;
    // turn control over to the image
    // TODO: (optionally) re-enable the EFI watchdog timer!

    // close open file handles
    UninitRefitLib();
    ReturnStatus = Status = refit_call3_wrapper(BS->StartImage, ChildImageHandle, NULL, NULL);

    // control returns here when the child image calls Exit()
    SPrint(ErrorInfo, 255, L"returned from %s", ImageTitle);
    CheckError(Status, ErrorInfo);
    if (IsDriver) {
        // Below should have no effect on most systems, but works
        // around bug with some EFIs that prevents filesystem drivers
        // from binding to partitions.
        ConnectFilesystemDriver(ChildImageHandle);
    }

    // re-open file handles
    ReinitRefitLib();

bailout_unload:
    // unload the image, we don't care if it works or not...
    if (!IsDriver)
        Status = refit_call1_wrapper(BS->UnloadImage, ChildImageHandle);

bailout:
    MyFreePool(FullLoadOptions);
    return ReturnStatus;
} /* EFI_STATUS StartEFIImage() */

// From gummiboot: Reboot the computer into its built-in user interface
static EFI_STATUS RebootIntoFirmware(VOID) {
    CHAR8 *b;
    UINTN size;
    UINT64 osind;
    EFI_STATUS err;

    osind = EFI_OS_INDICATIONS_BOOT_TO_FW_UI;

    err = EfivarGetRaw(&GlobalGuid, L"OsIndications", &b, &size);
    if (err == EFI_SUCCESS)
        osind |= (UINT64)*b;
    MyFreePool(b);

    err = EfivarSetRaw(&GlobalGuid, L"OsIndications", (CHAR8 *)&osind, sizeof(UINT64), TRUE);
    if (err != EFI_SUCCESS)
        return err;

    refit_call4_wrapper(RT->ResetSystem, EfiResetCold, EFI_SUCCESS, 0, NULL);
    Print(L"Error calling ResetSystem: %r", err);
    PauseForKey();
    return err;
}

// Record the value of the loader's name/description in rEFInd's "PreviousBoot" EFI variable,
// if it's different from what's already stored there.
VOID StoreLoaderName(IN CHAR16 *Name) {
    EFI_STATUS   Status;
    CHAR16       *OldName = NULL;
    UINTN        Length;

    if (Name) {
        Status = EfivarGetRaw(&RefindGuid, L"PreviousBoot", (CHAR8**) &OldName, &Length);
        if ((Status != EFI_SUCCESS) || (StrCmp(OldName, Name) != 0)) {
            EfivarSetRaw(&RefindGuid, L"PreviousBoot", (CHAR8*) Name, StrLen(Name) * 2 + 2, TRUE);
        } // if
        MyFreePool(OldName);
    } // if
} // VOID StoreLoaderName()

//
// EFI OS loader functions
//

// See http://www.thomas-krenn.com/en/wiki/Activating_the_Intel_VT_Virtualization_Feature
// for information on Intel VMX features
static VOID DoEnableAndLockVMX(VOID)
{
#if defined (EFIX64) | defined (EFI32)
    UINT32 msr = 0x3a;
    UINT32 low_bits = 0, high_bits = 0;

    // is VMX active ?
    __asm__ volatile ("rdmsr" : "=a" (low_bits), "=d" (high_bits) : "c" (msr));

    // enable and lock vmx if not locked
    if ((low_bits & 1) == 0) {
        high_bits = 0;
        low_bits = 0x05;
        msr = 0x3a;
        __asm__ volatile ("wrmsr" : : "c" (msr), "a" (low_bits), "d" (high_bits));
    }
#endif
} // VOID DoEnableAndLockVMX()

static VOID StartLoader(LOADER_ENTRY *Entry, CHAR16 *SelectionName)
{
    if (GlobalConfig.EnableAndLockVMX) {
        DoEnableAndLockVMX();
    }

    BeginExternalScreen(Entry->UseGraphicsMode, L"Booting OS");
    StoreLoaderName(SelectionName);
    StartEFIImage(Entry->Volume, Entry->LoaderPath, Entry->LoadOptions,
                  Basename(Entry->LoaderPath), Entry->OSType, !Entry->UseGraphicsMode, FALSE);
    FinishExternalScreen();
}

// Locate an initrd or initramfs file that matches the kernel specified by LoaderPath.
// The matching file has a name that begins with "init" and includes the same version
// number string as is found in LoaderPath -- but not a longer version number string.
// For instance, if LoaderPath is \EFI\kernels\bzImage-3.3.0.efi, and if \EFI\kernels
// has a file called initramfs-3.3.0.img, this function will return the string
// '\EFI\kernels\initramfs-3.3.0.img'. If the directory ALSO contains the file
// initramfs-3.3.0-rc7.img or initramfs-13.3.0.img, those files will NOT match.
// If more than one initrd file matches the extracted version string, the one
// that matches more characters AFTER (actually, from the start of) the version
// string is used.
// If more than one initrd file matches the extracted version string AND they match
// the same amount of characters, the initrd file with the shortest file name is used.
// If no matching init file can be found, returns NULL.
static CHAR16 * FindInitrd(IN CHAR16 *LoaderPath, IN REFIT_VOLUME *Volume) {
    CHAR16              *InitrdName = NULL, *FileName, *KernelVersion, *InitrdVersion, *Path;
    CHAR16              *KernelPostNum, *InitrdPostNum;
    UINTN               MaxSharedChars, SharedChars;
    STRING_LIST         *InitrdNames = NULL, *FinalInitrdName = NULL, *CurrentInitrdName = NULL, *MaxSharedInitrd;
    REFIT_DIR_ITER      DirIter;
    EFI_FILE_INFO       *DirEntry;

    FileName = Basename(LoaderPath);
    KernelVersion = FindNumbers(FileName);
    Path = FindPath(LoaderPath);

    // Add trailing backslash for root directory; necessary on some systems, but must
    // NOT be added to all directories, since on other systems, a trailing backslash on
    // anything but the root directory causes them to flake out!
    if (StrLen(Path) == 0) {
        MergeStrings(&Path, L"\\", 0);
    } // if
    DirIterOpen(Volume->RootDir, Path, &DirIter);
    // Now add a trailing backslash if it was NOT added earlier, for consistency in
    // building the InitrdName later....
    if ((StrLen(Path) > 0) && (Path[StrLen(Path) - 1] != L'\\'))
        MergeStrings(&Path, L"\\", 0);
    while (DirIterNext(&DirIter, 2, L"init*", &DirEntry)) {
        InitrdVersion = FindNumbers(DirEntry->FileName);
        if (((KernelVersion != NULL) && (MyStriCmp(InitrdVersion, KernelVersion))) ||
            ((KernelVersion == NULL) && (InitrdVersion == NULL))) {
                CurrentInitrdName = AllocateZeroPool(sizeof(STRING_LIST));
                if (InitrdNames == NULL)
                    InitrdNames = FinalInitrdName = CurrentInitrdName;
                if (CurrentInitrdName) {
                    CurrentInitrdName->Value = PoolPrint(L"%s%s", Path, DirEntry->FileName);
                    if (CurrentInitrdName != FinalInitrdName) {
                        FinalInitrdName->Next = CurrentInitrdName;
                        FinalInitrdName = CurrentInitrdName;
                    } // if
                } // if
        } // if
    } // while
    if (InitrdNames) {
        if (InitrdNames->Next == NULL) {
            InitrdName = StrDuplicate(InitrdNames -> Value);
        } else {
            MaxSharedInitrd = CurrentInitrdName = InitrdNames;
            MaxSharedChars = 0;
            while (CurrentInitrdName != NULL) {
                KernelPostNum = MyStrStr(LoaderPath, KernelVersion);
                InitrdPostNum = MyStrStr(CurrentInitrdName->Value, KernelVersion);
                SharedChars = NumCharsInCommon(KernelPostNum, InitrdPostNum);
                if (SharedChars > MaxSharedChars || (SharedChars == MaxSharedChars && StrLen(CurrentInitrdName->Value) < StrLen(MaxSharedInitrd->Value))) {
                    MaxSharedChars = SharedChars;
                    MaxSharedInitrd = CurrentInitrdName;
                } // if
                // TODO: Compute number of shared characters & compare with max.
                CurrentInitrdName = CurrentInitrdName->Next;
            }
            if (MaxSharedInitrd)
                InitrdName = StrDuplicate(MaxSharedInitrd->Value);
        } // if/else
    } // if
    DeleteStringList(InitrdNames);

    // Note: Don't FreePool(FileName), since Basename returns a pointer WITHIN the string it's passed.
    MyFreePool(KernelVersion);
    MyFreePool(Path);
    return (InitrdName);
} // static CHAR16 * FindInitrd()

LOADER_ENTRY * AddPreparedLoaderEntry(LOADER_ENTRY *Entry) {
    AddMenuEntry(&MainMenu, (REFIT_MENU_ENTRY *)Entry);

    return(Entry);
} // LOADER_ENTRY * AddPreparedLoaderEntry()

// Creates a copy of a menu screen.
// Returns a pointer to the copy of the menu screen.
static REFIT_MENU_SCREEN* CopyMenuScreen(REFIT_MENU_SCREEN *Entry) {
    REFIT_MENU_SCREEN *NewEntry;
    UINTN i;

    NewEntry = AllocateZeroPool(sizeof(REFIT_MENU_SCREEN));
    if ((Entry != NULL) && (NewEntry != NULL)) {
        CopyMem(NewEntry, Entry, sizeof(REFIT_MENU_SCREEN));
        NewEntry->Title = (Entry->Title) ? StrDuplicate(Entry->Title) : NULL;
        NewEntry->TimeoutText = (Entry->TimeoutText) ? StrDuplicate(Entry->TimeoutText) : NULL;
        if (Entry->TitleImage != NULL) {
            NewEntry->TitleImage = AllocatePool(sizeof(EG_IMAGE));
            if (NewEntry->TitleImage != NULL)
                CopyMem(NewEntry->TitleImage, Entry->TitleImage, sizeof(EG_IMAGE));
        } // if
        NewEntry->InfoLines = (CHAR16**) AllocateZeroPool(Entry->InfoLineCount * (sizeof(CHAR16*)));
        for (i = 0; i < Entry->InfoLineCount && NewEntry->InfoLines; i++) {
            NewEntry->InfoLines[i] = (Entry->InfoLines[i]) ? StrDuplicate(Entry->InfoLines[i]) : NULL;
        } // for
        NewEntry->Entries = (REFIT_MENU_ENTRY**) AllocateZeroPool(Entry->EntryCount * (sizeof (REFIT_MENU_ENTRY*)));
        for (i = 0; i < Entry->EntryCount && NewEntry->Entries; i++) {
            AddMenuEntry(NewEntry, Entry->Entries[i]);
        } // for
        NewEntry->Hint1 = (Entry->Hint1) ? StrDuplicate(Entry->Hint1) : NULL;
        NewEntry->Hint2 = (Entry->Hint2) ? StrDuplicate(Entry->Hint2) : NULL;
    } // if
    return (NewEntry);
} // static REFIT_MENU_SCREEN* CopyMenuScreen()

// Creates a copy of a menu entry. Intended to enable moving a stack-based
// menu entry (such as the ones for the "reboot" and "exit" functions) to
// to the heap. This enables easier deletion of the whole set of menu
// entries when re-scanning.
// Returns a pointer to the copy of the menu entry.
static REFIT_MENU_ENTRY* CopyMenuEntry(REFIT_MENU_ENTRY *Entry) {
    REFIT_MENU_ENTRY *NewEntry;

    NewEntry = AllocateZeroPool(sizeof(REFIT_MENU_ENTRY));
    if ((Entry != NULL) && (NewEntry != NULL)) {
        CopyMem(NewEntry, Entry, sizeof(REFIT_MENU_ENTRY));
        NewEntry->Title = (Entry->Title) ? StrDuplicate(Entry->Title) : NULL;
        if (Entry->BadgeImage != NULL) {
            NewEntry->BadgeImage = AllocatePool(sizeof(EG_IMAGE));
            if (NewEntry->BadgeImage != NULL)
                CopyMem(NewEntry->BadgeImage, Entry->BadgeImage, sizeof(EG_IMAGE));
        }
        if (Entry->Image != NULL) {
            NewEntry->Image = AllocatePool(sizeof(EG_IMAGE));
            if (NewEntry->Image != NULL)
                CopyMem(NewEntry->Image, Entry->Image, sizeof(EG_IMAGE));
        }
        if (Entry->SubScreen != NULL) {
            NewEntry->SubScreen = CopyMenuScreen(Entry->SubScreen);
        }
    } // if
    return (NewEntry);
} // REFIT_MENU_ENTRY* CopyMenuEntry()

// Creates a new LOADER_ENTRY data structure and populates it with
// default values from the specified Entry, or NULL values if Entry
// is unspecified (NULL).
// Returns a pointer to the new data structure, or NULL if it
// couldn't be allocated
LOADER_ENTRY *InitializeLoaderEntry(IN LOADER_ENTRY *Entry) {
    LOADER_ENTRY *NewEntry = NULL;

    NewEntry = AllocateZeroPool(sizeof(LOADER_ENTRY));
    if (NewEntry != NULL) {
        NewEntry->me.Title        = NULL;
        NewEntry->me.Tag          = TAG_LOADER;
        NewEntry->Enabled         = TRUE;
        NewEntry->UseGraphicsMode = FALSE;
        NewEntry->OSType          = 0;
        if (Entry != NULL) {
            NewEntry->LoaderPath      = (Entry->LoaderPath) ? StrDuplicate(Entry->LoaderPath) : NULL;
            NewEntry->Volume          = Entry->Volume;
            NewEntry->UseGraphicsMode = Entry->UseGraphicsMode;
            NewEntry->LoadOptions     = (Entry->LoadOptions) ? StrDuplicate(Entry->LoadOptions) : NULL;
            NewEntry->InitrdPath      = (Entry->InitrdPath) ? StrDuplicate(Entry->InitrdPath) : NULL;
        }
    } // if
    return (NewEntry);
} // LOADER_ENTRY *InitializeLoaderEntry()

// Adds InitrdPath to Options, but only if Options doesn't already include an
// initrd= line. Done to enable overriding the default initrd selection in a
// refind_linux.conf file's options list.
// Returns a pointer to a new string. The calling function is responsible for
// freeing its memory.
static CHAR16 *AddInitrdToOptions(CHAR16 *Options, CHAR16 *InitrdPath) {
    CHAR16 *NewOptions = NULL;

    if (Options != NULL)
        NewOptions = StrDuplicate(Options);
    if ((InitrdPath != NULL) && !StriSubCmp(L"initrd=", Options)) {
        MergeStrings(&NewOptions, L"initrd=", L' ');
        MergeStrings(&NewOptions, InitrdPath, 0);
    }
    return NewOptions;
} // CHAR16 *AddInitrdToOptions()

// Prepare a REFIT_MENU_SCREEN data structure for a subscreen entry. This sets up
// the default entry that launches the boot loader using the same options as the
// main Entry does. Subsequent options can be added by the calling function.
// If a subscreen already exists in the Entry that's passed to this function,
// it's left unchanged and a pointer to it is returned.
// Returns a pointer to the new subscreen data structure, or NULL if there
// were problems allocating memory.
REFIT_MENU_SCREEN *InitializeSubScreen(IN LOADER_ENTRY *Entry) {
    CHAR16              *FileName, *MainOptions = NULL;
    REFIT_MENU_SCREEN   *SubScreen = NULL;
    LOADER_ENTRY        *SubEntry;

    FileName = Basename(Entry->LoaderPath);
    if (Entry->me.SubScreen == NULL) { // No subscreen yet; initialize default entry....
        SubScreen = AllocateZeroPool(sizeof(REFIT_MENU_SCREEN));
        if (SubScreen != NULL) {
            SubScreen->Title = AllocateZeroPool(sizeof(CHAR16) * 256);
            SPrint(SubScreen->Title, 255, L"Boot Options for %s on %s",
                   (Entry->Title != NULL) ? Entry->Title : FileName, Entry->Volume->VolName);
            SubScreen->TitleImage = Entry->me.Image;
            // default entry
            SubEntry = InitializeLoaderEntry(Entry);
            if (SubEntry != NULL) {
                SubEntry->me.Title = StrDuplicate(L"Boot using default options");
                MainOptions = SubEntry->LoadOptions;
                SubEntry->LoadOptions = AddInitrdToOptions(MainOptions, SubEntry->InitrdPath);
                MyFreePool(MainOptions);
                AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
            } // if (SubEntry != NULL)
            SubScreen->Hint1 = StrDuplicate(SUBSCREEN_HINT1);
            if (GlobalConfig.HideUIFlags & HIDEUI_FLAG_EDITOR) {
                SubScreen->Hint2 = StrDuplicate(SUBSCREEN_HINT2_NO_EDITOR);
            } else {
                SubScreen->Hint2 = StrDuplicate(SUBSCREEN_HINT2);
            } // if/else
        } // if (SubScreen != NULL)
    } else { // existing subscreen; less initialization, and just add new entry later....
        SubScreen = Entry->me.SubScreen;
    } // if/else
    return SubScreen;
} // REFIT_MENU_SCREEN *InitializeSubScreen()

VOID GenerateSubScreen(LOADER_ENTRY *Entry, IN REFIT_VOLUME *Volume, IN BOOLEAN GenerateReturn) {
    REFIT_MENU_SCREEN  *SubScreen;
    LOADER_ENTRY       *SubEntry;
    CHAR16             *InitrdName, *KernelVersion = NULL;
    CHAR16             DiagsFileName[256];
    REFIT_FILE         *File;
    UINTN              TokenCount;
    CHAR16             **TokenList;

    // create the submenu
    if (StrLen(Entry->Title) == 0) {
        MyFreePool(Entry->Title);
        Entry->Title = NULL;
    }
    SubScreen = InitializeSubScreen(Entry);

    // loader-specific submenu entries
    if (Entry->OSType == 'M') {          // entries for macOS
#if defined(EFIX64)
        SubEntry = InitializeLoaderEntry(Entry);
        if (SubEntry != NULL) {
            SubEntry->me.Title        = L"Boot macOS with a 64-bit kernel";
            SubEntry->LoadOptions     = L"arch=x86_64";
            SubEntry->UseGraphicsMode = GlobalConfig.GraphicsFor & GRAPHICS_FOR_OSX;
            AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
        } // if

        SubEntry = InitializeLoaderEntry(Entry);
        if (SubEntry != NULL) {
            SubEntry->me.Title        = L"Boot macOS with a 32-bit kernel";
            SubEntry->LoadOptions     = L"arch=i386";
            SubEntry->UseGraphicsMode = GlobalConfig.GraphicsFor & GRAPHICS_FOR_OSX;
            AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
        } // if
#endif

        if (!(GlobalConfig.HideUIFlags & HIDEUI_FLAG_SINGLEUSER)) {
            SubEntry = InitializeLoaderEntry(Entry);
            if (SubEntry != NULL) {
                SubEntry->me.Title        = L"Boot macOS in verbose mode";
                SubEntry->UseGraphicsMode = FALSE;
                SubEntry->LoadOptions     = L"-v";
                AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
            } // if

#if defined(EFIX64)
            SubEntry = InitializeLoaderEntry(Entry);
            if (SubEntry != NULL) {
                SubEntry->me.Title        = L"Boot macOS in verbose mode (64-bit)";
                SubEntry->UseGraphicsMode = FALSE;
                SubEntry->LoadOptions     = L"-v arch=x86_64";
                AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
            }

            SubEntry = InitializeLoaderEntry(Entry);
            if (SubEntry != NULL) {
                SubEntry->me.Title        = L"Boot macOS in verbose mode (32-bit)";
                SubEntry->UseGraphicsMode = FALSE;
                SubEntry->LoadOptions     = L"-v arch=i386";
                AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
            }
#endif

            SubEntry = InitializeLoaderEntry(Entry);
            if (SubEntry != NULL) {
                SubEntry->me.Title        = L"Boot macOS in single user mode";
                SubEntry->UseGraphicsMode = FALSE;
                SubEntry->LoadOptions     = L"-v -s";
                AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
            } // if
        } // single-user mode allowed

        if (!(GlobalConfig.HideUIFlags & HIDEUI_FLAG_SAFEMODE)) {
            SubEntry = InitializeLoaderEntry(Entry);
            if (SubEntry != NULL) {
                SubEntry->me.Title        = L"Boot macOS in safe mode";
                SubEntry->UseGraphicsMode = FALSE;
                SubEntry->LoadOptions     = L"-v -x";
                AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
            } // if
        } // safe mode allowed

        // check for Apple hardware diagnostics
        StrCpy(DiagsFileName, L"System\\Library\\CoreServices\\.diagnostics\\diags.efi");
        if (FileExists(Volume->RootDir, DiagsFileName) && !(GlobalConfig.HideUIFlags & HIDEUI_FLAG_HWTEST)) {
            SubEntry = InitializeLoaderEntry(Entry);
            if (SubEntry != NULL) {
                SubEntry->me.Title        = L"Run Apple Hardware Test";
                MyFreePool(SubEntry->LoaderPath);
                SubEntry->LoaderPath      = StrDuplicate(DiagsFileName);
                SubEntry->Volume          = Volume;
                SubEntry->UseGraphicsMode = GlobalConfig.GraphicsFor & GRAPHICS_FOR_OSX;
                AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
            } // if
        } // if diagnostics entry found

    } else if (Entry->OSType == 'L') {   // entries for Linux kernels with EFI stub loaders
        File = ReadLinuxOptionsFile(Entry->LoaderPath, Volume);
        if (File != NULL) {
            InitrdName =  FindInitrd(Entry->LoaderPath, Volume);
            TokenCount = ReadTokenLine(File, &TokenList);
            KernelVersion = FindNumbers(Entry->LoaderPath);
            ReplaceSubstring(&(TokenList[1]), KERNEL_VERSION, KernelVersion);
            // first entry requires special processing, since it was initially set
            // up with a default title but correct options by InitializeSubScreen(),
            // earlier....
            if ((TokenCount > 1) && (SubScreen->Entries != NULL) && (SubScreen->Entries[0] != NULL)) {
                MyFreePool(SubScreen->Entries[0]->Title);
                SubScreen->Entries[0]->Title = TokenList[0] ? StrDuplicate(TokenList[0]) : StrDuplicate(L"Boot Linux");
            } // if
            FreeTokenLine(&TokenList, &TokenCount);
            while ((TokenCount = ReadTokenLine(File, &TokenList)) > 1) {
                ReplaceSubstring(&(TokenList[1]), KERNEL_VERSION, KernelVersion);
                SubEntry = InitializeLoaderEntry(Entry);
                SubEntry->me.Title = TokenList[0] ? StrDuplicate(TokenList[0]) : StrDuplicate(L"Boot Linux");
                MyFreePool(SubEntry->LoadOptions);
                SubEntry->LoadOptions = AddInitrdToOptions(TokenList[1], InitrdName);
                FreeTokenLine(&TokenList, &TokenCount);
                SubEntry->UseGraphicsMode = GlobalConfig.GraphicsFor & GRAPHICS_FOR_LINUX;
                AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
            } // while
            MyFreePool(KernelVersion);
            MyFreePool(InitrdName);
            MyFreePool(File);
        } // if

    } else if (Entry->OSType == 'E') {   // entries for ELILO
        SubEntry = InitializeLoaderEntry(Entry);
        if (SubEntry != NULL) {
            SubEntry->me.Title        = L"Run ELILO in interactive mode";
            SubEntry->LoadOptions     = L"-p";
            SubEntry->UseGraphicsMode = GlobalConfig.GraphicsFor & GRAPHICS_FOR_ELILO;
            AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
        }

        SubEntry = InitializeLoaderEntry(Entry);
        if (SubEntry != NULL) {
            SubEntry->me.Title        = L"Boot Linux for a 17\" iMac or a 15\" MacBook Pro (*)";
            SubEntry->LoadOptions     = L"-d 0 i17";
            SubEntry->UseGraphicsMode = GlobalConfig.GraphicsFor & GRAPHICS_FOR_ELILO;
            AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
        }

        SubEntry = InitializeLoaderEntry(Entry);
        if (SubEntry != NULL) {
            SubEntry->me.Title        = L"Boot Linux for a 20\" iMac (*)";
            SubEntry->LoadOptions     = L"-d 0 i20";
            SubEntry->UseGraphicsMode = GlobalConfig.GraphicsFor & GRAPHICS_FOR_ELILO;
            AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
        }

        SubEntry = InitializeLoaderEntry(Entry);
        if (SubEntry != NULL) {
            SubEntry->me.Title        = L"Boot Linux for a Mac Mini (*)";
            SubEntry->LoadOptions     = L"-d 0 mini";
            SubEntry->UseGraphicsMode = GlobalConfig.GraphicsFor & GRAPHICS_FOR_ELILO;
            AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
        }

        AddMenuInfoLine(SubScreen, L"NOTE: This is an example. Entries");
        AddMenuInfoLine(SubScreen, L"marked with (*) may not work.");

    } else if (Entry->OSType == 'X') {   // entries for xom.efi
        // by default, skip the built-in selection and boot from hard disk only
        Entry->LoadOptions = L"-s -h";

        SubEntry = InitializeLoaderEntry(Entry);
        if (SubEntry != NULL) {
            SubEntry->me.Title        = L"Boot Windows from Hard Disk";
            SubEntry->LoadOptions     = L"-s -h";
            SubEntry->UseGraphicsMode = GlobalConfig.GraphicsFor & GRAPHICS_FOR_WINDOWS;
            AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
        }

        SubEntry = InitializeLoaderEntry(Entry);
        if (SubEntry != NULL) {
            SubEntry->me.Title        = L"Boot Windows from CD-ROM";
            SubEntry->LoadOptions     = L"-s -c";
            SubEntry->UseGraphicsMode = GlobalConfig.GraphicsFor & GRAPHICS_FOR_WINDOWS;
            AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
        }

        SubEntry = InitializeLoaderEntry(Entry);
        if (SubEntry != NULL) {
            SubEntry->me.Title        = L"Run XOM in text mode";
            SubEntry->LoadOptions     = L"-v";
            SubEntry->UseGraphicsMode = GlobalConfig.GraphicsFor & GRAPHICS_FOR_WINDOWS;
            AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
        }
    } // entries for xom.efi
    if (GenerateReturn)
        AddMenuEntry(SubScreen, &MenuEntryReturn);
    Entry->me.SubScreen = SubScreen;
} // VOID GenerateSubScreen()

// Returns options for a Linux kernel. Reads them from an options file in the
// kernel's directory; and if present, adds an initrd= option for an initial
// RAM disk file with the same version number as the kernel file.
static CHAR16 * GetMainLinuxOptions(IN CHAR16 * LoaderPath, IN REFIT_VOLUME *Volume) {
    CHAR16 *Options = NULL, *InitrdName, *FullOptions = NULL, *KernelVersion;

    Options = GetFirstOptionsFromFile(LoaderPath, Volume);
    InitrdName = FindInitrd(LoaderPath, Volume);
    KernelVersion = FindNumbers(InitrdName);
    ReplaceSubstring(&Options, KERNEL_VERSION, KernelVersion);
    FullOptions = AddInitrdToOptions(Options, InitrdName);

    MyFreePool(Options);
    MyFreePool(InitrdName);
    MyFreePool(KernelVersion);
    return (FullOptions);
} // static CHAR16 * GetMainLinuxOptions()

// Read the specified file and add values of "ID", "NAME", or "DISTRIB_ID" tokens to
// OSIconName list. Intended for adding Linux distribution clues gleaned from
// /etc/lsb-release and /etc/os-release files.
static VOID ParseReleaseFile(CHAR16 **OSIconName, REFIT_VOLUME *Volume, CHAR16 *FileName) {
    UINTN       FileSize = 0;
    REFIT_FILE  File;
    CHAR16      **TokenList;
    UINTN       TokenCount = 0;

    if ((Volume == NULL) || (FileName == NULL) || (OSIconName == NULL) || (*OSIconName == NULL))
        return;

    if (FileExists(Volume->RootDir, FileName) &&
        (ReadFile(Volume->RootDir, FileName, &File, &FileSize) == EFI_SUCCESS)) {
        do {
            TokenCount = ReadTokenLine(&File, &TokenList);
            if ((TokenCount > 1) && (MyStriCmp(TokenList[0], L"ID") ||
                                     MyStriCmp(TokenList[0], L"NAME") ||
                                     MyStriCmp(TokenList[0], L"DISTRIB_ID"))) {
                MergeWords(OSIconName, TokenList[1], L',');
            } // if
            FreeTokenLine(&TokenList, &TokenCount);
        } while (TokenCount > 0);
        MyFreePool(File.Buffer);
    } // if
} // VOID ParseReleaseFile()

// Try to guess the name of the Linux distribution & add that name to
// OSIconName list.
static VOID GuessLinuxDistribution(CHAR16 **OSIconName, REFIT_VOLUME *Volume, CHAR16 *LoaderPath) {
    // If on Linux root fs, /etc/os-release or /etc/lsb-release file probably has clues....
    ParseReleaseFile(OSIconName, Volume, L"etc\\lsb-release");
    ParseReleaseFile(OSIconName, Volume, L"etc\\os-release");

    // Search for clues in the kernel's filename....
    if (StriSubCmp(L".fc", LoaderPath))
        MergeStrings(OSIconName, L"fedora", L',');
    if (StriSubCmp(L".el", LoaderPath))
        MergeStrings(OSIconName, L"redhat", L',');
} // VOID GuessLinuxDistribution()

// Sets a few defaults for a loader entry -- mainly the icon, but also the OS type
// code and shortcut letter. For Linux EFI stub loaders, also sets kernel options
// that will (with luck) work fairly automatically.
VOID SetLoaderDefaults(LOADER_ENTRY *Entry, CHAR16 *LoaderPath, REFIT_VOLUME *Volume) {
    CHAR16      *NameClues, *PathOnly, *NoExtension, *OSIconName = NULL, *Temp;
    CHAR16      ShortcutLetter = 0;

    NameClues = Basename(LoaderPath);
    PathOnly = FindPath(LoaderPath);
    NoExtension = StripEfiExtension(NameClues);

    if (Volume->DiskKind == DISK_KIND_NET) {
        MergeStrings(&NameClues, Entry->me.Title, L' ');
    } else {
        // locate a custom icon for the loader
        // Anything found here takes precedence over the "hints" in the OSIconName variable
        if (!Entry->me.Image) {
            Entry->me.Image = egLoadIconAnyType(Volume->RootDir, PathOnly, NoExtension, GlobalConfig.IconSizes[ICON_SIZE_BIG]);
        }
        if (!Entry->me.Image) {
            Entry->me.Image = egCopyImage(Volume->VolIconImage);
        }

        // Begin creating icon "hints" by using last part of directory path leading
        // to the loader
        Temp = FindLastDirName(LoaderPath);
        MergeStrings(&OSIconName, Temp, L',');
        MyFreePool(Temp);
        Temp = NULL;
        if (OSIconName != NULL) {
            ShortcutLetter = OSIconName[0];
        }

        // Add every "word" in the volume label, delimited by spaces, dashes (-), or
        // underscores (_), to the list of hints to be used in searching for OS
        // icons.
        MergeWords(&OSIconName, Volume->VolName, L',');
    } // if/else network boot

    // detect specific loaders
    if (StriSubCmp(L"bzImage", NameClues) || StriSubCmp(L"vmlinuz", NameClues) || StriSubCmp(L"kernel", NameClues)) {
        if (Volume->DiskKind != DISK_KIND_NET) {
            GuessLinuxDistribution(&OSIconName, Volume, LoaderPath);
            Entry->LoadOptions = GetMainLinuxOptions(LoaderPath, Volume);
        }
        MergeStrings(&OSIconName, L"linux", L',');
        Entry->OSType = 'L';
        if (ShortcutLetter == 0)
            ShortcutLetter = 'L';
        Entry->UseGraphicsMode = GlobalConfig.GraphicsFor & GRAPHICS_FOR_LINUX;
    } else if (StriSubCmp(L"refit", LoaderPath)) {
        MergeStrings(&OSIconName, L"refit", L',');
        Entry->OSType = 'R';
        ShortcutLetter = 'R';
    } else if (StriSubCmp(L"refind", LoaderPath)) {
        MergeStrings(&OSIconName, L"refind", L',');
        Entry->OSType = 'R';
        ShortcutLetter = 'R';
    } else if (StriSubCmp(MACOSX_LOADER_PATH, LoaderPath)) {
        MergeStrings(&OSIconName, L"mac", L',');
        Entry->OSType = 'M';
        ShortcutLetter = 'M';
        Entry->UseGraphicsMode = GlobalConfig.GraphicsFor & GRAPHICS_FOR_OSX;
    } else if (MyStriCmp(NameClues, L"diags.efi")) {
        MergeStrings(&OSIconName, L"hwtest", L',');
    } else if (MyStriCmp(NameClues, L"e.efi") || MyStriCmp(NameClues, L"elilo.efi") || StriSubCmp(L"elilo", NameClues)) {
        MergeStrings(&OSIconName, L"elilo,linux", L',');
        Entry->OSType = 'E';
        if (ShortcutLetter == 0)
            ShortcutLetter = 'L';
        Entry->UseGraphicsMode = GlobalConfig.GraphicsFor & GRAPHICS_FOR_ELILO;
    } else if (StriSubCmp(L"grub", NameClues)) {
        MergeStrings(&OSIconName, L"grub,linux", L',');
        Entry->OSType = 'G';
        ShortcutLetter = 'G';
        Entry->UseGraphicsMode = GlobalConfig.GraphicsFor & GRAPHICS_FOR_GRUB;
    } else if (MyStriCmp(NameClues, L"cdboot.efi") ||
               MyStriCmp(NameClues, L"bootmgr.efi") ||
               MyStriCmp(NameClues, L"bootmgfw.efi") ||
               MyStriCmp(NameClues, L"bkpbootmgfw.efi")) {
        MergeStrings(&OSIconName, L"win8", L',');
        Entry->OSType = 'W';
        ShortcutLetter = 'W';
        Entry->UseGraphicsMode = GlobalConfig.GraphicsFor & GRAPHICS_FOR_WINDOWS;
    } else if (MyStriCmp(NameClues, L"xom.efi")) {
        MergeStrings(&OSIconName, L"xom,win,win8", L',');
        Entry->OSType = 'X';
        ShortcutLetter = 'W';
        Entry->UseGraphicsMode = GlobalConfig.GraphicsFor & GRAPHICS_FOR_WINDOWS;
    }
    else if (StriSubCmp(L"ipxe", NameClues)) {
        Entry->OSType = 'N';
        ShortcutLetter = 'N';
        MergeStrings(&OSIconName, L"network", L',');
    } 

    if ((ShortcutLetter >= 'a') && (ShortcutLetter <= 'z'))
        ShortcutLetter = ShortcutLetter - 'a' + 'A'; // convert lowercase to uppercase
    Entry->me.ShortcutLetter = ShortcutLetter;
    if (Entry->me.Image == NULL)
        Entry->me.Image = LoadOSIcon(OSIconName, L"unknown", FALSE);
    MyFreePool(PathOnly);
} // VOID SetLoaderDefaults()

// Add a specified EFI boot loader to the list, using automatic settings
// for icons, options, etc.
static LOADER_ENTRY * AddLoaderEntry(IN CHAR16 *LoaderPath, IN CHAR16 *LoaderTitle, IN REFIT_VOLUME *Volume, IN BOOLEAN SubScreenReturn) {
    LOADER_ENTRY  *Entry;

    CleanUpPathNameSlashes(LoaderPath);
    Entry = InitializeLoaderEntry(NULL);
    Entry->DiscoveryType = DISCOVERY_TYPE_AUTO;
    if (Entry != NULL) {
        Entry->Title = StrDuplicate((LoaderTitle != NULL) ? LoaderTitle : LoaderPath);
        Entry->me.Title = AllocateZeroPool(sizeof(CHAR16) * 256);
        // Extra space at end of Entry->me.Title enables searching on Volume->VolName even if another volume
        // name is identical except for something added to the end (e.g., VolB1 vs. VolB12).
        // Note: Volume->VolName will be NULL for network boot programs.
        if ((Volume->VolName) && (!MyStriCmp(Volume->VolName, L"Recovery HD")))
            SPrint(Entry->me.Title, 255, L"%s ", (LoaderTitle != NULL) ? LoaderTitle : LoaderPath);
        else
            SPrint(Entry->me.Title, 255, L"%s ", (LoaderTitle != NULL) ? LoaderTitle : LoaderPath);
        Entry->me.Row = 0;
        Entry->me.BadgeImage = Volume->VolBadgeImage;
        if ((LoaderPath != NULL) && (LoaderPath[0] != L'\\')) {
            Entry->LoaderPath = StrDuplicate(L"\\");
        } else {
            Entry->LoaderPath = NULL;
        }
        MergeStrings(&(Entry->LoaderPath), LoaderPath, 0);
        Entry->Volume = Volume;
        SetLoaderDefaults(Entry, LoaderPath, Volume);
        GenerateSubScreen(Entry, Volume, SubScreenReturn);
        AddMenuEntry(&MainMenu, (REFIT_MENU_ENTRY *)Entry);
    }

    return(Entry);
} // LOADER_ENTRY * AddLoaderEntry()

// Add a Linux kernel as a submenu entry for another (pre-existing) Linux kernel entry.
static VOID AddKernelToSubmenu(LOADER_ENTRY * TargetLoader, CHAR16 *FileName, REFIT_VOLUME *Volume) {
    REFIT_FILE          *File;
    CHAR16              **TokenList = NULL, *InitrdName, *SubmenuName = NULL, *VolName = NULL;
    CHAR16              *Path = NULL, *Title, *KernelVersion;
    REFIT_MENU_SCREEN   *SubScreen;
    LOADER_ENTRY        *SubEntry;
    UINTN               TokenCount;

    File = ReadLinuxOptionsFile(TargetLoader->LoaderPath, Volume);
    if (File != NULL) {
        SubScreen = TargetLoader->me.SubScreen;
        InitrdName = FindInitrd(FileName, Volume);
        KernelVersion = FindNumbers(FileName);
        while ((TokenCount = ReadTokenLine(File, &TokenList)) > 1) {
            ReplaceSubstring(&(TokenList[1]), KERNEL_VERSION, KernelVersion);
            SubEntry = InitializeLoaderEntry(TargetLoader);
            SplitPathName(FileName, &VolName, &Path, &SubmenuName);
            MergeStrings(&SubmenuName, L": ", '\0');
            MergeStrings(&SubmenuName, TokenList[0] ? StrDuplicate(TokenList[0]) : StrDuplicate(L"Boot Linux"), '\0');
            Title = StrDuplicate(SubmenuName);
            LimitStringLength(Title, MAX_LINE_LENGTH);
            SubEntry->me.Title = Title;
            MyFreePool(SubEntry->LoadOptions);
            SubEntry->LoadOptions = AddInitrdToOptions(TokenList[1], InitrdName);
            MyFreePool(SubEntry->LoaderPath);
            SubEntry->LoaderPath = StrDuplicate(FileName);
            CleanUpPathNameSlashes(SubEntry->LoaderPath);
            SubEntry->Volume = Volume;
            FreeTokenLine(&TokenList, &TokenCount);
            SubEntry->UseGraphicsMode = GlobalConfig.GraphicsFor & GRAPHICS_FOR_LINUX;
            AddMenuEntry(SubScreen, (REFIT_MENU_ENTRY *)SubEntry);
        } // while
        MyFreePool(VolName);
        MyFreePool(Path);
        MyFreePool(SubmenuName);
        MyFreePool(InitrdName);
        MyFreePool(File);
        MyFreePool(KernelVersion);
    } // if
} // static VOID AddKernelToSubmenu()

// Returns -1 if (Time1 < Time2), +1 if (Time1 > Time2), or 0 if
// (Time1 == Time2). Precision is only to the nearest second; since
// this is used for sorting boot loader entries, differences smaller
// than this are likely to be meaningless (and unlikely!).
INTN TimeComp(IN EFI_TIME *Time1, IN EFI_TIME *Time2) {
    INT64 Time1InSeconds, Time2InSeconds;

    // Following values are overestimates; I'm assuming 31 days in every month.
    // This is fine for the purpose of this function, which is limited
    Time1InSeconds = Time1->Second + (Time1->Minute * 60) + (Time1->Hour * 3600) + (Time1->Day * 86400) +
                        (Time1->Month * 2678400) + ((Time1->Year - 1998) * 32140800);
    Time2InSeconds = Time2->Second + (Time2->Minute * 60) + (Time2->Hour * 3600) + (Time2->Day * 86400) +
                        (Time2->Month * 2678400) + ((Time2->Year - 1998) * 32140800);
    if (Time1InSeconds < Time2InSeconds)
        return (-1);
    else if (Time1InSeconds > Time2InSeconds)
        return (1);

    return 0;
} // INTN TimeComp()

// Adds a loader list element, keeping it sorted by date. EXCEPTION: Fedora's rescue
// kernel, which begins with "vmlinuz-0-rescue," should not be at the top of the list,
// since that will make it the default if kernel folding is enabled, so float it to
// the end.
// Returns the new first element (the one with the most recent date).
static struct LOADER_LIST * AddLoaderListEntry(struct LOADER_LIST *LoaderList, struct LOADER_LIST *NewEntry) {
    struct LOADER_LIST *LatestEntry, *CurrentEntry, *PrevEntry = NULL;
    BOOLEAN LinuxRescue = FALSE;

    LatestEntry = CurrentEntry = LoaderList;
    if (LoaderList == NULL) {
        LatestEntry = NewEntry;
    } else {
        if (StriSubCmp(L"vmlinuz-0-rescue", NewEntry->FileName))
            LinuxRescue = TRUE;
        while ((CurrentEntry != NULL) && !StriSubCmp(L"vmlinuz-0-rescue", CurrentEntry->FileName) &&
               (LinuxRescue || (TimeComp(&(NewEntry->TimeStamp), &(CurrentEntry->TimeStamp)) < 0))) {
            PrevEntry = CurrentEntry;
            CurrentEntry = CurrentEntry->NextEntry;
        } // while
        NewEntry->NextEntry = CurrentEntry;
        if (PrevEntry == NULL) {
            LatestEntry = NewEntry;
        } else {
            PrevEntry->NextEntry = NewEntry;
        } // if/else
    } // if/else
    return (LatestEntry);
} // static VOID AddLoaderListEntry()

// Delete the LOADER_LIST linked list
static VOID CleanUpLoaderList(struct LOADER_LIST *LoaderList) {
    struct LOADER_LIST *Temp;

    while (LoaderList != NULL) {
        Temp = LoaderList;
        LoaderList = LoaderList->NextEntry;
        MyFreePool(Temp->FileName);
        MyFreePool(Temp);
    } // while
} // static VOID CleanUpLoaderList()

// Returns FALSE if the specified file/volume matches the GlobalConfig.DontScanDirs
// or GlobalConfig.DontScanVolumes specification, or if Path points to a volume
// other than the one specified by Volume, or if the specified path is SelfDir.
// Returns TRUE if none of these conditions is met -- that is, if the path is
// eligible for scanning.
static BOOLEAN ShouldScan(REFIT_VOLUME *Volume, CHAR16 *Path) {
    CHAR16   *VolName = NULL, *DontScanDir, *PathCopy = NULL, *VolGuid = NULL;
    UINTN    i = 0;
    BOOLEAN  ScanIt = TRUE;

    VolGuid = GuidAsString(&(Volume->PartGuid));
    if ((IsIn(Volume->VolName, GlobalConfig.DontScanVolumes)) || (IsIn(Volume->PartName, GlobalConfig.DontScanVolumes)) ||
        (IsIn(VolGuid, GlobalConfig.DontScanVolumes))) {
        MyFreePool(VolGuid);
        return FALSE;
    } else {
        MyFreePool(VolGuid);
    } // if/else

    if (MyStriCmp(Path, SelfDirPath) && (Volume->DeviceHandle == SelfVolume->DeviceHandle))
        return FALSE;

    // See if Path includes an explicit volume declaration that's NOT Volume....
    PathCopy = StrDuplicate(Path);
    if (SplitVolumeAndFilename(&PathCopy, &VolName)) {
        if (VolName && !MyStriCmp(VolName, Volume->VolName)) {
            ScanIt = FALSE;
        } // if
    } // if Path includes volume specification
    MyFreePool(PathCopy);
    MyFreePool(VolName);
    VolName = NULL;

    // See if Volume is in GlobalConfig.DontScanDirs....
    while (ScanIt && (DontScanDir = FindCommaDelimited(GlobalConfig.DontScanDirs, i++))) {
        SplitVolumeAndFilename(&DontScanDir, &VolName);
        CleanUpPathNameSlashes(DontScanDir);
        if (VolName != NULL) {
            if (VolumeMatchesDescription(Volume, VolName) && MyStriCmp(DontScanDir, Path))
                ScanIt = FALSE;
        } else {
            if (MyStriCmp(DontScanDir, Path))
                ScanIt = FALSE;
        }
        MyFreePool(DontScanDir);
        MyFreePool(VolName);
        DontScanDir = NULL;
        VolName = NULL;
    } // while()

    return ScanIt;
} // BOOLEAN ShouldScan()

// Returns TRUE if the file is byte-for-byte identical with the fallback file
// on the volume AND if the file is not itself the fallback file; returns
// FALSE if the file is not identical to the fallback file OR if the file
// IS the fallback file. Intended for use in excluding the fallback boot
// loader when it's a duplicate of another boot loader.
static BOOLEAN DuplicatesFallback(IN REFIT_VOLUME *Volume, IN CHAR16 *FileName) {
    CHAR8           *FileContents, *FallbackContents;
    EFI_FILE_HANDLE FileHandle, FallbackHandle;
    EFI_FILE_INFO   *FileInfo, *FallbackInfo;
    UINTN           FileSize = 0, FallbackSize = 0;
    EFI_STATUS      Status;
    BOOLEAN         AreIdentical = FALSE;

    if (!FileExists(Volume->RootDir, FileName) || !FileExists(Volume->RootDir, FALLBACK_FULLNAME))
        return FALSE;

    CleanUpPathNameSlashes(FileName);

    if (MyStriCmp(FileName, FALLBACK_FULLNAME))
        return FALSE; // identical filenames, so not a duplicate....

    Status = refit_call5_wrapper(Volume->RootDir->Open, Volume->RootDir, &FileHandle, FileName, EFI_FILE_MODE_READ, 0);
    if (Status == EFI_SUCCESS) {
        FileInfo = LibFileInfo(FileHandle);
        FileSize = FileInfo->FileSize;
    } else {
        return FALSE;
    }

    Status = refit_call5_wrapper(Volume->RootDir->Open, Volume->RootDir, &FallbackHandle, FALLBACK_FULLNAME, EFI_FILE_MODE_READ, 0);
    if (Status == EFI_SUCCESS) {
        FallbackInfo = LibFileInfo(FallbackHandle);
        FallbackSize = FallbackInfo->FileSize;
    } else {
        refit_call1_wrapper(FileHandle->Close, FileHandle);
        return FALSE;
    }

    if (FallbackSize == FileSize) { // could be identical; do full check....
        FileContents = AllocatePool(FileSize);
        FallbackContents = AllocatePool(FallbackSize);
        if (FileContents && FallbackContents) {
            Status = refit_call3_wrapper(FileHandle->Read, FileHandle, &FileSize, FileContents);
            if (Status == EFI_SUCCESS) {
                Status = refit_call3_wrapper(FallbackHandle->Read, FallbackHandle, &FallbackSize, FallbackContents);
            }
            if (Status == EFI_SUCCESS) {
                AreIdentical = (CompareMem(FileContents, FallbackContents, FileSize) == 0);
            } // if
        } // if
        MyFreePool(FileContents);
        MyFreePool(FallbackContents);
    } // if/else

    // BUG ALERT: Some systems (e.g., DUET, some Macs with large displays) crash if the
    // following two calls are reversed. Go figure....
    refit_call1_wrapper(FileHandle->Close, FallbackHandle);
    refit_call1_wrapper(FileHandle->Close, FileHandle);
    return AreIdentical;
} // BOOLEAN DuplicatesFallback()

// Returns FALSE if two measures of file size are identical for a single file,
// TRUE if not or if the file can't be opened and the other measure is non-0.
// Despite the function's name, this isn't really a direct test of symbolic
// link status, since EFI doesn't officially support symlinks. It does seem
// to be a reliable indicator, though. (OTOH, some disk errors might cause a
// file to fail to open, which would return a false positive -- but as I use
// this function to exclude symbolic links from the list of boot loaders,
// that would be fine, since such boot loaders wouldn't work.)
// CAUTION: *FullName MUST be properly cleaned up (via CleanUpPathNameSlashes())
static BOOLEAN IsSymbolicLink(REFIT_VOLUME *Volume, CHAR16 *FullName, EFI_FILE_INFO *DirEntry) {
    EFI_FILE_HANDLE FileHandle;
    EFI_FILE_INFO   *FileInfo = NULL;
    EFI_STATUS      Status;
    UINTN           FileSize2 = 0;

    Status = refit_call5_wrapper(Volume->RootDir->Open, Volume->RootDir, &FileHandle, FullName, EFI_FILE_MODE_READ, 0);
    if (Status == EFI_SUCCESS) {
        FileInfo = LibFileInfo(FileHandle);
        if (FileInfo != NULL)
            FileSize2 = FileInfo->FileSize;
    }

    MyFreePool(FileInfo);

    return (DirEntry->FileSize != FileSize2);
} // BOOLEAN IsSymbolicLink()

// Returns TRUE if a file with the same name as the original but with
// ".efi.signed" is also present in the same directory. Ubuntu is using
// this filename as a signed version of the original unsigned kernel, and
// there's no point in cluttering the display with two kernels that will
// behave identically on non-SB systems, or when one will fail when SB
// is active.
// CAUTION: *FullName MUST be properly cleaned up (via CleanUpPathNameSlashes())
static BOOLEAN HasSignedCounterpart(IN REFIT_VOLUME *Volume, IN CHAR16 *FullName) {
    CHAR16 *NewFile = NULL;
    BOOLEAN retval = FALSE;

    MergeStrings(&NewFile, FullName, 0);
    MergeStrings(&NewFile, L".efi.signed", 0);
    if (NewFile != NULL) {
        if (FileExists(Volume->RootDir, NewFile))
            retval = TRUE;
        MyFreePool(NewFile);
    } // if

    return retval;
} // BOOLEAN HasSignedCounterpart()

// Scan an individual directory for EFI boot loader files and, if found,
// add them to the list. Exception: Ignores FALLBACK_FULLNAME, which is picked
// up in ScanEfiFiles(). Sorts the entries within the loader directory so that
// the most recent one appears first in the list.
// Returns TRUE if a duplicate for FALLBACK_FILENAME was found, FALSE if not.
static BOOLEAN ScanLoaderDir(IN REFIT_VOLUME *Volume, IN CHAR16 *Path, IN CHAR16 *Pattern)
{
    EFI_STATUS              Status;
    REFIT_DIR_ITER          DirIter;
    EFI_FILE_INFO           *DirEntry;
    CHAR16                  Message[256], *Extension, *FullName;
    struct LOADER_LIST      *LoaderList = NULL, *NewLoader;
    LOADER_ENTRY            *FirstKernel = NULL, *LatestEntry = NULL;
    BOOLEAN                 FoundFallbackDuplicate = FALSE, IsLinux = FALSE, InSelfPath;

    InSelfPath = MyStriCmp(Path, SelfDirPath);
    if ((!SelfDirPath || !Path || (InSelfPath && (Volume->DeviceHandle != SelfVolume->DeviceHandle)) ||
           (!InSelfPath)) && (ShouldScan(Volume, Path))) {
       // look through contents of the directory
       DirIterOpen(Volume->RootDir, Path, &DirIter);
       while (DirIterNext(&DirIter, 2, Pattern, &DirEntry)) {
          Extension = FindExtension(DirEntry->FileName);
          FullName = StrDuplicate(Path);
          MergeStrings(&FullName, DirEntry->FileName, L'\\');
          CleanUpPathNameSlashes(FullName);
          if (DirEntry->FileName[0] == '.' ||
              MyStriCmp(Extension, L".icns") ||
              MyStriCmp(Extension, L".png") ||
              (MyStriCmp(DirEntry->FileName, FALLBACK_BASENAME) && (MyStriCmp(Path, L"EFI\\BOOT"))) ||
              FilenameIn(Volume, Path, DirEntry->FileName, SHELL_NAMES) ||
              IsSymbolicLink(Volume, FullName, DirEntry) || /* is symbolic link */
              HasSignedCounterpart(Volume, FullName) || /* a file with same name plus ".efi.signed" is present */
              FilenameIn(Volume, Path, DirEntry->FileName, GlobalConfig.DontScanFiles) ||
              !IsValidLoader(Volume->RootDir, FullName)) {
                continue;   // skip this
          }

          NewLoader = AllocateZeroPool(sizeof(struct LOADER_LIST));
          if (NewLoader != NULL) {
             NewLoader->FileName = StrDuplicate(FullName);
             NewLoader->TimeStamp = DirEntry->ModificationTime;
             LoaderList = AddLoaderListEntry(LoaderList, NewLoader);
             if (DuplicatesFallback(Volume, FullName))
                FoundFallbackDuplicate = TRUE;
          } // if
          MyFreePool(Extension);
          MyFreePool(FullName);
       } // while

       NewLoader = LoaderList;
       while (NewLoader != NULL) {
           IsLinux = (StriSubCmp(L"bzImage", NewLoader->FileName) ||
                      StriSubCmp(L"vmlinuz", NewLoader->FileName) ||
                      StriSubCmp(L"kernel", NewLoader->FileName));
           if ((FirstKernel != NULL) && IsLinux && GlobalConfig.FoldLinuxKernels) {
               AddKernelToSubmenu(FirstKernel, NewLoader->FileName, Volume);
           } else {
               LatestEntry = AddLoaderEntry(NewLoader->FileName, NULL, Volume, !(IsLinux && GlobalConfig.FoldLinuxKernels));
               if (IsLinux && (FirstKernel == NULL))
                   FirstKernel = LatestEntry;
           }
           NewLoader = NewLoader->NextEntry;
       } // while
       if ((NewLoader != NULL) && (FirstKernel != NULL) && IsLinux && GlobalConfig.FoldLinuxKernels)
           AddMenuEntry(FirstKernel->me.SubScreen, &MenuEntryReturn);

       CleanUpLoaderList(LoaderList);
       Status = DirIterClose(&DirIter);
       // NOTE: EFI_INVALID_PARAMETER really is an error that should be reported;
       // but I've gotten reports from users who are getting this error occasionally
       // and I can't find anything wrong or reproduce the problem, so I'm putting
       // it down to buggy EFI implementations and ignoring that particular error....
       if ((Status != EFI_NOT_FOUND) && (Status != EFI_INVALID_PARAMETER)) {
          if (Path)
             SPrint(Message, 255, L"while scanning the %s directory on %s", Path, Volume->VolName);
          else
             SPrint(Message, 255, L"while scanning the root directory on %s", Path, Volume->VolName);
          CheckError(Status, Message);
       } // if (Status != EFI_NOT_FOUND)
    } // if not scanning a blacklisted directory

    return FoundFallbackDuplicate;
} /* static VOID ScanLoaderDir() */

// Run the IPXE_DISCOVER_NAME program, which obtains the IP address of the boot
// server and the name of the boot file it delivers.
CHAR16* RuniPXEDiscover(EFI_HANDLE Volume)
{
    EFI_STATUS       Status;
    EFI_DEVICE_PATH  *FilePath;
    EFI_HANDLE       iPXEHandle;
    CHAR16           *boot_info = NULL;
    UINTN            boot_info_size = 0;

    FilePath = FileDevicePath (Volume, IPXE_DISCOVER_NAME);
    Status = refit_call6_wrapper(BS->LoadImage, FALSE, SelfImageHandle, FilePath, NULL, 0, &iPXEHandle);
    if (Status != 0)
        return NULL;

    Status = refit_call3_wrapper(BS->StartImage, iPXEHandle, &boot_info_size, &boot_info);

    return boot_info;
} // RuniPXEDiscover()

// Scan for network (PXE) boot servers. This function relies on the presence
// of the IPXE_DISCOVER_NAME and IPXE_NAME program files on the volume from
// which rEFInd launched. As of December 6, 2014, these tools aren't entirely
// reliable. See BUILDING.txt for information on building them.
static VOID ScanNetboot() {
    CHAR16        *iPXEFileName = IPXE_NAME;
    CHAR16        *Location;
    REFIT_VOLUME  *NetVolume;

    if (FileExists(SelfVolume->RootDir, IPXE_DISCOVER_NAME) &&
        FileExists(SelfVolume->RootDir, IPXE_NAME) &&
        IsValidLoader(SelfVolume->RootDir, IPXE_DISCOVER_NAME) &&
        IsValidLoader(SelfVolume->RootDir, IPXE_NAME)) {
            Location = RuniPXEDiscover(SelfVolume->DeviceHandle);
            if (Location != NULL && FileExists(SelfVolume->RootDir, iPXEFileName)) {
                NetVolume = AllocatePool(sizeof(REFIT_VOLUME));
                CopyMem(NetVolume, SelfVolume, sizeof(REFIT_VOLUME));
                NetVolume->DiskKind = DISK_KIND_NET;
                NetVolume->VolBadgeImage = BuiltinIcon(BUILTIN_ICON_VOL_NET);
                NetVolume->PartName = NetVolume->VolName = NULL;
                AddLoaderEntry(iPXEFileName, Location, NetVolume, TRUE);
                MyFreePool(NetVolume);
            } // if support files exist and are valid
    } 
} // VOID ScanNetBoot()

// Adds *FullFileName as a macOS loader, if it exists.
// Returns TRUE if the fallback loader is NOT a duplicate of this one,
// FALSE if it IS a duplicate.
static BOOLEAN ScanMacOsLoader(REFIT_VOLUME *Volume, CHAR16* FullFileName) {
    BOOLEAN  ScanFallbackLoader = TRUE;
    CHAR16   *VolName = NULL, *PathName = NULL, *FileName = NULL;

    SplitPathName(FullFileName, &VolName, &PathName, &FileName);
    if (FileExists(Volume->RootDir, FullFileName) && !FilenameIn(Volume, PathName, L"boot.efi", GlobalConfig.DontScanFiles)) {
        AddLoaderEntry(FullFileName, L"macOS", Volume, TRUE);
        if (DuplicatesFallback(Volume, FullFileName))
            ScanFallbackLoader = FALSE;
    } // if
    MyFreePool(VolName);
    MyFreePool(PathName);
    MyFreePool(FileName);
    return ScanFallbackLoader;
} // VOID ScanMacOsLoader()

static VOID ScanEfiFiles(REFIT_VOLUME *Volume) {
    EFI_STATUS              Status;
    REFIT_DIR_ITER          EfiDirIter;
    EFI_FILE_INFO           *EfiDirEntry;
    CHAR16                  FileName[256], *Directory = NULL, *MatchPatterns, *VolName = NULL, *SelfPath, *Temp;
    UINTN                   i, Length;
    BOOLEAN                 ScanFallbackLoader = TRUE;
    BOOLEAN                 FoundBRBackup = FALSE;

    if (Volume && (Volume->RootDir != NULL) && (Volume->VolName != NULL) && (Volume->IsReadable)) {
        MatchPatterns = StrDuplicate(LOADER_MATCH_PATTERNS);
        if (GlobalConfig.ScanAllLinux)
            MergeStrings(&MatchPatterns, LINUX_MATCH_PATTERNS, L',');

        // check for macOS boot loader
        if (ShouldScan(Volume, MACOSX_LOADER_DIR)) {
            StrCpy(FileName, MACOSX_LOADER_PATH);
            ScanFallbackLoader &= ScanMacOsLoader(Volume, FileName);
            DirIterOpen(Volume->RootDir, L"\\", &EfiDirIter);
            while (DirIterNext(&EfiDirIter, 1, NULL, &EfiDirEntry)) {
                if (IsGuid(EfiDirEntry->FileName)) {
                    SPrint(FileName, 255, L"%s\\%s", EfiDirEntry->FileName, MACOSX_LOADER_PATH);
                    ScanFallbackLoader &= ScanMacOsLoader(Volume, FileName);
                    SPrint(FileName, 255, L"%s\\%s", EfiDirEntry->FileName, L"boot.efi");
                    if (!StriSubCmp(FileName, GlobalConfig.MacOSRecoveryFiles))
                        MergeStrings(&GlobalConfig.MacOSRecoveryFiles, FileName, L',');
                } // if
            } // while
            Status = DirIterClose(&EfiDirIter);

            // check for XOM
            StrCpy(FileName, L"System\\Library\\CoreServices\\xom.efi");
            if (FileExists(Volume->RootDir, FileName) && !FilenameIn(Volume, MACOSX_LOADER_DIR, L"xom.efi", GlobalConfig.DontScanFiles)) {
                AddLoaderEntry(FileName, L"Windows XP (XoM)", Volume, TRUE);
                if (DuplicatesFallback(Volume, FileName))
                    ScanFallbackLoader = FALSE;
            }
        } // if should scan Mac directory

        // check for Microsoft boot loader/menu
        if (ShouldScan(Volume, L"EFI\\Microsoft\\Boot")) {
            StrCpy(FileName, L"EFI\\Microsoft\\Boot\\bkpbootmgfw.efi");
            if (FileExists(Volume->RootDir, FileName) &&  !FilenameIn(Volume, L"EFI\\Microsoft\\Boot", L"bkpbootmgfw.efi",
                GlobalConfig.DontScanFiles)) {
                    AddLoaderEntry(FileName, L"Microsoft EFI boot (Boot Repair backup)", Volume, TRUE);
                    FoundBRBackup = TRUE;
                    if (DuplicatesFallback(Volume, FileName))
                        ScanFallbackLoader = FALSE;
            }
            StrCpy(FileName, L"EFI\\Microsoft\\Boot\\bootmgfw.efi");
            if (FileExists(Volume->RootDir, FileName) &&
                !FilenameIn(Volume, L"EFI\\Microsoft\\Boot", L"bootmgfw.efi", GlobalConfig.DontScanFiles)) {
                    if (FoundBRBackup)
                        AddLoaderEntry(FileName, L"Supposed Microsoft EFI boot (probably GRUB)", Volume, TRUE);
                    else
                        AddLoaderEntry(FileName, L"Microsoft EFI boot", Volume, TRUE);
                    if (DuplicatesFallback(Volume, FileName))
                        ScanFallbackLoader = FALSE;
            }
        } // if

        // scan the root directory for EFI executables
        if (ScanLoaderDir(Volume, L"\\", MatchPatterns))
            ScanFallbackLoader = FALSE;

        // scan subdirectories of the EFI directory (as per the standard)
        DirIterOpen(Volume->RootDir, L"EFI", &EfiDirIter);
        while (DirIterNext(&EfiDirIter, 1, NULL, &EfiDirEntry)) {
            if (MyStriCmp(EfiDirEntry->FileName, L"tools") || EfiDirEntry->FileName[0] == '.')
                continue;   // skip this, doesn't contain boot loaders or is scanned later
            SPrint(FileName, 255, L"EFI\\%s", EfiDirEntry->FileName);
            if (ScanLoaderDir(Volume, FileName, MatchPatterns))
                ScanFallbackLoader = FALSE;
        } // while()
        Status = DirIterClose(&EfiDirIter);
        if ((Status != EFI_NOT_FOUND) && (Status != EFI_INVALID_PARAMETER)) {
            Temp = PoolPrint(L"while scanning the EFI directory on %s", Volume->VolName);
            CheckError(Status, Temp);
            MyFreePool(Temp);
        } // if

        // Scan user-specified (or additional default) directories....
        i = 0;
        while ((Directory = FindCommaDelimited(GlobalConfig.AlsoScan, i++)) != NULL) {
            if (ShouldScan(Volume, Directory)) {
                SplitVolumeAndFilename(&Directory, &VolName);
                CleanUpPathNameSlashes(Directory);
                Length = StrLen(Directory);
                if ((Length > 0) && ScanLoaderDir(Volume, Directory, MatchPatterns))
                    ScanFallbackLoader = FALSE;
                MyFreePool(VolName);
            } // if should scan dir
            MyFreePool(Directory);
        } // while

        // Don't scan the fallback loader if it's on the same volume and a duplicate of rEFInd itself....
        SelfPath = DevicePathToStr(SelfLoadedImage->FilePath);
        CleanUpPathNameSlashes(SelfPath);
        if ((Volume->DeviceHandle == SelfLoadedImage->DeviceHandle) && DuplicatesFallback(Volume, SelfPath))
            ScanFallbackLoader = FALSE;

        // If not a duplicate & if it exists & if it's not us, create an entry
        // for the fallback boot loader
        if (ScanFallbackLoader && FileExists(Volume->RootDir, FALLBACK_FULLNAME) && ShouldScan(Volume, L"EFI\\BOOT") &&
            !FilenameIn(Volume, L"EFI\\BOOT", FALLBACK_BASENAME, GlobalConfig.DontScanFiles)) {
                AddLoaderEntry(FALLBACK_FULLNAME, L"Fallback boot loader", Volume, TRUE);
        }
    } // if
} // static VOID ScanEfiFiles()

// Scan internal disks for valid EFI boot loaders....
static VOID ScanInternal(VOID) {
    UINTN                   VolumeIndex;

    for (VolumeIndex = 0; VolumeIndex < VolumesCount; VolumeIndex++) {
        if (Volumes[VolumeIndex]->DiskKind == DISK_KIND_INTERNAL) {
            ScanEfiFiles(Volumes[VolumeIndex]);
        }
    } // for
} // static VOID ScanInternal()

// Scan external disks for valid EFI boot loaders....
static VOID ScanExternal(VOID) {
    UINTN                   VolumeIndex;

    for (VolumeIndex = 0; VolumeIndex < VolumesCount; VolumeIndex++) {
        if (Volumes[VolumeIndex]->DiskKind == DISK_KIND_EXTERNAL) {
            ScanEfiFiles(Volumes[VolumeIndex]);
        }
    } // for
} // static VOID ScanExternal()

// Scan internal disks for valid EFI boot loaders....
static VOID ScanOptical(VOID) {
    UINTN                   VolumeIndex;

    for (VolumeIndex = 0; VolumeIndex < VolumesCount; VolumeIndex++) {
        if (Volumes[VolumeIndex]->DiskKind == DISK_KIND_OPTICAL) {
            ScanEfiFiles(Volumes[VolumeIndex]);
        }
    } // for
} // static VOID ScanOptical()

// default volume badge icon based on disk kind
EG_IMAGE * GetDiskBadge(IN UINTN DiskType) {
    EG_IMAGE * Badge = NULL;

    switch (DiskType) {
        case BBS_HARDDISK:
            Badge = BuiltinIcon(BUILTIN_ICON_VOL_INTERNAL);
            break;
        case BBS_USB:
            Badge = BuiltinIcon(BUILTIN_ICON_VOL_EXTERNAL);
            break;
        case BBS_CDROM:
            Badge = BuiltinIcon(BUILTIN_ICON_VOL_OPTICAL);
            break;
    } // switch()
    return Badge;
} // EG_IMAGE * GetDiskBadge()

//
// pre-boot tool functions
//

static VOID StartTool(IN LOADER_ENTRY *Entry)
{
    BeginExternalScreen(Entry->UseGraphicsMode, Entry->me.Title + 6);  // assumes "Start <title>" as assigned below
    StoreLoaderName(Entry->me.Title);
    StartEFIImage(Entry->Volume, Entry->LoaderPath, Entry->LoadOptions,
                  Basename(Entry->LoaderPath), Entry->OSType, TRUE, FALSE);
    FinishExternalScreen();
} /* static VOID StartTool() */

static LOADER_ENTRY * AddToolEntry(REFIT_VOLUME *Volume,IN CHAR16 *LoaderPath, IN CHAR16 *LoaderTitle,
                                   IN EG_IMAGE *Image, IN CHAR16 ShortcutLetter, IN BOOLEAN UseGraphicsMode)
{
    LOADER_ENTRY *Entry;
    CHAR16       *TitleStr = NULL;

    Entry = AllocateZeroPool(sizeof(LOADER_ENTRY));

    TitleStr = PoolPrint(L"Start %s", LoaderTitle);
    Entry->me.Title = TitleStr;
    Entry->me.Tag = TAG_TOOL;
    Entry->me.Row = 1;
    Entry->me.ShortcutLetter = ShortcutLetter;
    Entry->me.Image = Image;
    Entry->LoaderPath = (LoaderPath) ? StrDuplicate(LoaderPath) : NULL;
    Entry->Volume = Volume;
    Entry->UseGraphicsMode = UseGraphicsMode;

    AddMenuEntry(&MainMenu, (REFIT_MENU_ENTRY *)Entry);
    return Entry;
} /* static LOADER_ENTRY * AddToolEntry() */

// Locates boot loaders. NOTE: This assumes that GlobalConfig.LegacyType is set correctly.
static VOID ScanForBootloaders(BOOLEAN ShowMessage) {
    UINTN    i;
    CHAR8    s;
    BOOLEAN  ScanForLegacy = FALSE;
    EG_PIXEL BGColor = COLOR_LIGHTBLUE;
    CHAR16   *HiddenTags;

    if (ShowMessage)
        egDisplayMessage(L"Scanning for boot loaders; please wait....", &BGColor, CENTER);

    // Determine up-front if we'll be scanning for legacy loaders....
    for (i = 0; i < NUM_SCAN_OPTIONS; i++) {
        s = GlobalConfig.ScanFor[i];
        if ((s == 'c') || (s == 'C') || (s == 'h') || (s == 'H') || (s == 'b') || (s == 'B'))
            ScanForLegacy = TRUE;
    } // for

    // If UEFI & scanning for legacy loaders & deep legacy scan, update NVRAM boot manager list
    if ((GlobalConfig.LegacyType == LEGACY_TYPE_UEFI) && ScanForLegacy && GlobalConfig.DeepLegacyScan) {
        BdsDeleteAllInvalidLegacyBootOptions();
        BdsAddNonExistingLegacyBootOptions();
    } // if

    HiddenTags = ReadHiddenTags(L"HiddenTags");
    if ((HiddenTags) && (StrLen(HiddenTags) > 0)) {
        MergeStrings(&GlobalConfig.DontScanFiles, HiddenTags, L',');
    }
    HiddenTags = ReadHiddenTags(L"HiddenLegacy");
    if ((HiddenTags) && (StrLen(HiddenTags) > 0)) {
        MergeStrings(&GlobalConfig.DontScanVolumes, HiddenTags, L',');
    }
    MyFreePool(HiddenTags);

    // scan for loaders and tools, add them to the menu
    for (i = 0; i < NUM_SCAN_OPTIONS; i++) {
        switch(GlobalConfig.ScanFor[i]) {
            case 'c': case 'C':
                ScanLegacyDisc();
                break;
            case 'h': case 'H':
                ScanLegacyInternal();
                break;
            case 'b': case 'B':
                ScanLegacyExternal();
                break;
            case 'm': case 'M':
                ScanUserConfigured(GlobalConfig.ConfigFilename);
                break;
            case 'e': case 'E':
                ScanExternal();
                break;
            case 'i': case 'I':
                ScanInternal();
                break;
            case 'o': case 'O':
                ScanOptical();
                break;
            case 'n': case 'N':
                ScanNetboot();
                break;
        } // switch()
    } // for

    // assign shortcut keys
    for (i = 0; i < MainMenu.EntryCount && MainMenu.Entries[i]->Row == 0 && i < 9; i++)
        MainMenu.Entries[i]->ShortcutDigit = (CHAR16)('1' + i);

    // wait for user ACK when there were errors
    FinishTextScreen(FALSE);
} // static VOID ScanForBootloaders()

// Checks to see if a specified file seems to be a valid tool.
// Returns TRUE if it passes all tests, FALSE otherwise
static BOOLEAN IsValidTool(IN REFIT_VOLUME *BaseVolume, CHAR16 *PathName) {
    CHAR16 *DontVolName = NULL, *DontPathName = NULL, *DontFileName = NULL, *DontScanThis;
    CHAR16 *TestVolName = NULL, *TestPathName = NULL, *TestFileName = NULL;
    BOOLEAN retval = TRUE;
    UINTN i = 0;

    if (FileExists(BaseVolume->RootDir, PathName) && IsValidLoader(BaseVolume->RootDir, PathName)) {
        SplitPathName(PathName, &TestVolName, &TestPathName, &TestFileName);
        while (retval && (DontScanThis = FindCommaDelimited(GlobalConfig.DontScanTools, i++))) {
            SplitPathName(DontScanThis, &DontVolName, &DontPathName, &DontFileName);
            if (MyStriCmp(TestFileName, DontFileName) &&
                ((DontPathName == NULL) || (MyStriCmp(TestPathName, DontPathName))) &&
                ((DontVolName == NULL) || (VolumeMatchesDescription(BaseVolume, DontVolName)))) {
                retval = FALSE;
            } // if
        } // while
    } else
        retval = FALSE;
    MyFreePool(TestVolName);
    MyFreePool(TestPathName);
    MyFreePool(TestFileName);
    return retval;
} // VOID IsValidTool()

// Locate a single tool from the specified Locations using one of the
// specified Names and add it to the menu.
static VOID FindTool(CHAR16 *Locations, CHAR16 *Names, CHAR16 *Description, UINTN Icon) {
    UINTN j = 0, k, VolumeIndex;
    CHAR16 *DirName, *FileName, *PathName, FullDescription[256];

    while ((DirName = FindCommaDelimited(Locations, j++)) != NULL) {
        k = 0;
        while ((FileName = FindCommaDelimited(Names, k++)) != NULL) {
            PathName = StrDuplicate(DirName);
            MergeStrings(&PathName, FileName, MyStriCmp(PathName, L"\\") ? 0 : L'\\');
            for (VolumeIndex = 0; VolumeIndex < VolumesCount; VolumeIndex++) {
                if ((Volumes[VolumeIndex]->RootDir != NULL) && (IsValidTool(Volumes[VolumeIndex], PathName))) {
                    SPrint(FullDescription, 255, L"%s at %s on %s", Description, PathName, Volumes[VolumeIndex]->VolName);
                    AddToolEntry(Volumes[VolumeIndex], PathName, FullDescription, BuiltinIcon(Icon), 'S', FALSE);
                } // if
            } // for
            MyFreePool(PathName);
            MyFreePool(FileName);
        } // while Names
        MyFreePool(DirName);
    } // while Locations
} // VOID FindTool()

// Add the second-row tags containing built-in and external tools (EFI shell,
// reboot, etc.)
static VOID ScanForTools(VOID) {
    CHAR16 *FileName = NULL, *VolName = NULL, *MokLocations, Description[256], *HiddenTools;
    REFIT_MENU_ENTRY *TempMenuEntry;
    UINTN i, j, VolumeIndex;
    UINT64 osind;
    CHAR8 *b = 0;
    UINT32 CsrValue;

    MokLocations = StrDuplicate(MOK_LOCATIONS);
    if (MokLocations != NULL)
        MergeStrings(&MokLocations, SelfDirPath, L',');

    HiddenTools = ReadHiddenTags(L"HiddenTools");
    if ((HiddenTools) && (StrLen(HiddenTools) > 0)) {
        MergeStrings(&GlobalConfig.DontScanTools, HiddenTools, L',');
    }
    MyFreePool(HiddenTools);

    for (i = 0; i < NUM_TOOLS; i++) {
        switch(GlobalConfig.ShowTools[i]) {
            // NOTE: Be sure that FileName is NULL at the end of each case.
            case TAG_SHUTDOWN:
                TempMenuEntry = CopyMenuEntry(&MenuEntryShutdown);
                TempMenuEntry->Image = BuiltinIcon(BUILTIN_ICON_FUNC_SHUTDOWN);
                AddMenuEntry(&MainMenu, TempMenuEntry);
                break;

            case TAG_REBOOT:
                TempMenuEntry = CopyMenuEntry(&MenuEntryReset);
                TempMenuEntry->Image = BuiltinIcon(BUILTIN_ICON_FUNC_RESET);
                AddMenuEntry(&MainMenu, TempMenuEntry);
                break;

            case TAG_ABOUT:
                TempMenuEntry = CopyMenuEntry(&MenuEntryAbout);
                TempMenuEntry->Image = BuiltinIcon(BUILTIN_ICON_FUNC_ABOUT);
                AddMenuEntry(&MainMenu, TempMenuEntry);
                break;

            case TAG_EXIT:
                TempMenuEntry = CopyMenuEntry(&MenuEntryExit);
                TempMenuEntry->Image = BuiltinIcon(BUILTIN_ICON_FUNC_EXIT);
                AddMenuEntry(&MainMenu, TempMenuEntry);
                break;

            case TAG_HIDDEN:
                if (GlobalConfig.HiddenTags) {
                    TempMenuEntry = CopyMenuEntry(&MenuEntryManageHidden);
                    TempMenuEntry->Image = BuiltinIcon(BUILTIN_ICON_FUNC_HIDDEN);
                    AddMenuEntry(&MainMenu, TempMenuEntry);
                }
                break;

            case TAG_FIRMWARE:
                if (EfivarGetRaw(&GlobalGuid, L"OsIndicationsSupported", &b, &j) == EFI_SUCCESS) {
                    osind = (UINT64)*b;
                    if (osind & EFI_OS_INDICATIONS_BOOT_TO_FW_UI) {
                        TempMenuEntry = CopyMenuEntry(&MenuEntryFirmware);
                        TempMenuEntry->Image = BuiltinIcon(BUILTIN_ICON_FUNC_FIRMWARE);
                        AddMenuEntry(&MainMenu, TempMenuEntry);
                    } // if
                } // if
                break;

            case TAG_SHELL:
                j = 0;
                while ((FileName = FindCommaDelimited(SHELL_NAMES, j++)) != NULL) {
                    if (IsValidTool(SelfVolume, FileName)) {
                        AddToolEntry(SelfVolume, FileName, L"EFI Shell", BuiltinIcon(BUILTIN_ICON_TOOL_SHELL),
                                     'S', FALSE);
                    }
                MyFreePool(FileName);
                } // while
                break;

            case TAG_GPTSYNC:
                j = 0;
                while ((FileName = FindCommaDelimited(GPTSYNC_NAMES, j++)) != NULL) {
                    if (IsValidTool(SelfVolume, FileName)) {
                        AddToolEntry(SelfLoadedImage->DeviceHandle, FileName, L"Hybrid MBR tool", BuiltinIcon(BUILTIN_ICON_TOOL_PART),
                                     'P', FALSE);
                    } // if
                    MyFreePool(FileName);
                } // while
                FileName = NULL;
                break;

            case TAG_GDISK:
                j = 0;
                while ((FileName = FindCommaDelimited(GDISK_NAMES, j++)) != NULL) {
                    if (IsValidTool(SelfVolume, FileName)) {
                        AddToolEntry(SelfVolume, FileName, L"disk partitioning tool",
                                     BuiltinIcon(BUILTIN_ICON_TOOL_PART), 'G', FALSE);
                    } // if
                    MyFreePool(FileName);
                } // while
                FileName = NULL;
                break;
            
            case TAG_NETBOOT:
                j = 0;
                while ((FileName = FindCommaDelimited(NETBOOT_NAMES, j++)) != NULL) {
                    if (IsValidTool(SelfVolume, FileName)) {
                        AddToolEntry(SelfVolume, FileName, L"Netboot",
                                     BuiltinIcon(BUILTIN_ICON_TOOL_NETBOOT), 'N', FALSE);
                    } // if
                    MyFreePool(FileName);
                } // while
                FileName = NULL;
                break;

            case TAG_APPLE_RECOVERY:
                for (VolumeIndex = 0; VolumeIndex < VolumesCount; VolumeIndex++) {
                    j = 0;
                    while ((FileName = FindCommaDelimited(GlobalConfig.MacOSRecoveryFiles, j++)) != NULL) {
                        if ((Volumes[VolumeIndex]->RootDir != NULL)) {
                            if ((IsValidTool(Volumes[VolumeIndex], FileName))) {
                                SPrint(Description, 255, L"Apple Recovery on %s", Volumes[VolumeIndex]->VolName);
                                AddToolEntry(Volumes[VolumeIndex], FileName, Description,
                                                BuiltinIcon(BUILTIN_ICON_TOOL_APPLE_RESCUE), 'R', TRUE);
                            } // if
                        } // if
                    } // while
                } // for
                break;

            case TAG_WINDOWS_RECOVERY:
                j = 0;
                while ((FileName = FindCommaDelimited(GlobalConfig.WindowsRecoveryFiles, j++)) != NULL) {
                    SplitVolumeAndFilename(&FileName, &VolName);
                    for (VolumeIndex = 0; VolumeIndex < VolumesCount; VolumeIndex++) {
                        if ((Volumes[VolumeIndex]->RootDir != NULL) &&
                            (IsValidTool(Volumes[VolumeIndex], FileName)) &&
                            ((VolName == NULL) || MyStriCmp(VolName, Volumes[VolumeIndex]->VolName))) {
                                SPrint(Description, 255, L"Microsoft Recovery on %s", Volumes[VolumeIndex]->VolName);
                                AddToolEntry(Volumes[VolumeIndex], FileName, Description,
                                             BuiltinIcon(BUILTIN_ICON_TOOL_WINDOWS_RESCUE), 'R', TRUE);
                        } // if
                    } // for
                } // while
                MyFreePool(FileName);
                FileName = NULL;
                MyFreePool(VolName);
                VolName = NULL;
                break;

            case TAG_MOK_TOOL:
                FindTool(MokLocations, MOK_NAMES, L"MOK utility", BUILTIN_ICON_TOOL_MOK_TOOL);
                break;

            case TAG_FWUPDATE_TOOL:
                FindTool(MokLocations, FWUPDATE_NAMES, L"firmware update utility", BUILTIN_ICON_TOOL_FWUPDATE);
                break;

            case TAG_CSR_ROTATE:
                if ((GetCsrStatus(&CsrValue) == EFI_SUCCESS) && (GlobalConfig.CsrValues)) {
                    TempMenuEntry = CopyMenuEntry(&MenuEntryRotateCsr);
                    TempMenuEntry->Image = BuiltinIcon(BUILTIN_ICON_FUNC_CSR_ROTATE);
                    AddMenuEntry(&MainMenu, TempMenuEntry);
                } // if
                break;

            case TAG_MEMTEST:
                FindTool(MEMTEST_LOCATIONS, MEMTEST_NAMES, L"Memory test utility", BUILTIN_ICON_TOOL_MEMTEST);
                break;

        } // switch()
    } // for
} // static VOID ScanForTools

// Rescan for boot loaders
VOID RescanAll(BOOLEAN DisplayMessage) {
    FreeList((VOID ***) &(MainMenu.Entries), &MainMenu.EntryCount);
    MainMenu.Entries = NULL;
    MainMenu.EntryCount = 0;
    ConnectAllDriversToAllControllers();
    ScanVolumes();
    ReadConfig(GlobalConfig.ConfigFilename);
    SetVolumeIcons();
    ScanForBootloaders(TRUE);
    ScanForTools();
} // VOID RescanAll()

#ifdef __MAKEWITH_TIANO

// Minimal initialization function
static VOID InitializeLib(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
    gST            = SystemTable;
    //    gImageHandle   = ImageHandle;
    gBS            = SystemTable->BootServices;
    //    gRS            = SystemTable->RuntimeServices;
    gRT = SystemTable->RuntimeServices; // Some BDS functions need gRT to be set
    EfiGetSystemConfigurationTable (&gEfiDxeServicesTableGuid, (VOID **) &gDS);
}

#endif

// Set up our own Secure Boot extensions....
// Returns TRUE on success, FALSE otherwise
static BOOLEAN SecureBootSetup(VOID) {
    EFI_STATUS Status;
    BOOLEAN    Success = FALSE;

    if (secure_mode() && ShimLoaded()) {
        Status = security_policy_install();
        if (Status == EFI_SUCCESS) {
            Success = TRUE;
        } else {
            Print(L"Failed to install MOK Secure Boot extensions");
            PauseForKey();
        }
    }
    return Success;
} // VOID SecureBootSetup()

// Remove our own Secure Boot extensions....
// Returns TRUE on success, FALSE otherwise
static BOOLEAN SecureBootUninstall(VOID) {
    EFI_STATUS Status;
    BOOLEAN    Success = TRUE;

    if (secure_mode()) {
        Status = security_policy_uninstall();
        if (Status != EFI_SUCCESS) {
            Success = FALSE;
            BeginTextScreen(L"Secure Boot Policy Failure");
            Print(L"Failed to uninstall MOK Secure Boot extensions; forcing a reboot.");
            PauseForKey();
            refit_call4_wrapper(RT->ResetSystem, EfiResetCold, EFI_SUCCESS, 0, NULL);
        }
    }
    return Success;
} // VOID SecureBootUninstall

// Sets the global configuration filename; will be CONFIG_FILE_NAME unless the
// "-c" command-line option is set, in which case that takes precedence.
// If an error is encountered, leaves the value alone (it should be set to
// CONFIG_FILE_NAME when GlobalConfig is initialized).
static VOID SetConfigFilename(EFI_HANDLE ImageHandle) {
    EFI_LOADED_IMAGE *Info;
    CHAR16 *Options, *FileName, *SubString;
    EFI_STATUS Status;

    Status = refit_call3_wrapper(BS->HandleProtocol, ImageHandle, &LoadedImageProtocol, (VOID **) &Info);
    if ((Status == EFI_SUCCESS) && (Info->LoadOptionsSize > 0)) {
        Options = (CHAR16 *) Info->LoadOptions;
        SubString = MyStrStr(Options, L" -c ");
        if (SubString) {
            FileName = StrDuplicate(&SubString[4]);
            if (FileName) {
                LimitStringLength(FileName, 256);
            }

            if (FileExists(SelfDir, FileName)) {
                GlobalConfig.ConfigFilename = FileName;
            } else {
                Print(L"Specified configuration file (%s) doesn't exist; using\n'refind.conf' default\n", FileName);
                MyFreePool(FileName);
            } // if/else
        } // if
    } // if
} // VOID SetConfigFilename()

// Adjust the GlobalConfig.DefaultSelection variable: Replace all "+" elements with the
// rEFInd PreviousBoot variable, if it's available. If it's not available, delete that
// element.
VOID AdjustDefaultSelection() {
    UINTN i = 0, j;
    CHAR16 *Element = NULL, *NewCommaDelimited = NULL, *PreviousBoot = NULL;
    EFI_STATUS Status;

    while ((Element = FindCommaDelimited(GlobalConfig.DefaultSelection, i++)) != NULL) {
        if (MyStriCmp(Element, L"+")) {
            Status = EfivarGetRaw(&RefindGuid, L"PreviousBoot", (CHAR8 **) &PreviousBoot, &j);
            if (Status == EFI_SUCCESS) {
                MyFreePool(Element);
                Element = PreviousBoot;
            } else {
                Element = NULL;
            }
        } // if
        if (Element && StrLen(Element)) {
            MergeStrings(&NewCommaDelimited, Element, L',');
        } // if
        MyFreePool(Element);
    } // while
    MyFreePool(GlobalConfig.DefaultSelection);
    GlobalConfig.DefaultSelection = NewCommaDelimited;
} // AdjustDefaultSelection()

//
// main entry point
//
EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS         Status;
    BOOLEAN            MainLoopRunning = TRUE;
    BOOLEAN            MokProtocol;
    REFIT_MENU_ENTRY   *ChosenEntry;
    UINTN              MenuExit, i;
    CHAR16             *SelectionName = NULL;
    EG_PIXEL           BGColor = COLOR_LIGHTBLUE;

    // bootstrap
    InitializeLib(ImageHandle, SystemTable);
    Status = InitRefitLib(ImageHandle);
    if (EFI_ERROR(Status))
        return Status;

    // read configuration
    CopyMem(GlobalConfig.ScanFor, "ieom      ", NUM_SCAN_OPTIONS);
    FindLegacyBootType();
    if (GlobalConfig.LegacyType == LEGACY_TYPE_MAC)
       CopyMem(GlobalConfig.ScanFor, "ihebocm   ", NUM_SCAN_OPTIONS);
    SetConfigFilename(ImageHandle);
    MokProtocol = SecureBootSetup();

    // Scan volumes first to find SelfVolume, which is required by LoadDrivers();
    // however, if drivers are loaded, a second call to ScanVolumes() is needed
    // to register the new filesystem(s) accessed by the drivers.
    // Also, ScanVolumes() must be done before ReadConfig(), which needs
    // SelfVolume->VolName.
    ScanVolumes();
    if (LoadDrivers())
        ScanVolumes();
    ReadConfig(GlobalConfig.ConfigFilename);
    AdjustDefaultSelection();

    if (GlobalConfig.SpoofOSXVersion && GlobalConfig.SpoofOSXVersion[0] != L'\0')
        SetAppleOSInfo();

    InitScreen();
    WarnIfLegacyProblems();
    MainMenu.TimeoutSeconds = GlobalConfig.Timeout;

    // disable EFI watchdog timer
    refit_call4_wrapper(BS->SetWatchdogTimer, 0x0000, 0x0000, 0x0000, NULL);

    // further bootstrap (now with config available)
    SetupScreen();
    SetVolumeIcons();
    ScanForBootloaders(FALSE);
    ScanForTools();
    // SetupScreen() clears the screen; but ScanForBootloaders() may display a
    // message that must be deleted, so do so
    BltClearScreen(TRUE);
    pdInitialize();

    if (GlobalConfig.ScanDelay > 0) {
       if (GlobalConfig.ScanDelay > 1)
          egDisplayMessage(L"Pausing before disk scan; please wait....", &BGColor, CENTER);
       for (i = 0; i < GlobalConfig.ScanDelay; i++)
          refit_call1_wrapper(BS->Stall, 1000000);
       RescanAll(GlobalConfig.ScanDelay > 1);
       BltClearScreen(TRUE);
    } // if

    if (GlobalConfig.DefaultSelection)
       SelectionName = StrDuplicate(GlobalConfig.DefaultSelection);
    if (GlobalConfig.ShutdownAfterTimeout)
        MainMenu.TimeoutText = L"Shutdown";

    while (MainLoopRunning) {
        MenuExit = RunMainMenu(&MainMenu, &SelectionName, &ChosenEntry);

        // The Escape key triggers a re-scan operation....
        if (MenuExit == MENU_EXIT_ESCAPE) {
            MenuExit = 0;
            RescanAll(TRUE);
            continue;
        }

        if ((MenuExit == MENU_EXIT_TIMEOUT) && GlobalConfig.ShutdownAfterTimeout) {
            ChosenEntry->Tag = TAG_SHUTDOWN;
        }

        switch (ChosenEntry->Tag) {

            case TAG_REBOOT:    // Reboot
                TerminateScreen();
                refit_call4_wrapper(RT->ResetSystem, EfiResetCold, EFI_SUCCESS, 0, NULL);
                MainLoopRunning = FALSE;   // just in case we get this far
                break;

            case TAG_SHUTDOWN: // Shut Down
                TerminateScreen();
                refit_call4_wrapper(RT->ResetSystem, EfiResetShutdown, EFI_SUCCESS, 0, NULL);
                MainLoopRunning = FALSE;   // just in case we get this far
                break;

            case TAG_ABOUT:    // About rEFInd
                AboutrEFInd();
                break;

            case TAG_LOADER:   // Boot OS via .EFI loader
                StartLoader((LOADER_ENTRY *)ChosenEntry, SelectionName);
                break;

            case TAG_LEGACY:   // Boot legacy OS
                StartLegacy((LEGACY_ENTRY *)ChosenEntry, SelectionName);
                break;

            case TAG_LEGACY_UEFI: // Boot a legacy OS on a non-Mac
                StartLegacyUEFI((LEGACY_ENTRY *)ChosenEntry, SelectionName);
                break;

            case TAG_TOOL:     // Start a EFI tool
                StartTool((LOADER_ENTRY *)ChosenEntry);
                break;

            case TAG_HIDDEN:  // Manage hidden tag entries
                ManageHiddenTags();
                break;

            case TAG_EXIT:    // Terminate rEFInd
                if ((MokProtocol) && !SecureBootUninstall()) {
                   MainLoopRunning = FALSE;   // just in case we get this far
                } else {
                   BeginTextScreen(L" ");
                   return EFI_SUCCESS;
                }
                break;

            case TAG_FIRMWARE: // Reboot into firmware's user interface
                RebootIntoFirmware();
                break;

            case TAG_CSR_ROTATE:
                RotateCsrValue();
                break;

        } // switch()
    } // while()

    // If we end up here, things have gone wrong. Try to reboot, and if that
    // fails, go into an endless loop.
    refit_call4_wrapper(RT->ResetSystem, EfiResetCold, EFI_SUCCESS, 0, NULL);
    EndlessIdleLoop();

    return EFI_SUCCESS;
} /* efi_main() */
