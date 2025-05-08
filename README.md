# TimeDefuser
TimeDefuser is a kernel-mode Windows driver that patches the kernel to neutralize the timebomb,
which is seen on most prerelease builds that has been ever compiled.

This patch patches the code itself that is responsible for the timebomb so it is the most effective
way to neutralize it.

While it should theoretically work on every build ever, ~~this patch is initially developed atop Windows 8 x64
and it is currently unknown that it would work as-is on older versions or not.~~ **UPDATE:** Windows XP and earlier,
and x86-32 support is now implemented.

> [!WARNING]
> This driver is intended to remove the **Windows expiration date only**

It will not remove the expiration date of
- Your abusive relationship
- 100-minute Minecraft demo
- The Pepsi can from 1956 that is inside your fridge for whatever reason
- Aceyware "Tracey" Operating System version 0.1.3

> [!IMPORTANT]
> This driver will **not** patch Windows Product Activation or any other similar mechanism.

# Usage
1. Enable test-signing
2. Download the latest release and obtain "devcon" utility.
3. Execute "devcon install C:\Path\to\TimeDefuser.inf Root\TimeDefuser"
4. Allow the installition and wait for "Driver Installition Complete" message
5. If your system didn't crash so far, check expiration date from "winver", if it's not there that means that it worked.

# Build
Get the latest WDK, open the solution and hit the compile button.