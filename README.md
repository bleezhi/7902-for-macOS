# 7902-for-macOS

An experimental macOS kernel extension for the MediaTek MT7902 (PCI ID
`14C3:7902`) wireless controller.

**Status: early development — PCI attach and BAR0 mapping code added; hardware validation and Wi-Fi integration are not done.**

## Current milestone

The first driver milestone is intentionally conservative:

- Match the MediaTek vendor/device ID `14C3:7902`.
- Attach to the PCI function through IOKit.
- Read the PCI vendor and device IDs.
- Open the PCI provider, enable PCI memory decoding (restoring its previous state on detach), map BAR0 and report its size.
- Expose the detected IDs and BAR size as IOKit properties.

It does **not** enable bus mastering, DMA, interrupts, device resets, firmware
loading, or MT7902 power/ownership transitions. It does enable PCI memory
space decoding, which is required to map BAR0, and restores the prior setting
when it detaches. It does not register an AirPort or
802.11 network interface.

The Linux `mt76` driver is being used as a hardware-behaviour reference. Its
MT7902 L1 register remapping means chip registers such as `MT_HW_CHIPID`
(`0x70010200`) must not be accessed as naïve BAR0 offsets.

## Repository layout

- `MT7902Wlan/` — IOKit driver source and KEXT personality.
- `Scripts/build.sh` — x86_64 KEXT build script.
- `.github/workflows/build.yml` — manual build and optional release workflow.

## Build requirements

- Intel/x86_64 macOS build host.
- Xcode command-line tools.
- Apple Kernel Debug Kit (KDK) matching the target macOS kernel build.
- Set `KDK_KERNEL_HEADERS` to the KDK's
  `System/Library/Frameworks/Kernel.framework/Headers` directory if automatic
  discovery does not find it.

Build:

```bash
VERSION=0.1.0-alpha.1 Scripts/build.sh
```

Output:

```
build/MT7902Wlan.kext/
```

The KEXT is not yet suitable for everyday use (**yet!**). Kernel extensions run in kernel
space; only test on a system where you can recover through your OpenCore EFI.

## Planned work

1. Validate PCI attach and BAR mapping on the actual MT7902.
2. Implement the documented direct/L1 register access safely.
3. Read and validate chip ID/revision after the required device ownership
   sequence is understood.
4. Implement reset, interrupts, DMA and firmware transport.
5. Investigate macOS Wi-Fi framework integration separately.
