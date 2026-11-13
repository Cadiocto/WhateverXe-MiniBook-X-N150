// DSI rotation helper for CHUWI MiniBook X N150.
// Targets only the verified built-in AppleIntelFramebuffer@0.
// Does not modify EFI, kexts, boot arguments, or system security settings.

#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/graphics/IOGraphicsLib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#define WEX_DSI_PATH "/IGPU@2/AppleIntelFramebuffer@0"
#define WEX_IOFB_SET_TRANSFORM 0x00000400U
#define WEX_IO_SCALE_ROTATE_270 0x50U
#define WEX_DSI_ROTATE_OPTIONS (WEX_IOFB_SET_TRANSFORM | (WEX_IO_SCALE_ROTATE_270 << 16))
#define WEX_EXPECTED_TRANSFORM 80
#define WEX_ATTEMPTS 30
#define WEX_RETRY_USEC 2000000

static int numberProperty(io_service_t service, CFStringRef key, int64_t *out) {
    CFTypeRef value = IORegistryEntryCreateCFProperty(service, key, kCFAllocatorDefault, 0);
    if (!value) return 0;
    int ok = CFGetTypeID(value) == CFNumberGetTypeID() &&
             CFNumberGetValue((CFNumberRef)value, kCFNumberSInt64Type, out);
    CFRelease(value);
    return ok;
}

static int findDSI(io_service_t *outService) {
    CGDirectDisplayID displays[16];
    uint32_t count = 0;
    if (CGGetOnlineDisplayList(16, displays, &count) != kCGErrorSuccess) return 0;

    for (uint32_t i = 0; i < count; ++i) {
        if (!CGDisplayIsBuiltin(displays[i])) continue;
        io_service_t service = CGDisplayIOServicePort(displays[i]);
        if (!service) continue;

        char path[1024] = {0};
        if (IORegistryEntryGetPath(service, kIOServicePlane, path) != KERN_SUCCESS ||
            !strstr(path, WEX_DSI_PATH)) {
            IOObjectRelease(service);
            continue;
        }
        *outService = service;
        return 1;
    }
    return 0;
}

int main(void) {
    for (unsigned attempt = 1; attempt <= WEX_ATTEMPTS; ++attempt) {
        io_service_t service = IO_OBJECT_NULL;
        if (!findDSI(&service)) {
            fprintf(stderr, "DSI not ready (%u/%u)\n", attempt, WEX_ATTEMPTS);
            usleep(WEX_RETRY_USEC);
            continue;
        }

        int64_t probe = -1, transform = -1;
        const int hasProbe = numberProperty(service, CFSTR("IOFBProbeOptions"), &probe);
        const int hasTransform = numberProperty(service, CFSTR("IOFBTransform"), &transform);

        if (!hasProbe || !(probe & WEX_IOFB_SET_TRANSFORM)) {
            fprintf(stderr, "Refusing request: DSI transform capability not advertised (probe=%lld)\n",
                    (long long)probe);
            IOObjectRelease(service);
            return 3;
        }

        if (hasTransform && transform == WEX_EXPECTED_TRANSFORM) {
            printf("DSI already has desired transform: %lld\n", (long long)transform);
            IOObjectRelease(service);
            return 0;
        }

        kern_return_t result = IOServiceRequestProbe(service, WEX_DSI_ROTATE_OPTIONS);
        int64_t after = -1;
        const int verified = numberProperty(service, CFSTR("IOFBTransform"), &after);
        printf("attempt=%u options=0x%08x request=0x%x transform=%lld verified=%d\n",
               attempt, WEX_DSI_ROTATE_OPTIONS, result, (long long)after, verified);

        IOObjectRelease(service);
        if (result == KERN_SUCCESS && verified && after == WEX_EXPECTED_TRANSFORM)
            return 0;

        usleep(WEX_RETRY_USEC);
    }

    fprintf(stderr, "Could not verify DSI transform 80 after %u attempts\n", WEX_ATTEMPTS);
    return 4;
}
