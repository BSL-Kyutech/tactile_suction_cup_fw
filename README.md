# np2040con_rosbridge_8ch_pub

A sample code for publishing float numbers from Arduino Nano RP2040 Connect via rosbridge_server.\

# Prerequisites

Please make sure that your arduino IDE has ArduinoJSON and WebSockets2_Generic installed.

# Usage

1. update the "defines.h". At least, "ssid", "password", and "websockets_server_host" have to be updated.
1. open the ino file and compile/write and keep it off
1. run "ros2 launch rosbridge_server rosbridge_websocket_launch.xml" on the machine that has the IP specified as "websockets_server_host".
1. turn on your device.