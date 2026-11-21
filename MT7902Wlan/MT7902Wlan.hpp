#ifndef MT7902WLAN_HPP
#define MT7902WLAN_HPP

#include <IOKit/IOService.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOMemoryDescriptor.h>

class MT7902Wlan final : public IOService
{
    OSDeclareDefaultStructors(MT7902Wlan)

private:
    IOPCIDevice *fPCIDevice;
    IOMemoryMap *fBAR0;
    bool fProviderOpened;
    bool fMemoryWasEnabled;

public:
    bool init(OSDictionary *dictionary = nullptr) override;
    bool start(IOService *provider) override;
    void stop(IOService *provider) override;
};

#endif
