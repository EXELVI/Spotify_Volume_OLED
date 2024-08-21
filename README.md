# 🎵 Spotify Volume OLED

[![GitHub stars](https://img.shields.io/github/stars/EXELVI/Spotify_Volume_OLED?style=for-the-badge)](https://github.com/EXELVI/Spotify_Volume_OLED/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/EXELVI/Spotify_Volume_OLED?style=for-the-badge)](https://github.com/EXELVI/Spotify_Volume_OLED)
[![GitHub issues](https://img.shields.io/github/issues/EXELVI/Spotify_Volume_OLED?style=for-the-badge)](https://github.com/EXELVI/Spotify_Volume_OLED/issues)
[![Last Commit](https://img.shields.io/github/last-commit/EXELVI/Spotify_Volume_OLED?style=for-the-badge)](https://github.com/EXELVI/Spotify_Volume_OLED/commits/main)

This project uses an OLED display to visualize and control the volume of your Spotify playback devices using a physical rotary encoder.

## 📜 Overview

**Spotify Volume OLED** allows you to monitor and adjust the volume of your Spotify playback devices using an Arduino-based setup. The project features a rotary encoder to change the volume, and an OLED display to visualize the volume level and device status.

## 🚀 Features

- **Real-time Volume Control**: Adjust the volume of your Spotify devices in real-time.
- **OLED Display**: Visualizes the current volume level, device name, and Wi-Fi connection status.
- **Rotary Encoder Interface**: Simple and intuitive volume control using a rotary encoder.

## 🛠️ Hardware Requirements

- Arduino UNO R4 WiFi
- 0.96" OLED Display (SSD1306)
- Rotary Encoder
- LED Matrix Display 

## 📦 Installation

1. **Clone the repository**:
    ```bash
    git clone https://github.com/EXELVI/Spotify_Volume_OLED.git
    ```

2. **Install Required Libraries**:
    Make sure to install the following Arduino libraries:
    - `Adafruit GFX Library`
    - `Adafruit SSD1306`
    - `Arduino_JSON`
    - `WiFi`
    - `ArduinoGraphics`
    - `Arduino_LED_Matrix`

3. **Configure Secrets**:
    Create a file named `arduino_secrets.h` and add your WiFi credentials and Spotify API credentials:
    ```cpp
    #define SECRET_SSID "your-SSID"
    #define SECRET_PASS "your-password"
    #define SECRET_CLIENT_ID "your-spotify-client-id"
    #define SECRET_CLIENT_SECRET "your-spotify-client-secret"
    #define SECRET_BASE64 "your-base64-auth"
    ```

## 📋 Usage

1. **Upload the Sketch**:
    Upload the Arduino sketch to your board using the Arduino IDE.

2. **Connect to Wi-Fi**:
    The device will attempt to connect to the Wi-Fi network using the credentials provided.

3. **Authorize Spotify**:
    Access the provided URL from the OLED display using a web browser to authorize your Spotify account.

4. **Control the Volume**:
    Use the rotary encoder to adjust the volume and observe the changes on the OLED display.

## 🔌 Pins

The following pins are used in the project:

| Pin | Component |
| --- | --------- |
| 3   | CLK       |
| 4   | DT        |
| 2   | SW        |
| SDA | OLED SDA  |
| SCL | OLED SCL  |


## 🤝 Contributing

Contributions are welcome! Please open an issue or submit a pull request for any improvements or bug fixes.