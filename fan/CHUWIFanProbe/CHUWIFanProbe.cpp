#include <IOKit/IOLib.h>
#include <IOKit/IOService.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <libkern/c++/OSContainers.h>

class CHUWIFanProbe : public IOService {
    OSDeclareDefaultStructors(CHUWIFanProbe)
public:
    bool start(IOService *provider) override;
    void stop(IOService *provider) override;
};

OSDefineMetaClassAndStructors(CHUWIFanProbe, IOService)

bool CHUWIFanProbe::start(IOService *provider) {
    if (!super::start(provider)) return false;

    IOACPIPlatformDevice *ptid = OSDynamicCast(IOACPIPlatformDevice, provider);
    if (!ptid) {
        IOLog("[CHUWIFanProbe] ERROR: provider is not IOACPIPlatformDevice\n");
        return true;
    }

    IOLog("[CHUWIFanProbe] matched ACPI device %s; read-only OSDD probe starting\n",
          ptid->getName());

    OSObject *result = nullptr;
    IOReturn status = ptid->evaluateObject("OSDD", &result, nullptr, 0, 0);
    IOLog("[CHUWIFanProbe] PTID.OSDD status=0x%x result=%s\n",
          status,
          result ? result->getMetaClass()->getClassName() : "(null)");

    if (status == kIOReturnSuccess && result) {
        OSArray *array = OSDynamicCast(OSArray, result);
        if (array) {
            IOLog("[CHUWIFanProbe] OSDD array count=%u\n", array->getCount());
            for (unsigned int i = 0; i < array->getCount(); ++i) {
                OSObject *item = array->getObject(i);
                OSNumber *number = OSDynamicCast(OSNumber, item);
                OSData *data = OSDynamicCast(OSData, item);
                if (number) {
                    IOLog("[CHUWIFanProbe] OSDD[%u] OSNumber=%llu (0x%llx)\n",
                          i,
                          (unsigned long long)number->unsigned64BitValue(),
                          (unsigned long long)number->unsigned64BitValue());
                } else if (data) {
                    IOLog("[CHUWIFanProbe] OSDD[%u] OSData length=%u\n",
                          i, data->getLength());
                } else if (item) {
                    IOLog("[CHUWIFanProbe] OSDD[%u] type=%s\n",
                          i, item->getMetaClass()->getClassName());
                } else {
                    IOLog("[CHUWIFanProbe] OSDD[%u] is null\n", i);
                }
            }
        } else {
            IOLog("[CHUWIFanProbe] OSDD result is not an OSArray\n");
        }
        result->release();
    } else {
        if (result) result->release();
        IOLog("[CHUWIFanProbe] OSDD unavailable; no EC writes attempted\n");
    }

    return true;
}

void CHUWIFanProbe::stop(IOService *provider) {
    IOLog("[CHUWIFanProbe] stopped\n");
    super::stop(provider);
}
