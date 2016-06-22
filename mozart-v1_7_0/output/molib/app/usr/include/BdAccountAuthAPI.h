
#ifndef __BD_ACCOUNT_AUTH_H_
#define __BD_ACCOUNT_AUTH_H_

#include <stdbool.h>

typedef enum {
    AUTH_ERROR_SUCCESS = 0,
    AUTH_ERROR_FAILED_GET_CODE = 1,
    AUTH_ERROR_KEY_CONFIRM_FAILED = 2,
    AUTH_ERROR_THREAD_LAUNCH_FAIL = 3,
    AUTH_ERROR_ALLOC_FAIL = 4,
    AUTH_ERROR_MISSING_PARAMETERS = 5,
    AUTH_ERROR_AUTH_NOT_STARTED = 6,
} bd_account_auth_err_t;


typedef enum {
    UNAUTH_ERROR_SUCCESS = 0,
    UNAUTH_ERROR_EMPTY_RESPONSE = -1,
    UNAUTH_ERROR_INVALID_RESPONSE = -2,
    UNAUTH_ERROR_SERVER_REFUSE = -3,
} bd_account_unauth_err_t;


#ifdef  __cplusplus
extern "C" {
#endif
//void SendAppUserCodeVerificationUrl(int res, char * user_code_str);

/*
 * bd_account_auth_err_t BDAccountAuthBegin(void (*errorCallback)(bd_account_auth_err_t errorCode), void (*deviceKeyReceived)(const char* deviceKey));
 * Begin the process of authenticating the device with the server.
 * 
 * Parameters:
 *      void (*errorCallback)(bd_account_auth_err_t errorCode)
 *      A callback that will be called on error and when the authentication is complete.
 *
 *      void (*deviceKeyReceived)(const char* deviceKey)
 *      A callback that will be called when a device key is received from server.  The key can be used to allow the device to access user's account.
 *
 * return:
 *      AUTH_ERROR_MISSING_PARAMETERS if errorCallback or deviceKeyReceived is NULL
 *      AUTH_ERROR_THREAD_LAUNCH_FAIL if failed to launch thread for making the authentication requests
 *      AUTH_ERROR_ALLOC_FAIL if failed to allocate resources needed during auth
 *      AUTH_ERROR_SUCCESS on success, the final result will be passed through callbacks
 *
 * Full auth process:
 * 1. BDAccountAuthBegin is called.
 * 2. The library obtains device key and passes it to deviceKeyReceived callback.
 * 3. App uses the the device key to authenticate the device and calls BDAccountAuthDeviceKeyAuthed when it's finished.
 * 4. The library finalises the authentication and calls errorCallback with error code AUTH_ERROR_SUCCESS to indicate success.
 */
bd_account_auth_err_t BDAccountAuthBegin(void (*errorCallback)(bd_account_auth_err_t errorCode), void (*deviceKeyReceived)(const char* deviceKey));

/*
 * Call this to finalize the auth after the device key passed to deviceKeyReceived callback and App finished the negotiation with the internet server
 * The final result will be passed through the errorCallback passed to BDAccountAuthBegin
 * returns AUTH_ERROR_AUTH_NOT_STARTED if no pending auth was found, else returns AUTH_ERROR_SUCCESS
 */
bd_account_auth_err_t BDAccountAuthDeviceKeyAuthed();

/*
 * Call this to cancel started authenticate process
 */
void BDAccountAuthCancel();

/*
  * bool BdAccountGetStatus (void);
  * Check if device is authenticated to user's account.
  * return:
  *     true if authenticated, else false.
  */
bool BdAccountGetStatus (void);

/*
  * void BdAccountDropKeys (void);
  * Remove the local keys for accessing user's account. No requests to server will be made.
  * Usually used before Authentification process.
  */
void BdAccountDropKeys (void);

/*
  * int BdAccountUnauth (void (*resultCallback)(int errorCode));
  * Make a request to server to invaidate the local keys.
  * Remove the local keys for accessing user's account.
  * Used when users select to unbind the device from their account.
  * 
  * The result is passed by a call to resultCallback. 
  * This function is synchronous and might block the caller.
  */
int BdAccountUnauth (void (*resultCallback)(bd_account_unauth_err_t errorCode));

#ifdef  __cplusplus
}
#endif

#endif /*__BD_ACCOUNT_AUTH_H_*/

