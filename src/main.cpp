#include <Arduino.h>
#include <unordered_set>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <totp.hpp>
#include "HTTPDateClient.h"

std::unordered_set<uint64_t>* wifiBlacklist;
TOTP *totp;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 15 * 60000);
HTTPDateClient httpTimeClient("http://www.google.com/", 0, 15 * 60000);
String latestTOTP;
long latestTOTPtime = 0;
long iterations = 0;

enum class State
{
  Connecting,
  Working
};
State curState = State::Connecting;

enum class TimeSyncStrat
{
  NTP,
  HTTP
};
TimeSyncStrat tsStrat = TimeSyncStrat::NTP;

void setup()
{
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(16, OUTPUT);
  digitalWrite(16, 1);
  wifiBlacklist = new std::unordered_set<uint64_t>();
  Serial.begin(115200);
  WiFi.persistent(false);

  // put your setup code here, to run once:
  totp = new TOTP("KAZGMUTLNFTFKVCNG5RW2ODQJA2TEZCUMVDVGNBVLFMVCUTWGJREM4KLO5ZVGWDLKZ2DE6KELJZVSN3TKFLWW2KBPFFDKUCTOBYG4WQ=", 120, 4);

  latestTOTP = "";
}

uint64_t sumBSSID(uint8_t *bssid)
{
  uint64_t sum = 0;
  for (size_t i = 0; i < 6; i++)
  {
    sum = (sum << 8) | bssid[i];
  }
  return sum;
}

void loopConnecting()
{
  Serial.println("Finding open Wi-Fi network...");
  digitalWrite(12, 0);
  digitalWrite(13, 1);
  digitalWrite(16, 1);

  String foundSSID;
  uint8_t *foundBSSID;
  String foundBSSIDstr;

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  bool keepScanning = true;
  while (keepScanning)
  {
    int found = WiFi.scanNetworks();
    Serial.println("Scan complete");
    if (!found)
    {
      Serial.println("No Wi-Fi networks found, scanning again...");
      continue;
    }
    for (int i = 0; i < found; i++)
    {
      foundSSID = WiFi.SSID(i);
      foundBSSID = WiFi.BSSID(i);
      foundBSSIDstr = WiFi.BSSIDstr(i);
      uint8_t enc = WiFi.encryptionType(i);
      Serial.println("Found " + foundSSID + " (" + (enc == ENC_TYPE_NONE ? "open" : "closed") + ")");
      if (enc == ENC_TYPE_NONE && !wifiBlacklist->count(sumBSSID(foundBSSID)))
      {        
        keepScanning = false;
        break;
      }
    }
    if(!keepScanning) {
      break;
    }
    Serial.println("No open Wi-Fi networks found, scanning again...");
    // clear blacklist so we try every network again
    wifiBlacklist->clear();
    delay(100);
  }

  WiFi.begin(foundSSID.c_str(), nullptr, 0, foundBSSID);

  digitalWrite(16, 0);
  Serial.print("WiFi connecting to " + foundSSID + " (" + foundBSSIDstr + ")");
  int iters = 0;
  while (WiFi.status() != WL_CONNECTED && iters++ < 30)
  {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.disconnect();
    Serial.println(" timeout");
    wifiBlacklist->insert(sumBSSID(foundBSSID));
    // we didn't change state, loopConnecting will run again
    return;
  }
  Serial.println(" connected");
  delay(500);

  timeClient.begin();
  Serial.println("Attempting to obtain time through NTP...");
  if (timeClient.forceUpdate())
  {
    Serial.println("Current time obtained through NTP");
    curState = State::Working;
    tsStrat = TimeSyncStrat::NTP;
    return;
  }
  timeClient.end();

  Serial.println("Attempting to obtain time through HTTP...");
  httpTimeClient.begin();
  if (httpTimeClient.forceUpdate())
  {
    Serial.println("Current time obtained through HTTP");
    curState = State::Working;
    tsStrat = TimeSyncStrat::HTTP;
    return;
  }
  httpTimeClient.end();

  Serial.println("Could not obtain current time through network " + foundSSID + " (" + foundBSSIDstr + ")");
  wifiBlacklist->insert(sumBSSID(foundBSSID));
  // we didn't change state, loopConnecting will run again
}

void loopWorking()
{
  digitalWrite(12, 1);
  digitalWrite(13, 0);
  digitalWrite(16, 1);
  // put your main code here, to run repeatedly:
  long codeTime;
  if (tsStrat == TimeSyncStrat::NTP)
  {
    if (!timeClient.update())
    {
      Serial.println("Error updating time through NTP");
    }
    codeTime = timeClient.getEpochTime();
  }
  else if (tsStrat == TimeSyncStrat::HTTP)
  {
    if (!httpTimeClient.update())
    {
      Serial.println("Error updating time through HTTP");
    }
    codeTime = httpTimeClient.getEpochTime();
  }

  char curTOTP[15];
  totp->getCode(codeTime, curTOTP);
  iterations++;
  if (strcmp(curTOTP, latestTOTP.c_str()))
  {
    String ssid = "CROSS-" + String(curTOTP);
    WiFi.softAP(ssid.c_str(), "qazedcol");
    if (tsStrat == TimeSyncStrat::NTP)
      Serial.println("Current UTC date and time is " + timeClient.getFormattedTime());
    else if (tsStrat == TimeSyncStrat::HTTP)
      Serial.println("Current UTC date and time is " + httpTimeClient.getFormattedTime());

    if (latestTOTPtime > 0)
    {
      Serial.printf("Calculating %d codes/second\n", iterations / (codeTime - latestTOTPtime));
    }
    Serial.println("Updated SSID to " + ssid);
    Serial.println("My BSSID is " + WiFi.softAPmacAddress());

    latestTOTP = String(curTOTP);
    latestTOTPtime = codeTime;
    iterations = 0;
  }
  delay(500);
}

void loop()
{
  switch (curState)
  {
  case State::Connecting:
    loopConnecting();
    break;
  case State::Working:
    loopWorking();
    break;
  }
}