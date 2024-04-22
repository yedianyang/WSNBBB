#include <iostream>
#include <libusb-1.0/libusb.h>
#include <cstring>
#include <unistd.h>


int main() {
    libusb_device_handle* dev_handle = nullptr; // Handle for USB device
    int r; // For return values
    libusb_init(nullptr); // Initialize the LIBUSB library

    // Open the device with the Vendor ID and Product ID
    dev_handle = libusb_open_device_with_vid_pid(nullptr, 0x056A, 0x0221);
    if (dev_handle == nullptr) {
        std::cerr << "Cannot open device\n";
        return 1;
    }

    // Detach kernel driver if necessary
    if (libusb_kernel_driver_active(dev_handle, 0) == 1) {
        libusb_detach_kernel_driver(dev_handle, 0);
    }

    // Claim interface 0 of the device
    r = libusb_claim_interface(dev_handle, 0);
    if (r < 0) {
        std::cerr << "Cannot Claim Interface\n";
        return 1;
    }

    unsigned char data[33]; // Data for control transfers and reading endpoint
    int actual; // Actual bytes transferred

    // USB control transfer for setting report
    data[0] = 0x80;
    data[1] = 0x01;
    data[2] = 0x03;
    data[3] = 0x01;
    data[4] = 0x01;
    std::memset(data + 5, 0, 33 - 5);
    libusb_control_transfer(dev_handle, 0x21, 0x09, 0x0380, 0, data, 33, 0);

    // Another example of a control transfer
    data[0] = 0x80;
    data[1] = 0x01;
    data[2] = 0x0A;
    data[3] = 0x01;
    data[4] = 0x01;
    data[5] = 0x0B;
    data[6] = 0x01;
    std::memset(data + 7, 0, 33 - 7);
    libusb_control_transfer(dev_handle, 0x21, 0x09, 0x0380, 0, data, 33, 0);

    // Read from the device
    unsigned char endpoint_address = 0x81; // Endpoint address for reading
    while (true) {
        r = libusb_interrupt_transfer(dev_handle, endpoint_address, data, sizeof(data), &actual, 0);
        if (r == 0) {
            std::cout << "Read " << actual << " bytes: ";
            for (int i = 0; i < actual; ++i)
                std::cout << std::hex << static_cast<int>(data[i]) << " ";
            std::cout << std::endl;
        } else {
            std::cerr << "Read error: " << libusb_error_name(r) << std::endl;
            break;
        }
        sleep(1); // Sleep for a bit before trying again
    }

    // Release interface and close device
    libusb_release_interface(dev_handle, 0);
    libusb_close(dev_handle);
    libusb_exit(nullptr); // Close the session
    return 0;
}