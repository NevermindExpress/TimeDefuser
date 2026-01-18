# TimeDefuser
TimeDefuser is a kernel-mode Windows driver that patches the kernel to neutralize the expiration date (a.k.a. timebomb),
which is seen on most prerelease builds that has been ever compiled.

This patch patches the timebomb code itself in the kernel so it is the most effective and versatile way to neutralize it, instead of activation patching (i.e. policy files or registry editing) which is not available in many builds.

All builds are theoretically supported but not all builds are tested, see the notes for more info, or the end of this readme for screenshots.

> [!WARNING]
> This driver is intended to remove the **Windows builds'** expiration date only

It will not remove the expiration date of
- Your abusive relationship
- 100-minute Minecraft demo
- The Pepsi can from 1956 that is inside your fridge for whatever reason
- Aceyware "Tracey" Operating System version 0.1.3 (or whatever its name ends up being)
- ???
- Evaluation retail Windows builds. While it theoretically should work, such configuration is not supported and any bug reports regarding to them will be closed without any further action.

> [!IMPORTANT]
> This driver will **not** patch Windows Product Activation or any other similar mechanism. These other mechanisms can be preferred as well in supported builds but here is not their place.

# Notes Per Version
### Windows 2000/XP 
- Use legacy version with those.
- Also note that alternative methods such as registry edits are available for those.
- **I KNOW that they do, so don't come to say me "muh set GracePeriod to 0" or "muh use TweakNT"**. This tweak for NT 5.x exists more as proof of concept, and both this patch or other tweaks will do the work. 
### Post-reset Windows Vista & Early 7
- They suck. Avoid using these versions at all. After build expires, buggy WPA breaks the timebomb which makes this patch not get applied anyway, and shows the "Activate Windows" dialog which logs you off if you say no; considering that those builds can skip the windeploy and boot to OOBE/desktop at all in the first place (https://github.com/NevermindExpress/TimeDefuser/issues/3). See https://github.com/NevermindExpress/TimeDefuser/issues/2 and https://github.com/NevermindExpress/TimeDefuser/issues/2#issuecomment-2970226626 for more info.
- These builds are *wontfix* because there is nothing to fix/can be fixed in the first place. Blame Microsoft.
- Alternative patch methods should be used for those. See https://github.com/NevermindExpress/TimeDefuser/issues/2#issuecomment-2904890597
### Later Windows 7 (at least 67xx and later)
- Since TimeDefuser 1.7.1 they are now working working without hitting into page fault (see #3), though they are still subject to PatchGuard detections, an active investigation is going for them at #8. 
### Windows 8
- Some builds such as 7880 has a partially broken timebomb that effectively gets disabled if you install at current date instead of rolling it back to pre-expiration before install. See https://github.com/NevermindExpress/TimeDefuser/issues/5
- Certain builds such as aforementioned are also subject to crashes by PatchGuard, while others such as the ones with the screenshots below are not. See https://github.com/NevermindExpress/TimeDefuser/issues/5#issuecomment-3369399950
- Few builds can be patched with policy/spp files replacement. **Again, I KNOW 'THEY' CAN BE PATCHED**. "MUH FBL builds can be patched by doing X/can be used at current date without doing anything" well, my thing can patch **ALL** versions (except ones that have superior PatchGuard) while your method can only fix a few builds.
### Windows 10/11
> [!IMPORTANT]
> Windows 10 builds are also subject to flight signing, which are code signatures that gets invalid after expiration date, thus preventing system from booting or to be used properly. 
> Getting over this requires additional work (resigning all binaries and disabling integrity checks, or patching bootloader & ci.dll) which is not covered by this project.
- Works on pre-RTM, post-RTM ("insider") builds are untested but they likely are same as pre-RTM unless KASLR is enabled, which is not supported by this driver.

# Usage
1. Enable test-signing (disabling driver signature enforcement might also be necessary.)
2. Download the latest release and obtain "devcon" utility (available in WDK and also in some .cab files).
3. Execute `devcon install C:\Path\to\TimeDefuser.inf Root\TimeDefuser`
4. Allow the installition and wait for "Driver Installition Complete" message
5. If your system didn't crash so far, check expiration date from "winver", if it's not there that means that it worked.
6. If you want to/need to uninstall, execute `devcon remove Root\TimeDefuser` and reboot (or just delete the .sys file).

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
![Windows 7973 x64-2025-05-04-16-08-40](https://github.com/user-attachments/assets/f3d3a116-5b67-4b8f-bd4c-d907485a435b)
![Windows 8331](https://github.com/user-attachments/assets/b49c313c-2768-4f24-bcfb-1cea1a072d88)
![Windows 10072 x64-2025-11-10-12-53-19](https://github.com/user-attachments/assets/02bb0087-762a-4a2b-98c9-16b3bf850a0d)
![Windows 2526-2025-05-08-17-39-56](https://github.com/user-attachments/assets/24e4f5c9-5cdc-4eae-b91f-dc13bb93a22c)

# Thanks to
- **Microsoft** for Windows, Windbg and all else.
- **archive.org and BetaArchive** for preserving beta builds and debug symbols.
- **Dimitrios Vlachos** for showing interest while I was developing this.
- **All the precious testers** that opened up issues.
