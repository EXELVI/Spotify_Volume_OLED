#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
#include <Wire.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

ArduinoLEDMatrix matrix;

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET); // Create an instance of the display

int pinA = 3;  // CLK
int pinB = 4;  // DT
int pinSW = 2; // SW
int encoderPosCount = 0;

int pinALast;
int aVal;
boolean bCW;

#include "WiFiS3.h"
#include <WiFiUdp.h>
#include <Arduino_JSON.h>
#include "WiFiSSLClient.h"

#include "icons.h"

// Array of all bitmaps for convenience. (Total bytes used to store images in PROGMEM = 112)
const int epd_bitmap_allArray_LEN = 1;
const unsigned char *epd_bitmap_allArray[1] = {
    epd_bitmap_lock_fill};

const char *host = "api.spotify.com";
#include "arduino_secrets.h"

int wifiStatus = WL_IDLE_STATUS;
WiFiUDP Udp;

WiFiServer server(80);

unsigned long lastConnectionTime = 0;
const unsigned long postingInterval = 30 * 1000; // 30 seconds
JSONVar myObject;

String device_name = "Device";
bool support_volume = true;
int volume = 0;

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
String client_id = SECRET_CLIENT_ID;
String client_secret = SECRET_CLIENT_SECRET;
String base64auth = SECRET_BASE64;
int keyIndex = 0; // your network key index number (needed only for WEP)

String code = "";
String token = "";
bool requestSent = false;
bool waitingCallback = true;

bool editing = false;

int status = WL_IDLE_STATUS;

WiFiSSLClient client;

unsigned long lastBlinkTime = 0;
const unsigned long blinkInterval = 500;

unsigned long refreshTime = 0;
const unsigned long refreshInterval = 100;

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 5;

int clockWiseCount = 0;
int counterClockWiseCount = 0;

String getRandomString(int length)
{
  String randomString;
  char charset[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  for (int n = 0; n < length; n++)
  {
    randomString += charset[random(0, sizeof(charset) - 1)];
  }
  return randomString;
}

String getSpotifyRequestUrl()
{
  String redirect_uri = "http://" + WiFi.localIP().toString() + "/callback";
  String scope = "user-read-playback-state user-modify-playback-state";
  String state = getRandomString(16);

  String url = "https://accounts.spotify.com/authorize?" + String("client_id=") + client_id + String("&response_type=code") + String("&redirect_uri=") + redirect_uri + String("&scope=") + scope + String("&state=") + state;

  Serial.println(url);
  return url;
}

void printToMatrix(char text[])
{

  matrix.beginDraw();
  matrix.clear();
  matrix.textScrollSpeed(100);

  matrix.stroke(0xFFFFFFFF);
  matrix.textFont(Font_5x7);
  matrix.beginText(0, 1, 0xFFFFFF);
  matrix.println(text);
  matrix.endText(SCROLL_LEFT);

  matrix.endDraw();
}

void printIntToMatrix(int number)
{

  matrix.beginDraw();
  matrix.clear();

  matrix.stroke(0xFFFFFFFF);
  char cstr[16];
  itoa(number, cstr, 10);
  matrix.textFont(number < 100 ? Font_5x7 : Font_4x6);
  matrix.beginText(0, 1, 0xFFFFFF);
  matrix.println(cstr);
  matrix.endText(NO_SCROLL);

  matrix.endDraw();
}

void printVolumeBar(int percent) // Into oled
{
  int percentMapped = map(percent, 0, 100, 0, 128);
  display.drawRect(0, 0, 128, 8, WHITE);
  display.fillRect(0, 0, percentMapped, 8, WHITE);
  display.drawRect(49, 7, 30, 9, WHITE);
  display.setCursor(51, 8);
  display.print(String(percent) + "%");
}

bool blinked = false;

void printMain(bool editing)
{
  display.clearDisplay();

  if (!editing)
  {
    display.setCursor(0, 0);
    display.println(device_name);
    printWifiBar();
    display.setCursor(support_volume ? 0 : 35, 8);
    display.println("Volume:");
  }
  else
  {
    printVolumeBar(encoderPosCount);
  }

  if (!support_volume)
  {
    display.drawBitmap(0, 10, epd_bitmap_allArray[0], 32, 23, WHITE);
  }

  display.setCursor(support_volume ? 0 : 35, 16);
  display.setTextSize(2);
  display.print(String(volume) + "%");
  display.setTextSize(1);

  display.display();
}

void setup()
{
  Serial.begin(9600);
  while (!Serial)
  {
    ;
  }
  matrix.begin();

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Initialize the OLED display

  int width = display.width();
  int height = display.height();

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);

  pinMode(pinA, INPUT);
  pinMode(pinB, INPUT);
  pinMode(pinSW, INPUT_PULLUP);

  pinALast = digitalRead(pinA);

  if (WiFi.status() == WL_NO_MODULE)
  {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true)
      ;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION)
  {
    Serial.println("Please upgrade the firmware");
  }

  while (status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
  }

  printWifiStatus();
  delay(5000);

  server.begin();

  getSpotifyRequestUrl();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Connect to:");
  display.setCursor(0, 8);
  display.println("http://" + WiFi.localIP().toString() + "/");
  display.setCursor(0, 16);
  display.println("to authorize.");
  display.setCursor(0, 24);
  display.println("Awaiting callback...");
  display.display();
}

void loop()
{
  if (client.available())
  {
    if (token == "")
    {
      String line = client.readStringUntil('\r');
      Serial.println(line);

      if (line == "\n")
      {
        Serial.println("headers received");
        String payload = client.readString();
        JSONVar myObject = JSON.parse(payload);
        Serial.println(payload);

        if (myObject.hasOwnProperty("error"))
        {
          Serial.println("Error");
          display.clearDisplay();
          display.setCursor(0, 0);
          display.println("Error");
          display.setCursor(0, 8);
          display.println(myObject["error"]);
          display.setCursor(0, 16);
          display.println(myObject["error_description"]);
          display.display();
          delay(10000);
          token = "";
          code = "";
          waitingCallback = true;
          requestSent = false;
          display.clearDisplay();
          display.setCursor(0, 0);
          display.println("Connect to:");
          display.setCursor(0, 8);
          display.println("http://" + WiFi.localIP().toString() + "/");
          display.setCursor(0, 16);
          display.println("to authorize.");
          display.display();
          client.stop();
          return;
        }
        else if (myObject.hasOwnProperty("access_token"))
        {
          token = (String)myObject["access_token"];
          Serial.println(token);
          display.setCursor(0, 24);
          display.println("Token Received");
          display.display();

          delay(1000);
          client.stop();

          delay(1000);
          Serial.print("Starting connection to server... " + String(host) + ": ");

          display.clearDisplay();
          display.setCursor(0, 0);
          display.println("Starting connection to ");
          display.setCursor(0, 8);
          display.println(host);
          display.display();

          if (client.connect(host, 443))
          {
            Serial.println("connected to server");
            display.setCursor(0, 16);
            display.println("Connected to server");
            display.display();
            client.println("GET /v1/me/player HTTP/1.1");
            client.println("Host: api.spotify.com");
            client.println("Authorization: Bearer " + token);
            client.println("Content-Type: application/json");
            client.println("Accept: application/json");
            client.println("Connection: close");
            client.println();
          }
          else
          {
            Serial.println("connection failed");
            display.setCursor(0, 16);
            display.println("Connection failed");
            display.display();
          }
        }
        else
        {
          Serial.println("Error");
          display.clearDisplay();
          display.setCursor(0, 0);
          display.println("Error");
          display.setCursor(0, 8);
          display.println("Error");
          display.display();
          delay(10000);
          token = "";
          code = "";
          waitingCallback = true;
          requestSent = false;
          display.clearDisplay();
          display.setCursor(0, 0);
          display.println("Connect to:");
          display.setCursor(0, 8);
          display.println("http://" + WiFi.localIP().toString() + "/");
          display.setCursor(0, 16);
          display.println("to authorize.");
          display.display();
          client.stop();
          return;
        }
      }
    }
    else
    {
      String line = client.readStringUntil('\r');
      Serial.println(line);
      // get song name, volume,
      if (line.indexOf("volume_percent") != -1)
      {
        int volumeIndex = line.indexOf("volume_percent") + 17;
        int volumeEndIndex = line.indexOf(",", volumeIndex);
        volume = line.substring(volumeIndex, volumeEndIndex).toInt();
        Serial.println(volume);
        encoderPosCount = volume;
        printMain(editing);
      }
      if (line.indexOf("name") != -1)
      {
        int nameIndex = line.indexOf("\"name\"") + 10;
        int nameEndIndex = line.indexOf(",", nameIndex);
        String name = line.substring(nameIndex, nameEndIndex - 1);
        device_name = name;
        Serial.println(name);
        printMain(editing);
      }
      if (line.indexOf("supports_volume") != -1)
      {
        int supportsVolumeIndex = line.indexOf("supports_volume") + 19;
        int supportsVolumeEndIndex = line.indexOf(",", supportsVolumeIndex);
        String supportsVolume = line.substring(supportsVolumeIndex, supportsVolumeEndIndex);
        support_volume = supportsVolume == "true";
        Serial.println(support_volume);
        printMain(editing);
      }
    }
  }

  if (waitingCallback)
  {
    WiFiClient clientServer = server.available();

    if (clientServer)
    { // callback
      Serial.println("new client");
      String currentLine = "";
      while (clientServer.connected())
      {
        if (clientServer.available())
        {
          char c = clientServer.read();
          Serial.write(c);
          if (c == '\n')
          {
            if (currentLine.length() == 0)
            {
              String curl = getSpotifyRequestUrl();
              clientServer.println("HTTP/1.1 200 OK");
              clientServer.println("Content-type:text/html");
              clientServer.println();
              clientServer.println("<!doctype html>");
              clientServer.println("<html data-bs-theme=\"dark\" lang=\"en\">");
              clientServer.println("  <head>");
              clientServer.println("    <meta charset=\"utf-8\">");
              clientServer.println("    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
              clientServer.println("    <title>Spotify Volume</title>");
              clientServer.println("    <link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css\" rel=\"stylesheet\" integrity=\"sha384-QWTKZyjpPEjISv5WaRU9OFeRpok6YctnYmDr5pNlyT2bRjXh0JMhjY6hW+ALEwIH\" crossorigin=\"anonymous\">");
              clientServer.println("  </head>");
              clientServer.println("  <body>");
              clientServer.println("    <div class=\"container-md text-center\">");
              clientServer.println("      <h1 class=\"display-5\">Spotify Volume</h1>");
              clientServer.println("      <a class=\"btn btn-primary\" role=\"button\" href=\"" + curl + "\">Callback Url</a>");
              clientServer.println("     </div>");
              clientServer.println("    <script src=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/js/bootstrap.bundle.min.js\" integrity=\"sha384-YvpcrYf0tY3lHB60NNkmXc5s9fDVZLESaAA55NDzOxhy9GkcIdslK1eN7N6jIeHz\" crossorigin=\"anonymous\"></script>");
              clientServer.println("  </body>");
              clientServer.println("</html>");
              clientServer.println();

              break;
            }
            else
            {
              currentLine = "";
            }
          }
          else if (c != '\r')
          {
            currentLine += c;
          }
          if (currentLine.indexOf("GET /callback?code=") != -1 && currentLine.indexOf("HTTP/1.1") != -1)
          {
            int codeIndex = currentLine.indexOf("GET /callback?code=") + 20;
            int stateIndex = currentLine.indexOf("&state=");
            code = currentLine.substring(codeIndex - 1, stateIndex);
            waitingCallback = false;
            Serial.println(code);

            display.clearDisplay();
            display.setCursor(0, 0);
            display.println("Callback Received");
            display.display();

            clientServer.println("HTTP/1.1 200 OK");
            clientServer.println("Content-type:text/html");
            clientServer.println();
            clientServer.println("<!doctype html>");
            clientServer.println("<html data-bs-theme=\"dark\" lang=\"en\">");
            clientServer.println("  <head>");
            clientServer.println("    <meta charset=\"utf-8\">");
            clientServer.println("    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            clientServer.println("    <title>Spotify Volume</title>");
            clientServer.println("    <link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css\" rel=\"stylesheet\" integrity=\"sha384-QWTKZyjpPEjISv5WaRU9OFeRpok6YctnYmDr5pNlyT2bRjXh0JMhjY6hW+ALEwIH\" crossorigin=\"anonymous\">");
            clientServer.println("  </head>");
            clientServer.println("  <body>");
            clientServer.println("    <div class=\"container-md text-center\">");
            clientServer.println("      <h1 class=\"display-5\">Spotify Volume</h1>");
            clientServer.println("      <h2 class=\"text-success\">Callback Received</h2>");
            clientServer.println("      <a class=\"btn btn-primary\" role=\"button\" href=\"/\">Home</a>");
            clientServer.println("     </div>");
            clientServer.println("    <script src=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/js/bootstrap.bundle.min.js\" integrity=\"sha384-YvpcrYf0tY3lHB60NNkmXc5s9fDVZLESaAA55NDzOxhy9GkcIdslK1eN7N6jIeHz\" crossorigin=\"anonymous\"></script>");
            clientServer.println("  </body>");
            clientServer.println("</html>");
            clientServer.println();

            break;
          }
        }
      }
    }
  }
  else if (token == "" && !requestSent)
  {

    // get spotify user token from code
    String redirect_uri = "http://" + WiFi.localIP().toString() + "/callback";

    String body = "code=" + code + "&redirect_uri=" + redirect_uri + "&grant_type=authorization_code";
    String auth = base64auth;
    String headers = "content-type: application/x-www-form-urlencoded\nAuthorization: Basic " + auth;

    // request
    client.stop();

    Serial.print("\nStarting connection to server accounts.spotify.com: ");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Starting connection to ");
    display.setCursor(0, 8);
    display.println("accounts.spotify.com");
    display.display();

    if (client.connect("accounts.spotify.com", 443))
    {
      Serial.println("connected to server");
      display.setCursor(0, 16);
      display.println("Connected to server");
      display.display();
      client.println("POST /api/token HTTP/1.1");
      client.println("Host: accounts.spotify.com");
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.println("Authorization: Basic " + auth);
      client.print("Content-Length: ");
      client.println(body.length());
      client.println();
      client.println(body);

      lastConnectionTime = millis();
      requestSent = true;
    }
    else
    {
      Serial.println("connection failed");
      display.setCursor(0, 16);
      display.println("Connection failed");
      display.display();
    }
  }
  else if (token != "")
  {
    aVal = digitalRead(pinA);
    if (aVal != pinALast)
    {

      if (!support_volume)
      {
        for (int i = 0; i < 3; i++)
        {
          display.drawBitmap(0, 10, epd_bitmap_allArray[0], 32, 23, WHITE);
          display.display();
          delay(500);
          display.clearDisplay();
          display.setCursor(0, 0);
          display.println(device_name);
          printWifiBar();
          display.setCursor(support_volume ? 0 : 35, 8);
          display.println("Volume:");
          display.setCursor(support_volume ? 0 : 35, 16);
          display.setTextSize(2);
          display.print(String(volume) + "%");
          display.setTextSize(1);
          display.display();
          delay(500);
        }
        printMain(editing);
        editing = false;
        return;
      }

      editing = true;

      if (digitalRead(pinB) != aVal)
      {
        encoderPosCount++;
        encoderPosCount++;
        bCW = true;
        if (encoderPosCount > 100)
        {
          encoderPosCount = 100;
        }
      }
      else
      {
        encoderPosCount--;
        encoderPosCount--;
        bCW = false;
        if (encoderPosCount < 0)
        {
          encoderPosCount = 0;
        }
      }
      Serial.print("Rotated: ");
      Serial.println(bCW ? "clockwise" : "counterclockwise");
      Serial.print("Encoder Position: ");
      Serial.println(encoderPosCount);
      printMain(editing);
    }
    pinALast = aVal;
    if ((millis() - lastConnectionTime > postingInterval) && requestSent && !editing)
    {
      lastConnectionTime = millis();

      Serial.print("Starting connection to server api.spotify.com: ");

      // update
      client.stop();

      Serial.print("Starting connection to server... " + String(host) + ": ");

      if (client.connect(host, 443))
      {
        Serial.println("connected to server");
        client.println("GET /v1/me/player HTTP/1.1");
        client.println("Host: api.spotify.com");
        client.println("Authorization: Bearer " + token);
        client.println("Content-Type: application/json");
        client.println("Accept: application/json");
        client.println("Connection: close");
        client.println();
      }
      else
      {
        Serial.println("connection failed");
      }
    }

    if (digitalRead(pinSW) == LOW)
    {
      if (editing)
      {
        editing = false;

        volume = encoderPosCount;
        printMain(editing);
        // request
        client.stop();

        Serial.print("\nStarting connection to server api.spotify.com: ");

        if (client.connect("api.spotify.com", 443))
        {
          Serial.println("connected to server");
          client.println("PUT /v1/me/player/volume?volume_percent=" + String(encoderPosCount) + " HTTP/1.1");
          client.println("Host: api.spotify.com");
          client.println("Authorization: Bearer " + token);
          client.println("Content-Type: application/json");
          client.println("Content-Length: 0");
          client.println("Accept: application/json");
          client.println("Connection: close");
          client.println();
        }
        else
        {
          Serial.println("connection failed");
        }
        printIntToMatrix(encoderPosCount);
        delay(1000);
      }
    }
  }
}

void printWifiBar() // Function to print the WiFi bar
{
  long rssi = WiFi.RSSI(); // Get the RSSI (Received Signal Strength Indicator)

  // It is a simple function that draws a bar based on the signal strength
  if (rssi > -55)
  {
    display.fillRect(102, 7, 4, 1, WHITE);
    display.fillRect(107, 6, 4, 2, WHITE);
    display.fillRect(112, 4, 4, 4, WHITE);
    display.fillRect(117, 2, 4, 6, WHITE);
    display.fillRect(122, 0, 4, 8, WHITE);
  }
  else if (rssi<-55 & rssi> - 65)
  {
    display.fillRect(102, 7, 4, 1, WHITE);
    display.fillRect(107, 6, 4, 2, WHITE);
    display.fillRect(112, 4, 4, 4, WHITE);
    display.fillRect(117, 2, 4, 6, WHITE);
    display.drawRect(122, 0, 4, 8, WHITE);
  }
  else if (rssi<-65 & rssi> - 75)
  {
    display.fillRect(102, 7, 4, 1, WHITE);
    display.fillRect(107, 6, 4, 2, WHITE);
    display.fillRect(112, 4, 4, 4, WHITE);
    display.drawRect(117, 2, 4, 6, WHITE);
    display.drawRect(122, 0, 4, 8, WHITE);
  }
  else if (rssi<-75 & rssi> - 85)
  {
    display.fillRect(102, 7, 4, 1, WHITE);
    display.fillRect(107, 6, 4, 2, WHITE);
    display.drawRect(112, 4, 4, 4, WHITE);
    display.drawRect(117, 2, 4, 6, WHITE);
    display.drawRect(122, 0, 4, 8, WHITE);
  }
  else if (rssi<-85 & rssi> - 96)
  {
    display.fillRect(102, 7, 4, 1, WHITE);
    display.drawRect(107, 6, 4, 2, WHITE);
    display.drawRect(112, 4, 4, 4, WHITE);
    display.drawRect(117, 2, 4, 6, WHITE);
    display.drawRect(122, 0, 4, 8, WHITE);
  }
  else
  {
    display.drawRect(102, 7, 4, 1, WHITE);
    display.drawRect(107, 6, 4, 2, WHITE);
    display.drawRect(112, 4, 4, 4, WHITE);
    display.drawRect(117, 2, 4, 6, WHITE);
    display.drawRect(122, 0, 4, 8, WHITE);
  }
}

void printWifiStatus() // Function to print the WiFi status
{
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(String(WiFi.SSID()));

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  display.setCursor(0, 20);
  display.println(ip);

  long rssi = WiFi.RSSI();

  printWifiBar();

  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");

  display.setCursor(0, 10);
  display.print(String(rssi) + " dBm");
  display.display();
}