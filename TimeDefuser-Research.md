# TimeDefuser: Security Research into Windows Timebombs for Expiration Date Enforcement

**Author:** Oğuzhan Karacan (@NevermindExpress)  
**Type:** Independent Security Research 
**Date:** May 4, 2025 (Initial release), January 20, 2026 (Whitepaper)  
**Repository:** https://github.com/NevermindExpress/TimeDefuser 
**Revision:** 1

*This document analyzes Windows kernel expiration enforcement mechanisms and presents a proof-of-concept implementation for educational and research purposes only.*

## 1. Prologue
Every prerelease and evaluation build of Microsoft Windows since Windows 2000 included a *timebomb* mechanism designed 
to prevent users from being able to keep using the prerelease builds after expiration date. This mechanism is often tightly integrated 
with Windows Product Activation (WPA) which will cause system to become *deactivated*, leading to effects such as blocking user logon or 
restricting system features (e.g., personalization); but it standalone bringins the system down with a `0x98 END_OF_NT_EVALUATION_PERIOD` 
bugcheck (a.k.a. a *blue screen*) approximately two hours after system boot.

This research examines how this mechanism operates internally and demonstrates how it can be exploited for arbitrary kernel-level behavior. 
A proof-of-concept (PoC) implementation is provided that disables expiration enforcement entirely.

For legal reasons, this document avoids vague descriptions and direct references to leaked Windows NT source codes.

## 2. Expiration Date Storage and Enforcement
The location of the expiration date has evolved over time: initially residing in the registry, then getting moved into WPA licensing data and product policies. 
In all cases, it is ultimately loaded into specific locations in kernel memory which are used by the timebomb enforcement logic and therefore constitute the primary
targets of this research.

The expiration date is located inside three fields in kernel memory, first two are undocumented kernel globals named `ExpNtExpirationDate` and 
`ExpNtExpiratonData`, and third field is the `SystemExpiratonDate` field of `KUSER_SHARED_DATA` structure, which is used by user mode
applications (including `winver.exe` itself). Additionally, a boolean flag named `ExpNextExpirationIsFatal` is set during the first enforcement cycle.

These fields are initialized during system boot by the `ExInitializeTimeRefresh` routine, which retrieves expiration date using version-specific mechanisms. 
On Windows XP and earlier, this function also spawns a background task called `ExpWatchExpirationDataWork` that prevents modification of the registry values of 
expiration date. Beginning with Windows Vista, this protection is implemented through different mechanisms, discussed later in this document.

The core timebomb enforcement code resides inside the internal kernel routine `ExpTimeRefreshWork` that runs each hour. In addition to synchronizing system 
and hardware clocks, this function checks if expiration date has passed, and initiates enforcement actions with specific behavior varying by Windows version. 

On Windows XP and earlier, it simply compares current system time against the expiration date stored in kernel memory. If `ExpNextExpirationIsFatal` is false, 
it displays an error message to user by `NtRaiseHardError` indicating that the build has expired and sets `ExpNextExpirationIsFatal`. On subsequent execution, it 
halts the system with aforementioned bugcheck.

Beginning with Windows Vista, the mechanism becomes more sophisticated. Instead of relying solely on data stored in kernel memory, it introduces the `ExGetExpirationDate` 
function to retrieve the expiration date dynamically. This function is invoked both during boot by `ExInitializeTimeRefresh` and periodically by `ExpTimeRefreshWork` during each
enforcement cycle. Its return value is then used to repopulate the expiration date fields in kernel memory, effectively reasserting expiration state on every execution of the 
timebomb logic and invalidating the method of zero-ing those fields for bypassing timebomb logic on Windows XP and before.

## 3. Finding and Patching The Timebomb Logic in Kernel Memory.
In the previous section, we established that on Windows Vista and later, expiration date fields in kernel memory are repopulated during each enforcement cycle. The first two fields 
(`ExpNtExpirationDate` and `ExpNtExpirationData`) are internal kernel global variables whose addresses vary between kernel builds. This does not apply to the third field, 
`SystemExpirationDate`, which corresponds to offset `0x2C8` within the `KUSER_SHARED_DATA` structure.

`KUSER_SHARED_DATA` is a well defined but undocumented structure in Windows NT which is used for fast access to various system-wide values from user mode with an
increasing size and contents with each Windows NT version. Due to its purpose, this structure is always mapped at a fixed virtual address depending on architecture 
(i.e., `0xffdf0000` on x86) regardless of the Windows NT build.

Because timebomb enforcement is implemented in two routines (`ExInitializeTimeRefresh` and `ExpTimeRefreshWork`), there are as much amount of immediate absolute address references 
to `+0x2c8` offset of `KUSER_SHARED_DATA` that does not exist in anywhere else in kernel memory. By searching for this absolute address reference within kernel memory, the locations
where `SystemExpirationDate` is repopulated can be identified (with the first occurrence typically belonging to `ExpTimeRefreshWork`). By iterating backward from this reference,
the relative call target of `ExGetExpirationDate` can be resolved.

At this point, several patching strategies are possible
1. Resolving the relative address and patching `ExGetExpirationDate` function
2. Patching the caller instruction to call an arbitrary function
3. Modifying other portions of the timebomb enforcement routines themselves.

On Windows XP and earlier, the approach slightly changes due to lack of an `ExGetExpirationDate` function and repopulation of fields including the `SystemExpirationDate` in each cycle.
For those systems, retrieving `SystemExpirationDate` and searching for that same value within kernel memory will yield other two fields of expiration date. By then searching for references
to their addresses, `ExpTimeRefreshWork` can be found.

## 4. Limitations

### PatchGuard (Kernel Patch Protection)
64-bit versions of Windows kernel includes a Kernel Patch Protection mechanism called "PatchGuard" which may detect modifications to timebomb enforcement routines depending on the version, limiting
the applicability of this technique. 32-bit versions does not include this mechanism.

### Stack Safety Checks
Windows NT kernel checks for any improperities happening in stack. This includes stack corruptions, shifts caused by imbalances, and direct attacks performed against stack. If the kernel detects any 
such improperities, it will bring the system down. Calling convention semantics and stack discipline must be retained to not trigger these protections.

### Security Cookies and Control Flow Guard
Modern Windows kernels employ stack cookies, Control Flow Guard (CFG), and related mitigations to detect stack smashing and invalid control transfers. This especially causes extra difficulty when patching
`ExGetExpirationDate` as modification of function prologues will trigger those protections and cause system to halt. This is more prevalent in 32-bit x86 systems.

## 5. Advantages Over Simply Having A Kernel Driver
While any desired operation could also be performed using a kernel driver, this approach offers several advantages:

1. **Non-persistence:** Instead of keeping a continuously running kernel driver, the kernel can be patched and execution can terminate immediately, leaving no persistent artifacts behind.
2. **Consistency and Versatility:** Time synchronization and expiration enforcement routines are relatively stable and easier to target compared to many other kernel subsystems.
3. **Independence from Driver Loading Mechanisms:** This approach can be applied using a future arbitrary kernel memory read/write vulnerability, without requiring driver loading, signature bypasses, or test-signing configurations.

## 6. Proof-of-Concept Implementation
This research is also bundled with a practical Proof-of-Concept (PoC) implementation of the techniques described. It can be retrieved from the same repository this paper published at 
(https://github.com/NevermindExpress/TimeDefuser). The PoC serves as a practical demonstration that simply removes expiration date enforcement entirely.

## 7. Threat Model and Abuse Potential
While this research focuses on expiration enforcement mechanisms, the techniques described are broadly applicable to kernel patching and runtime modification of enforcement logic. 
In a threat context, these routines can be modified to do any arbitrary and potentially malicious activities with a frequency of once per hour.
## 8. Responsible Use and Ethical Considerations
This research is conducted and presented for educational, defensive, and academic purposes. The techniques described are intended to improve understanding of kernel enforcement mechanisms, 
tamper resistance, and system integrity, and to contribute to the broader field of operating system security research.

The proof-of-concept implementation is provided solely as a demonstration of feasibility and is not intended for unauthorized use, circumvention of licensing agreements, violation of 
software terms of service, or as a weapon for system compromising. Users of this research are responsible for ensuring compliance with all applicable laws, licensing agreements, and ethical standards.

The author does not condone or support the misuse of these techniques for piracy, evasion of security controls, or unauthorized system modification. Any experimentation should be conducted 
in controlled environments, such as virtual machines or test systems, and with appropriate authorization.

## 9. Conclusion
This research examined the internal mechanisms used by Windows to enforce expiration dates on prerelease and evaluation builds, focusing on their evolution, implementation, and enforcement at the kernel level. 
By analyzing the storage, propagation, and periodic refresh of expiration state, this work demonstrated how these mechanisms can be reliably located and modified in memory.

The accompanying proof-of-concept implementation illustrates the feasibility of these techniques across multiple Windows versions and architectures, while also highlighting real-world constraints 
imposed by modern kernel mitigations such as PatchGuard, control flow integrity, and stack protection mechanisms.

Beyond expiration enforcement, the techniques described in this paper underscore broader implications for kernel security, particularly in the context of integrity enforcement, runtime patching, 
and tamper resistance. This work contributes to a deeper understanding of how kernel-level enforcement mechanisms operate and how they may be strengthened against unauthorized modification.

