# TimeDefuser
TimeDefuser is a kernel-mode Windows driver that patches the kernel to neutralize the timebomb,
which is seen on most prerelease builds that has been ever compiled.

This patch patches the timebomb code itself in the kernel so it is the most effective and versatile way to neutralize it, instead of activation patching which is not available in many builds.

All builds are theoretically supported but not all builds are tested, see the notes for more info, or the end of this readme for screenshots.

Due to ~~lack of attention~~ nothing really left to do, this project is in maintenance mode and I will only fix bugs unless I find some fancy improvement ideas and energy & will to implement them.

> [!WARNING]
> This driver is intended to remove the **Windows builds'** expiration date only

It will not remove the expiration date of
- Your abusive relationship
- 100-minute Minecraft demo
- The Pepsi can from 1956 that is inside your fridge for whatever reason
- Aceyware "Tracey" Operating System version 0.1.3
- ???
- Evalution retail Windows builds. While it theoretically should work, such configuration is not supported and any bug reports regarding to them will be closed without any further action.

> [!IMPORTANT]
> This driver will **not** patch Windows Product Activation or any other similar mechanism. These other mechanisms can be preferred as well in supported builds but I don't support them.

# Notes Per Version
### Windows 2000/XP 
- Use legacy version with those.
- Also note that alternative methods such as registry edits are available for those.
- **I KNOW that they do, so don't come to say me "muh set GracePeriod to 0" or "muh use TweakNT"**. This tweak for NT 5.x exists more as proof of concept, and both this patch or other tweaks will do the work. 
### Post-reset Windows Vista & Early 7 (later 7 is untested)
- They suck. Avoid using these versions at all. After build expires, buggy WPA breaks the timebomb which makes this patch not get applied anyway, and shows the "Activate Windows" dialog which logs you off if you say no; considering that those builds can skip the windeploy and boot to OOBE/desktop at all in the first place (https://github.com/NevermindExpress/TimeDefuser/issues/3). See https://github.com/NevermindExpress/TimeDefuser/issues/2 and https://github.com/NevermindExpress/TimeDefuser/issues/2#issuecomment-2970226626 for more info.
- These builds are *wontfix* because there is nothing to fix/can be fixed in the first place. Blame Microsoft.
- Alternative patch methods should be used for those. See https://github.com/NevermindExpress/TimeDefuser/issues/2#issuecomment-2904890597
### Windows 8
- Some builds such as 7880 has a partially broken timebomb that effectively gets disabled if you install at current date instead of rolling it back to pre-expiration before install. See https://github.com/NevermindExpress/TimeDefuser/issues/5
- Certain builds such as aforementioned are also subject to crashes by PatchGuard, while others such as the ones with the screenshots below are not. See https://github.com/NevermindExpress/TimeDefuser/issues/5#issuecomment-3369399950
- Few builds can be patched with policy/spp files replacement. **Again, I KNOW 'THEY' CAN BE PATCHED**. "MUH FBL builds can be patched by doing X/can be used at current date without doing anything" well, my thing can patch **ALL** versions (except ones that have superior PatchGuard) while your method can only fix a few builds.
### Windows 10/11
- Untested. Likely same as 8 unless KASLR is enabled, which is not supported by this driver.

# Usage
1. Enable test-signing (and also disable driver signature enforcement at boot if you end up with boot recovery or signature error at boot)
2. Download the latest release and obtain "devcon" utility (available in WDK).
3. Execute "devcon install C:\Path\to\TimeDefuser.inf Root\TimeDefuser"
4. Allow the installition and wait for "Driver Installition Complete" message
5. If your system didn't crash so far, check expiration date from "winver", if it's not there that means that it worked.

# Testing and Bug Reporting
The driver can either work correctly, crash the system, fail or work but not enough to fully patch the currently working system.
In all cases the usage of kernel debugger is required to tell which one of those cases happen, and also for why exactly the system crashes.

Driver logs will look like this when it works:
![image](https://github.com/user-attachments/assets/c141ba8e-38ac-4d14-8c85-71b0edd127bd)

Builds with debug symbols are recommended to try, due to symbols making debugging much easier.

# Build
## Windows 7 and Later
1. Get the latest WDK 
2. Open the solution 
3. Hit the compile button.
4. Get the TimeDefuser.inf and change the \$ARCH\$ to target architecture
## Windows Vista and Earlier
*Should also apply to later versions as long as a compatible WDK is used for target version.*
1. Get a WDK/DDK compatible with your target version.
2. Open the build environment console
3. Locate to source folder and execute "nmake"
4. Get the TimeDefuser.inf (for Vista) or TimeDefuserLegacy.inf (for XP and earlier) and change the \$ARCH\$ to target architecture

# Screenshots
![Windows 8175 x64-2025-05-04-16-05-34](https://github.com/user-attachments/assets/380167b9-e24a-458a-b5ba-597313c6bbd3)
![Windows 7973 x64-2025-05-04-16-08-40](https://github.com/user-attachments/assets/f3d3a116-5b67-4b8f-bd4c-d907485a435b)
![Windows 2526-2025-05-08-17-39-56](https://github.com/user-attachments/assets/24e4f5c9-5cdc-4eae-b91f-dc13bb93a22c)

# Thanks to
- **Microsoft** for Windows, Windbg and all else.
- **archive.org and BetaArchive** for preserving beta builds and debug symbols.
- **Dimitrios Vlachos** for motivational support.
- All the precious testers that opened up issues.
