# TimeDefuser

TimeDefuser is a Windows kernel security research project on enforcement of expiration dates (a.k.a. a "*timebomb*") on prerelease Windows builds, how to patch them for gaining arbitrary code execution, 
and a proof-of-concept (shared for education and research purposes) that removes expiration date enforcement from the kernel.
The PoC driver in this repository patches the timebomb code itself in the kernel, which differs from widespread "activation-based" patches (policy files, registry edits, etc.).
Thus, it is the most effective and versatile way to neutralize it, unlike activation-based patching methods which are not available in many builds.

All builds are *theoretically* supported, but not all builds are tested. See the notes below and screenshots at the end of this document.

Full whitepaper and technical analysis is located [here](/TimeDefuser-Research.md). The rest of this document is about the PoC driver that removes expiration date enforcement from the system.

# TimeDefuser PoC Driver

> [!WARNING]
> This driver is intended to remove the **Windows builds'** expiration date only

It will not remove the expiration date of
- Your abusive relationship
- 100-minute Minecraft demo
- The Pepsi can from 1956 that is inside your fridge for whatever reason
- Aceyware "Tracey" Operating System version 0.1.3 (or whatever it ends up being called)
- ???
- Evaluation retail Windows builds. While it *may* work, this configuration is unsupported and any related bug reports will be closed.

> [!IMPORTANT]
> This driver will **not** patch Windows Product Activation or any other similar mechanism. These other mechanisms can be preferred as well in supported builds but here is not their place.

# Notes
- A good amount of x64 builds can detect this via PatchGuard (basically a mechanism in Windows kernel that detects unauthorized modifications to kernel code, does not exist in x86).
Getting over it will weaponize this already versatile patch, so disabling PatchGuard will never be implemented. But as an user, you still have workarounds:
	- Force enable kernel debugger at boot, which will disable PatchGuard
	- Patch the kernel image itself with offline patcher, instead of runtime patching with driver.
- This patch can technically be ported to ARM, ARM64 and Itanium hosts but due to lack of an environment to run and debug Windows on these platforms, this is not possible at the moment.

## Notes for Offline Patcher
- Do not use kernel driver with offline patched systems, as the driver is nonfunctional with patched kernels (as there is nothing to remove).
- Windows XP and earlier builds are not supported, usage of kernel driver is required for those builds.

## Notes Per Version

### Windows 2000/XP 
- As written above, these builds does not support offline patching.
- **I KNOW there are "easier" methods, so don't come to say me "muh set GracePeriod to 0" or "muh use TweakNT"**. This tweak for NT 5.x exists more as proof of concept, and both this patch or other tweaks will do the work. 
### Post-reset Windows Vista & Early 7
- They suck. Avoid using these versions at all. After build expires, buggy WPA breaks the timebomb which makes this patch not get applied anyway, and shows the "Activate Windows" dialog which logs you off if you say no; considering that those builds can successfully finish the windeploy and boot to OOBE/desktop at all in the first place (https://github.com/NevermindExpress/TimeDefuser/issues/3). See https://github.com/NevermindExpress/TimeDefuser/issues/2 and https://github.com/NevermindExpress/TimeDefuser/issues/2#issuecomment-2970226626 for more info.
- These builds are *wontfix* because there is nothing to fix/can be fixed in the first place. Blame Microsoft.
### Later Windows 7 (at least 67xx and later)
- Since TimeDefuser 1.7.1 they are now working working without hitting into page fault (see #3), though they are still subject to PatchGuard detections. 
### Windows 8
- Some builds such as 7880 has a partially broken timebomb that effectively gets disabled if you install at current date instead of setting it to pre-expiration before install. See https://github.com/NevermindExpress/TimeDefuser/issues/5
- **Again, I KNOW 'THEY' CAN BE PATCHED WITH POLICY/SPP FILES REPLACEMENT**. "MUH FBL builds can be patched by doing X/can be used at current date without doing anything" well, my thing can patch **ALL** versions (except ones that have superior PatchGuard) while your method can only fix a few builds.
### Windows 10/11
> [!IMPORTANT]
> Windows 10 builds are also subject to flight signing, which are code signatures that gets invalid after expiration date, thus preventing system from booting or to be used properly. 
> Getting over this requires additional work (resigning all binaries and disabling integrity checks, or patching bootloader & ci.dll) which is not covered by this project.
- Works on pre-RTM, post-RTM ("insider") builds are untested but they likely are same as pre-RTM unless KASLR is enabled, which is not supported by this driver.

# Usage
Since TimeDefuser 1.8.3, INF file is deprecated and the driver is instead installed as a service with `sc.exe`. A script for installing named `Installer.bat` will be bundled with subsequent releases.
- If your system didn't crash after installition, check expiration date from "winver". Absence of the expiration date means that driver has worked.
- **(x64 systems only)** Wait for several minutes, the system might crash after a few minutes of installition with a `0x109 CRITICAL_STRUCTURE_CORRUPTION` bugcheck. See notes about more info.
- If you need to remove driver, simply execute `sc delete TimeDefuser` and reboot.

# Testing and Bug Reporting
The driver can either work correctly, crash the system, fail or work but not enough to fully patch the currently working system.
In all cases the usage of kernel debugger is required to tell which one of those cases happen, and also for why exactly the system crashes.

Driver logs will look like this when it works:
```
[*] TimeDefuser: version 1.8.1 loaded | Compiled on Jan 18 2026 18:23:45 | https://github.com/NevermindExpress/TimeDefuser
[+] TimeDefuser: SystemExpirationDate is 0x1c9faa80a3d2980
[+] TimeDefuser: Kernel Base address is 0xFFFFF80002478000 and size is 5992448
[*] TimeDefuser: No or mismatching cached addresses are found on registry.
[+] TimeDefuser: PAGEDATA Section found at 0xFFFFF80002990000 with size 56688
[+] TimeDefuser: searching for stamp at 0xFFFFF80002990000 in 56688 bytes
[+] TimeDefuser: Timebomb stamp found at 0xFFFFF80002990250
[+] TimeDefuser: ExpNtExpirationDate address is 0xFFFFF80002990250 (first occurrance)
[+] TimeDefuser: PAGELK Section found at 0xFFFFF80002725000 with size 100329
[+] TimeDefuser: searching at 0xFFFFF80002725000 in 100329 bytes
[+] TimeDefuser: Potential TimeRef found at 0xFFFFF8000272866A
[+] TimeDefuser: CALL instruction found at 0xFFFFF80002728664
[*] TimeDefuser: Invalid address, skipping this one...
[+] TimeDefuser: CALL instruction found at 0xFFFFF80002728648
[+] TimeDefuser: ExGetExpirationDate found at 0xFFFFF80002783274
[+] TimeDefuser: Patch completed successfully.

```

Builds with debug symbols are recommended to try, due to symbols making debugging much easier.

# Build
Starting with version 1.8.1, TimeDefuser does not depend on any WDK anymore. Instead, it implements 
it's own frestanding build environment that implements just as much as what TimeDefuser needs.

1. Open the solution file corresponding to your VS version (or open the oldest one available and retarget it)
2. Go to Build -> Batch build, select all
3. That's it.

# Screenshots
These screenshots are all taken by me.
![Windows 6776-2026-01-18-22-59-02](https://github.com/user-attachments/assets/86956d0b-6d3b-446f-882a-1f8d4eaeb226)
![Windows 7973 x64-2025-05-04-16-08-40](https://github.com/user-attachments/assets/f3d3a116-5b67-4b8f-bd4c-d907485a435b)
![Windows 8331 x64-2026-01-18-22-58-14](https://github.com/user-attachments/assets/7d746160-5626-4af5-916f-f57215eeccc0)
![Windows 10072 x64-2025-11-10-12-53-19](https://github.com/user-attachments/assets/02bb0087-762a-4a2b-98c9-16b3bf850a0d)
![Windows 2526-2025-05-08-17-39-56](https://github.com/user-attachments/assets/24e4f5c9-5cdc-4eae-b91f-dc13bb93a22c)

# Thanks to
- **Microsoft** for Windows, Windbg and all else.
- **archive.org and BetaArchive** for preserving beta builds and debug symbols.
- **Dimitrios Vlachos** for showing interest while I was developing this.
- **All the precious testers** that opened up issues.
