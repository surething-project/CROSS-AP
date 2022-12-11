/**
 * The MIT License (MIT)
 * Copyright (c) 2015 by Fabrice Weinberg
 * Copyright (c) 2019 by Gabriel Maia
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "HTTPDateClient.h"
#include "sscanf.h"
extern "C"
{
#include "user_interface.h"
}

HTTPDateClient::HTTPDateClient()
{
}

HTTPDateClient::HTTPDateClient(int timeOffset)
{
  this->_timeOffset = timeOffset;
}

HTTPDateClient::HTTPDateClient(const char *httpServerName)
{
  this->_httpServerName = httpServerName;
}

HTTPDateClient::HTTPDateClient(const char *httpServerName, int timeOffset)
{
  this->_timeOffset = timeOffset;
  this->_httpServerName = httpServerName;
}

HTTPDateClient::HTTPDateClient(const char *httpServerName, int timeOffset, int updateInterval)
{
  this->_timeOffset = timeOffset;
  this->_httpServerName = httpServerName;
  this->_updateInterval = updateInterval;
}

void HTTPDateClient::begin()
{
  this->_httpSetup = true;
}

bool HTTPDateClient::forceUpdate()
{
  return forceUpdate(this->_httpServerName, 0);
}

bool HTTPDateClient::forceUpdate(String url, int recurDepth)
{
#ifdef DEBUG_HTTPDateClient
  Serial.println("Update from HTTP Server");
#endif

  unsigned long reqStart = millis();
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;

  https.setTimeout(5000);
  bool beginResult = false;
  if(url.startsWith("https://")) {
    beginResult = https.begin(*client, url);
  } else {
    beginResult = https.begin(url);
  }
  if (!beginResult) {
    Serial.println("Failure on HTTP begin");
  }

  const char *headerKeys[] = {"date", "location"};
  https.collectHeaders(headerKeys, 2);
  int code = https.GET();
  if (code < 200 || code >= 400)
  {
    Serial.println(String("Error HTTP response code (") + code + ")");
    https.end();
    return false;
  }

  String headerDate = https.header("date");

  if (headerDate.length() < 10)
  {
    if ((code == 301 || code == 302 || code == 307) && recurDepth < 5)
    {
      // redirects injected by captive portals may not include the date header (even though the captive portal login page includes it)
      // so, attempt to follow redirect
      String headerLocation = https.header("location");
      if (headerLocation.length() > 0)
      {
        https.end();
        Serial.println("Following redirect to " + headerLocation);
        return forceUpdate(headerLocation, recurDepth + 1);
      }
    }
    Serial.println("Invalid date header in HTTP response");
    https.end();
    return false;
  }

  https.end();

  int h, m, s, d, month = -1, y;
  char month_name[4];
  // headerDate contains something like Mon, 08 Apr 2019 15:12:46 GMT
  int read = sscanf(headerDate.c_str(), "%4*s %d %3s %4d %2d:%2d:%2d", &d, month_name, &y, &h, &m, &s);
  if (read != 6)
  {
    Serial.println("Error parsing HTTP date (read " + String(read) + ")");
    return false;
  }
  const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  for (int i = 0; i < 12; i++)
  {
    if (!strncmp(months[i], month_name, 3))
    {
      month = i;
    }
  }
  if (month < 0)
  {
    Serial.println("Invalid month in HTTP date");
    return false;
  }

  // system_mktime expects month in range 1..12
  this->_currentEpoc = system_mktime(y, month + 1, d, h, m, s);

  this->_lastUpdate = reqStart;
  return true;
}

bool HTTPDateClient::update()
{
  if ((millis() - this->_lastUpdate >= this->_updateInterval) // Update after _updateInterval
      || this->_lastUpdate == 0)
  { // Update if there was no update yet.
    if (!this->_httpSetup)
      this->begin(); // setup the UDP client if needed
    return this->forceUpdate();
  }
  return true;
}

unsigned long HTTPDateClient::getEpochTime()
{
  return this->_timeOffset +                      // User offset
         this->_currentEpoc +                     // Epoc returned by the NTP server
         ((millis() - this->_lastUpdate) / 1000); // Time since last update
}

int HTTPDateClient::getDay()
{
  return (((this->getEpochTime() / 86400L) + 4) % 7); //0 is Sunday
}
int HTTPDateClient::getHours()
{
  return ((this->getEpochTime() % 86400L) / 3600);
}
int HTTPDateClient::getMinutes()
{
  return ((this->getEpochTime() % 3600) / 60);
}
int HTTPDateClient::getSeconds()
{
  return (this->getEpochTime() % 60);
}

String HTTPDateClient::getFormattedTime()
{
  unsigned long rawTime = this->getEpochTime();
  unsigned long hours = (rawTime % 86400L) / 3600;
  String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

  unsigned long minutes = (rawTime % 3600) / 60;
  String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

  unsigned long seconds = rawTime % 60;
  String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

  return hoursStr + ":" + minuteStr + ":" + secondStr;
}

void HTTPDateClient::end()
{
  this->_httpSetup = false;
}

void HTTPDateClient::setTimeOffset(int timeOffset)
{
  this->_timeOffset = timeOffset;
}

void HTTPDateClient::setUpdateInterval(int updateInterval)
{
  this->_updateInterval = updateInterval;
}