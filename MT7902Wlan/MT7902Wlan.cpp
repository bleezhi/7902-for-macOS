#include "MT7902Wlan.hpp"

#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>

#define MT7902_VENDOR_ID 0x14C3
#define MT7902_DEVICE_ID 0x7902

OSDefineMetaClassAndStructors(MT7902Wlan, IOService)

bool MT7902Wlan::init(OSDictionary *dictionary)
{
    if (!IOService::init(dictionary))
        return false;

    fPCIDevice = nullptr;
    fBAR0 = nullptr;
    fProviderOpened = false;
    fMemoryWasEnabled = false;
    return true;
}

bool MT7902Wlan::start(IOService *provider)
{
    if (!IOService::start(provider))
        return false;

    IOPCIDevice *pci = OSDynamicCast(IOPCIDevice, provider);
    if (pci == nullptr) {
        IOLog("MT7902Wlan: provider is not an IOPCIDevice\n");
        return false;
    }

    const UInt16 vendor = pci->configRead16(kIOPCIConfigVendorID);
    const UInt16 device = pci->configRead16(kIOPCIConfigDeviceID);

    IOLog("MT7902Wlan: PCI function %04x:%04x detected\n", vendor, device);

    if (vendor != MT7902_VENDOR_ID || device != MT7902_DEVICE_ID) {
        IOLog("MT7902Wlan: refusing unexpected PCI function\n");
        return false;
    }

    /*
     * Keep this first milestone deliberately read-only:
     * no bus mastering, DMA, interrupts, reset, firmware or ownership
     * transitions are enabled here.
     */
    if (!pci->open(this)) {
        IOLog("MT7902Wlan: could not open PCI provider\\n");
        return false;
    }

    fProviderOpened = true;
    fMemoryWasEnabled = pci->setMemoryEnable(true);
    fPCIDevice = pci;
    fPCIDevice->retain();

    fBAR0 = fPCIDevice->mapDeviceMemoryWithIndex(0);
    if (fBAR0 == nullptr) {
        IOLog("MT7902Wlan: unable to map BAR0; PCI attach aborted\n");
        if (!fMemoryWasEnabled)
            fPCIDevice->setMemoryEnable(false);
        fPCIDevice->close(this);
        fProviderOpened = false;
        fPCIDevice->release();
        fPCIDevice = nullptr;
        return false;
    }

    IOLog("MT7902Wlan: BAR0 mapped, length=0x%llx bytes\n",
          (unsigned long long)fBAR0->getLength());

    /*
     * MT7902 chip ID (0x70010200) is accessed through the device's L1
     * remap window, not by treating that address as a BAR0 offset.
     * The remap protocol is intentionally not touched until its register
     * definitions and power/ownership sequence are implemented.
     */
    setProperty("MT7902,PCI-Vendor-ID", (unsigned long long)vendor, 16);
    setProperty("MT7902,PCI-Device-ID", (unsigned long long)device, 16);
    setProperty("MT7902,BAR0-Length", (unsigned long long)fBAR0->getLength(), 64);

    registerService();
    IOLog("MT7902Wlan: read-only PCI probe attached successfully\n");
    return true;
}

void MT7902Wlan::stop(IOService *provider)
{
    IOLog("MT7902Wlan: stopping PCI probe\n");

    if (fBAR0 != nullptr) {
        fBAR0->release();
        fBAR0 = nullptr;
    }

    if (fPCIDevice != nullptr) {
        if (!fMemoryWasEnabled)
            fPCIDevice->setMemoryEnable(false);
        if (fProviderOpened) {
            fPCIDevice->close(this);
            fProviderOpened = false;
        }
        fPCIDevice->release();
        fPCIDevice = nullptr;
    }

    IOService::stop(provider);
}
