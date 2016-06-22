/*
 * DeviceSettingInterface.h
 * Callback interface for controlling the device.
 * All the functions with void return type are assumed asynchronous, the results of the calls
 * should be passed to library by callbacks defined in SettingServer.h
 *
 */


#ifndef __DEVICE_SETTING_INTERFACE_H_
#define __DEVICE_SETTING_INTERFACE_H_

#include <inttypes.h>
#include <stdbool.h>

#define WIFI_INFO_LEN 255
#define WIFI_SSID_LEN 255

struct BD_WifiScanResult_t
{
    char SSID[WIFI_SSID_LEN];             	// Wifi SSID
    char WifiInfo[WIFI_INFO_LEN];         	// Wifi encryption, keymanagement etc info as string in form of [WPA-PSK-CCMP][ESS]
    uint8_t signalLevel;    			                // level converted to scale from 0 to 5
    struct BD_WifiScanResult_t* next;     // results returned as linked list, next node in list
};

class BDSettingServ_Callbacks
{
public:

// Get information about the device
    virtual const char* getDeviceGUID() = 0;          // Return the device GUID as zero terminated hex char array
    virtual const char* getDeviceMCUInfo() = 0;    // Return the description as zero terminated char array, maximum allowed length is 0xFFFF bytes
    virtual const char* getDeviceType() = 0;           // Return the type as zero terminated char array, the maximum allowed length is 19 bytes

// Control offline VR
// Pass the updated state via void BDSettingServ_OfflineVRStateChanged(bool newState); - interface.
// It is safe to call void BDSettingServ_OfflineVRStateChanged(bool newState) directly from callback.
    virtual void setOfflineVRState(bool enabled) = 0;
    virtual void getOfflineVRState() = 0;
    virtual void setOfflineVRInStandbyMode(bool enabled) = 0;
    virtual void getOfflineVRInStandbyMode() = 0;

// control online VR modes
    virtual void setOnlineVrConversationMode(bool enabled) = 0;
    virtual void getOnlineVrConversationMode() = 0;

// Baidu account authentication
    /*
    * Asynchronous requests. The results are passed via following functions defined in SettingServer.h
    *
    * void BDSettingServ_GotBaiduAccountAuthCredentials(const char* authURL, const char* authKey);
    * void BDSettingServ_BaiduAccountAuthStateChanged(bool authed); (used to notify success of auth, unauth, drop keys and current status when being asked, always pass the current auth status)
    * void BDSettingServ_BaiduAccountAuthFailed(int error);
    */
    // auth
    virtual void beginAuthBaiduAccount() = 0;
    virtual void authKeyClearanceComplete() = 0;    // gets called when client has cleared the key, the davice can make the call to get the session secrets etc.
    virtual void cancelAuth() = 0;                  // The user canceled the authentication or an error ocurred in the client.
    // Unauth
    virtual void unauthFromBaiduAccount() = 0;
    virtual void dropBaiduAccountKeys() = 0;
    // get state
    virtual void getBaiduAccountAuthState() = 0;

//  Get/store shared secret
    virtual int setDeviceSecret(const char* deviceSecret) = 0;
    virtual const char* getDeviceSecret() = 0;

// Get/store device location

    /*
     * Store the passed device location. This info may be used when sending drift bottles to nearby users, tuning for local radio stations etc.
     * Parameters:
     *  regionName:     UTF-8 encoded string containing user friendly name of the location, may be NULL if not set by the connected client.
     *  GPSLattitude:   GPS lattitude of the device. range from +90 to -90. 404 if not defined by connecting device.
     *  GPSLongitude:   GPS longitude of the device. range from +180 to -180. 404 if not defined by connecting device.
     */
    virtual int setDeviceLocation(const char* regionName, double GPSLattitude, double GPSLongitude) = 0;

    /*
     * getDeviceLocation()
     *
     * Get previously stored device location.
     *
     * Parameters:
     *  regionName:         on return should contain pointer to zero terminated UTF-8 encoded string containing user friendly location name, Set to NULL if not known.
     *  freeNameWhenDone:   set to true if caller should free pointer returned by regionName when it's not needed anymore.
     *  GPSLattitude:       GPS lattitude of the device. range from +90 to -90. Set to 404 if not known.
     *  GPSLongitude:       GPS longitude of the device. range from +180 to -180. Set to 404 if not known.
     */
    virtual void getDeviceLocation(char** regionName, bool *freeNameWhenDone, double *GPSLattitude, double *GPSLongitude) = 0;

// Wifi setting interface

    // Begin scanning for surrounding wifi networks,
    // when results become available, make a call to void BDSettingServ_WifiScanResultsAvailable(); defined in SettingServer.h
    virtual bool startWifiScan() = 0;

    // Stop scanning surrounding networks
    virtual bool stopWifiScan() = 0;

    // Return list of wifi networks found during last scan
    virtual struct BD_WifiScanResult_t* getWifiScanResults() = 0;

    // Connect to a wifi, properties contains wifi encryption, keymanagement etc info as string in form of [WPA-PSK-CCMP][ESS],
    // notice that properties may be NULL if the phone sending the settings doesn't know it (with iPhone this is a possibility),
    // in this case the program should try to determine the correct properties by it self based on passed parameters (scan for SSID and check what is the correct setup),
    // return 0 if passed parameters are ok
    // return -2 if SSID needs password, but one is not passsed
    // return -3 if needs ident, but one is not passed
    // return -4 if needs password and ident, but neither one was passed.
    virtual int connectToWifi(const char* SSID, const char* properties, const char* password, const char*ident) = 0;

    // Function to check if WIFI AP is open, pass true if in AP mode and AP is open, else return false
    virtual bool isAccesspointOpen()=0;

    // Function to check if in Wifi client mode, pass true if is in client mode AND is connected to WIFI, else return false
    virtual bool isWifiClientConnected()=0;
};

#endif
