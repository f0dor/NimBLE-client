# BLE Client-side implementation of a speed (time) measuring device

## Description
Using ESP32-C3 microcontroller and the NimBLE stack, I've managed to create a server (master) implementation of a speed measuring device. The device is able to measure the time it takes an object to travel between two points and calculate its speed. The time it took to traverse the distance between the two points is calculated by measuring the time it takes for the object to pass two HC-SR04 sensors, and is then displayed on a small display on the master device.

## Features
- Lightweight and efficient implementation
- Easy to use (no retroreflector needed)
- Two buttons only

## Installation
1. Clone the repository.
2. Install the required dependencies.
3. Build the project.

## Usage
1. Connect your ESP32-C3 via USB to your computer.
2. Flash your ESP32-C3 device with this code.
3. Monitor the workings of the server via the serial monitor.

## Contributing
Contributions are welcome!

## License
This project is licensed under the [MIT License](LICENSE).

## Contact
- Email: [marko05simic@gmail.com](mailto:marko05simic@gmail.com)