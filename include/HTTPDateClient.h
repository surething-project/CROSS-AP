#pragma once

#include <Arduino.h>

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

class HTTPDateClient {
  private:
    bool          _httpSetup      = false;

    const char*   _httpServerName = "google.com"; // Default time server
    int           _timeOffset     = 0;

    unsigned int  _updateInterval = 60000;  // In ms

    unsigned long _currentEpoc    = 0;      // In s
    unsigned long _lastUpdate     = 0;      // In ms

    bool forceUpdate(String url, int recurDepth);

  public:
    HTTPDateClient();
    HTTPDateClient(int timeOffset);
    HTTPDateClient(const char* httpServerName);
    HTTPDateClient(const char* httpServerName, int timeOffset);
    HTTPDateClient(const char* httpServerName, int timeOffset, int updateInterval);

    /**
     * Starts the underlying HTTP client
     */
    void begin();

    /**
     * This should be called in the main loop of your application. By default an update from the HTTP Server is only
     * made every 60 seconds. This can be configured in the HTTPDateClient constructor.
     *
     * @return true on success, false on failure
     */
    bool update();

    /**
     * This will force the update from the HTTP Server.
     *
     * @return true on success, false on failure
     */
    bool forceUpdate();

    int getDay();
    int getHours();
    int getMinutes();
    int getSeconds();

    /**
     * Changes the time offset. Useful for changing timezones dynamically
     */
    void setTimeOffset(int timeOffset);

    /**
     * Set the update interval to another frequency. E.g. useful when the
     * timeOffset should not be set in the constructor
     */
    void setUpdateInterval(int updateInterval);

    /**
     * @return time formatted like `hh:mm:ss`
     */
    String getFormattedTime();

    /**
     * @return time in seconds since Jan. 1, 1970
     */
    unsigned long getEpochTime();

    /**
     * Stops the underlying UDP client
     */
    void end();
};
