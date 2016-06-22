#ifndef DEVICECONTROLERRORCODES_H_INCLUDED
#define DEVICECONTROLERRORCODES_H_INCLUDED

#define BaiduAccountErrorBase 1000
#define OfflineVRErrorBase 1500
#define OnlineVRErrorBase 1600

// ========== BAIDU account related ==========
// Use these codes with void BDSettingServ_BaiduAccountAuthFailed(int error);
    // Use this if have internal problems finishing the request in void BDSettingServ_Callbacks::beginAuthBaiduAccount()
    #define BaiduAccountAuthRequestError (BaiduAccountErrorBase+1)
    // Problems caused by server or network issues
    #define BaiduAccountAuthErrGetCredentialsFail (BaiduAccountErrorBase+0)
    #define BaiduAccountAuthErrAuthKeyInvalidated (BaiduAccountErrorBase+2)
    #define BaiduAccountAuthErrConfirmFailed (BaiduAccountErrorBase+3)
    // Use this if have internal problems finishing the request in void BDSettingServ_Callbacks::getBaiduAccountAuthState()
    #define BaiduAccountErrGetStatusFailed (BaiduAccountErrorBase+4)
    // Unauthenticate errors
    // Use this if have internal problems finishing the request in void BDSettingServ_Callbacks::dropBaiduAccountKeys()
    #define BaiduAccountDropKeysRequestError (BaiduAccountErrorBase+5)
    // Use this if have internal problems finishing the request in void BDSettingServ_Callbacks::unauthFromBaiduAccount()
    #define BaiduAccountUnauthenticateRequestError (BaiduAccountErrorBase+6)


// ========== Offline vr control related ==========
// Use these codes with void BDSettingServ_OfflineVRControlError(int error);
    // notify error if have internal problems in void BDSettingServ_Callbacks::setOfflineVRState(bool enabled) or void BDSettingServ_Callbacks::getOfflineVRState()
    #define GetOfflineVRStateFailed (OfflineVRErrorBase + 0)
    #define EnableOfflineVRFailed (OfflineVRErrorBase + 1)
    #define DisableOfflineVRFailed (OfflineVRErrorBase + 2)
    // notify error if have internal problems in
    #define GetOfflineVRStandbyFlagStateFaile (OfflineVRErrorBase + 3)
    #define SetOfflineVRInStandbyFlagFailed (OfflineVRErrorBase + 4)
    #define ClearOfflineVRInStandbyFlagFailed (OfflineVRErrorBase + 5)

// ========== Online vr control related ==========
    #define GetOnlineVRConversationModeStateFailed (OnlineVRErrorBase+0)
    #define EnableOnlineVRConversationModeFailed (OnlineVRErrorBase+1)
    #define DisableOnlineVRConversationModeFailed (OnlineVRErrorBase+2)

#endif // DEVICECONTROLERRORCODES_H_INCLUDED
