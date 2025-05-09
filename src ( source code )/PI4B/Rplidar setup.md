```bash
sudo apt update
sudo apt install git build-essential
git clone https://github.com/Slamtec/rplidar_sdk.git
cd rplidar_sdk
make
sudo ./output/Linux/Release/ultra_simple --channel --serial /dev/serial0 460800
```
