#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <cstdlib>
#include <string>

using namespace std;

const char* I2C_BUS = "/dev/i2c-1"; // I2C bus device
const int ESP32_ADDR = 0x08;         // ESP32 slave address

int main() {
    // Open I2C bus
    int file;
    if ((file = open(I2C_BUS, O_RDWR)) < 0) {
        cerr << "Failed to open the I2C bus" << endl;
        return 1;
    }

    // Specify the address of the I2C Slave to communicate with
    if (ioctl(file, I2C_SLAVE, ESP32_ADDR) < 0) {
        cerr << "Failed to acquire bus access or talk to slave." << endl;
        return 1;
    }

    string choice;
    while (true) {
        cout << "\nSelect direction:\n";
        cout << "1 - Forward\n";
        cout << "2 - Backward\n";
        cout << "3 - Left\n";
        cout << "4 - Right\n";
        cout << "0 - Exit\n";
        cout << ">>>> ";
        cin >> choice;

        int code = 0;
        string direction;

        if (choice == "0") break;
        else if (choice == "1") { direction = "Forward"; code = 1; }
        else if (choice == "2") { direction = "Backward"; code = 2; }
        else if (choice == "3") { direction = "Left"; code = 3; }
        else if (choice == "4") { direction = "Right"; code = 4; }
        else {
            cout << "Invalid choice, enter 0-4" << endl;
            continue;
        }

        // Send code as a single byte over I2C
        if (write(file, &code, 1) != 1) {
            cerr << "Failed to write to the I2C bus" << endl;
        } else {
            cout << "Sent " << direction << " (" << code << ") to ESP32" << endl;
        }
    }

    close(file);
    cout << "Exiting..." << endl;
    return 0;
}

