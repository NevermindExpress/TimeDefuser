# TimeDefuser
TimeDefuser is a kernel-mode Windows driver that patches the kernel to neutralize the timebomb,
which is seen on most prerelease builds that has been ever compiled.

This patch patches the timebomb code itself in the kernel so it is the most effective way to neutralize it.

All builds are theoretically supported but not all builds are tested, see the end of this readme for screenshots.

**Due to lack of attention this project will not be maintained anymore unless anyone ever makes a bug report about it not working.**

> [!WARNING]
> This driver is intended to remove the **Windows expiration date only**

It will not remove the expiration date of
- Your abusive relationship
- 100-minute Minecraft demo
- The Pepsi can from 1956 that is inside your fridge for whatever reason
- Aceyware "Tracey" Operating System version 0.1.3
- ???

> [!IMPORTANT]
> This driver will **not** patch Windows Product Activation or any other similar mechanism.

# Usage
1. Enable test-signing
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
