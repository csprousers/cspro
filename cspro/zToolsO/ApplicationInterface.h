#pragma once


// an interface for cross-platform behavior

class ApplicationInterface
{
public:
    virtual ~ApplicationInterface() { }

    // reads a barcode, displaying the (potentially empty) message text to the user;
    // if no barcode is read, return SharableString()
    virtual SharableString BarcodeRead(const std::string& message_text) = 0;


    // returns whether the device is connected to any of the specified types;
    // platform-specific implementations can choose to ignore the parameter flags
    virtual bool IsNetworkConnected(bool wifi, bool mobile) = 0;
};
