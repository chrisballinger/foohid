/**
 * Create a virtual keyboard.
 * Compile me with: gcc keyboard.c -o virtual_keyboard -framework IOKit
 */

#include <IOKit/IOKitLib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char report_descriptor[] = {
     0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x81, 0x01,                    //   INPUT (Cnst,Ary,Abs)
    0x95, 0x05,                    //   REPORT_COUNT (5)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x05, 0x08,                    //   USAGE_PAGE (LEDs)
    0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
    0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x03,                    //   REPORT_SIZE (3)
    0x91, 0x01,                    //   OUTPUT (Cnst,Ary,Abs)
    0x95, 0x06,                    //   REPORT_COUNT (6)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0x09, 0x00,                    //   USAGE (Reserved (no event indicated))
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x7f,                    //   LOGICAL_MAXIMUM (127)
    0xb1, 0x02,                    //   FEATURE (Data,Var,Abs)
    0xc0                           // END_COLLECTION
};

struct keyboard_report_t {
    uint8_t modifier;
    uint8_t reserved;
    uint8_t key_codes[6];
    uint8_t reserved_bis;
};

#define SERVICE_NAME "it_unbit_foohid"

#define FOOHID_CREATE 0  // create selector
#define FOOHID_SEND 2  // send selector

#define DEVICE_NAME "Foohid Virtual KB"
#define DEVICE_SN "SN 123456"

int main() {
    io_iterator_t iterator;
    io_service_t service;
    io_connect_t connect;

    // Get a reference to the IOService
    kern_return_t ret = IOServiceGetMatchingServices(kIOMasterPortDefault, IOServiceMatching(SERVICE_NAME), &iterator);

    if (ret != KERN_SUCCESS) {
        printf("Unable to access IOService.\n");
        exit(1);
    }

    // Iterate till success
    int found = 0;
    while ((service = IOIteratorNext(iterator)) != IO_OBJECT_NULL) {
        ret = IOServiceOpen(service, mach_task_self(), 0, &connect);

        if (ret == KERN_SUCCESS) {
            found = 1;
            break;
        }

        IOObjectRelease(service);
    }
    IOObjectRelease(iterator);

    if (!found) {
        printf("Unable to open IOService.\n");
        exit(1);
    }

    // Fill up the input arguments.
    uint32_t input_count = 8;
    uint64_t input[input_count];
    input[0] = (uint64_t) strdup(DEVICE_NAME);  // device name
    input[1] = strlen((char *)input[0]);  // name length
    input[2] = (uint64_t) report_descriptor;  // report descriptor
    input[3] = sizeof(report_descriptor);  // report descriptor len
    input[4] = (uint64_t) strdup(DEVICE_SN);  // serial number
    input[5] = strlen((char *)input[4]);  // serial number len
    input[6] = (uint64_t) 2;  // vendor ID
    input[7] = (uint64_t) 3;  // device ID

    ret = IOConnectCallScalarMethod(connect, FOOHID_CREATE, input, input_count, NULL, 0);
    if (ret != KERN_SUCCESS) {
        printf("Unable to create HID device. May be fine if created previously.\n");
    }

    // Arguments to be passed through the HID message.
    struct keyboard_report_t keyboard;
    uint32_t send_count = 4;
    uint64_t send[send_count];
    send[0] = (uint64_t)input[0];  // device name
    send[1] = strlen((char *)input[0]);  // name length
    send[2] = (uint64_t) &keyboard;  // keyboard struct
    send[3] = sizeof(struct keyboard_report_t);  // keyboard struct len

    int i = 0;
    for(i = 0; ; i++) {
        keyboard.modifier = 0;

        if (i % 2 == 0) {
            keyboard.key_codes[0] = 0x04;
        } else {
            keyboard.key_codes[0] = 0x00;
        }

        ret = IOConnectCallScalarMethod(connect, FOOHID_SEND, send, send_count, NULL, 0);
        if (ret != KERN_SUCCESS) {
            printf("Unable to send message to HID device.\n");
        }

        sleep(1);  // sleep for a second
    }
}
