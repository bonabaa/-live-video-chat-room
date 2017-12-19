//****************************************************************************
//  File:       TJIPSYS.H 
//  Content:    This module defines the interfaces and messages between 
//              TjIpSys.dll and applications.
//              
//
//  Copyright (c) TigerJet Network Inc., 2001
//
//****************************************************************************

#ifndef TJIPSYS_H
#define TJIPSYS_H

#ifdef __cplusplus
extern "C" {
#endif

#define VK_POUND			0x88			// Virtual key code for '#'
#define VK_ALT_H			0x89			// Virtual key code for Alt+H
#define VK_ALT_U			0x8a			// Virtual key code for Alt+U
#define VK_ALT_TCD			0x8b			// Virtual key code for Alt+TCD
#define VK_CALLGATEWAY		0x8c			// Virtual key code for gateway call
#define VK_HANGUPGATEWAY	0x8d			// Virtual key code for gateway call
#define VK_OFF_HOOK			0x8e			// Virtual key code for off-hook(proslic only)
#define VK_ON_HOOK			0x8f			// Virtual key code for on-hook(proslic only)
// unassigned range C1-DA 
#define VK_SHOWBUDDY		0xC1			// Virtual key code for showing buddylist
#define VK_ACCEPT_CALL		0xC2			// Virtual key code for accepting incoming call
#define VK_BOOK				0xC3			// Virtual key code for book key

typedef BOOL (*TJIPSYSCALL)(int id, int param, void * ex);
typedef void (*PTJIPSYS_CALLBACK)(void* context, UINT message, WPARAM wParam, LPARAM lParam);

enum USER_TO_TJIPSYS_MSG {
	TJIP_SYS_OPEN,						// Open TigerJet device driver
	TJIP_SYS_CLOSE,						// Close TigerJet device driver
	TJIP_TJ560VENDOR_COMMAND,			// Send TigerJet 560 vendor command
	TJIP_READ_TJ560PROSLIC_DIRECT_REG,	// Read Proslic direct reg from Tj560 
	TJIP_WRITE_TJ560PROSLIC_DIRECT_REG,	// Write Proslic direct reg from Tj560
	TJIP_READ_TJ560PROSLIC_INDIRECT_REG,// Read Proslic indirect reg from Tj560
	TJIP_WRITE_TJ560PROSLIC_INDIRECT_REG,// Write Proslic indirect reg from Tj560
	TJIP_SCAN_TJ320PROSLIC_KEY,			// Scan the TigerJet 320 PCI Proslic key input
	TJIP_CHECK_IS_TJ560DEVICE_PLUGGED,	// Check if TigerJet 560 USB device is plugged in
	TJIP_PC2PC,							// Launch PC2PC internet call
	TJIP_PC2PHONE,						// Launch PC2PHONE internet call 
	TJIP_LAUNCH_EXPLORER,				// Launch internet explorer
	TJIP_CHECK_IS_NM301_INSTALLED,		// Check if Netmeeting is installed
	TJIP_INSTALL_NM301,					// Install Netmeeting from program group
	TJIP_SEND_VIRTUAL_KEY,				// Send virtual key to focused desktop app
	TJIP_CLOSE_PC2PC,					// Close PC2PC internet call
	TJIP_START_TJIP_DEV_DETECT,			// Start auto-detection of TjIP devices
	TJIP_STOP_TJIP_DEV_DETECT,			// Stop auto-detection of TjIP devices
	TJIP_HANDLE_DEVICE_CHANGE,			// Handle USB device changes
	TJIP_GET_SERIAL_NUMBER,				// Get serial number of the device
	TJIP_INIT_DTMF_TO_PPG,				// Initialize DTMF tones used by USB PPG
	TJIP_SEND_DTMF_TO_PPG,				// Send DTMF tone to USB PPG
	TJIP_SET_USB_PPG_ON_HOOK,			// Set USB PPG on hook
	TJIP_SET_USB_PPG_OFF_HOOK,			// Set USB PPG off hook
	TJIP_SET_USB_PPG_PROSLIC_CLOCK,		// Set USB PPG + Proslic clock
	TJIP_SET_USB_PPG_PROSLIC_SWITCH,	// Set USB PPG + Proslic switch (bewteen Phone and PPG (DAA))
	TJIP_ECC_INIT,						// Initialize ECC in either microphone or speaker path
	TJIP_ECC_CLOSE,						// Close ECC
	TJIP_ECC_SET_OPTION,				// Set ECC options
	TJIP_ECC_PROCESS_SPEAKER_AUDIO,		// Send speaker audio(source of echo) to ECC
	TJIP_ECC_PROCESS_MICROPHONE_AUDIO,	// Send Microphone audio to ECC, then return the echo cancelled audio back from ECC
	TJIP_OPEN_MIXER,					// Open mixer for Tj560 USB audio for volume control
	TJIP_CLOSE_MIXER,					// Close mixer for Tj560 USB audio for volume control
	TJIP_GET_WAVEIN_VOL,				// Get wavein volume
	TJIP_SET_WAVEIN_VOL,				// Set wavein volume
	TJIP_GET_WAVEOUT_VOL,				// Get waveout volume
	TJIP_SET_WAVEOUT_VOL,				// Set waveout volume
	TJIP_GET_MASTER_WAVEOUT_VOL,		// Get master waveout volume
	TJIP_SET_MASTER_WAVEOUT_VOL,		// Set master waveout volume
	TJIP_GET_MICROPHONE_MUTE,			// Get microphone mute state
	TJIP_SET_MICROPHONE_MUTE,			// Set microphone mute on/off
	TJIP_GET_SPEAKER_MUTE,				// Get speaker mute state
	TJIP_SET_SPEAKER_MUTE,				// Set speaker mute on/off
	TJIP_GET_MASTER_SPEAKER_MUTE,		// Get master speaker mute state
	TJIP_SET_MASTER_SPEAKER_MUTE,		// Set master speaker mute on/off
	TJIP_ON_MIXER_CTRL_CHANGE,			// Function to handle mixer control changes from external applications
	TJIP_SET_CALLBACK,					// Function to register a callback to receive HID kyes from TjIpSys.dll in case that window messaging is not available
	TJIP_SET_DTMF_TO_PPG,				// Set custom DTMF tone via external wave file
	TJIP_SET_TJ320_ECHO_CANCELLER,		// Turn on/off echo canceller (For Tj320 PCI device only)
	TJIP_RESET_TJ320_ECHO_CANCELLER,	// Reset echo canceller (For Tj320 PCI device only)
	TJIP_SET_TJ320_GC,					// Turn on/off Gain Control (For Tj320 PCI device only)
	TJIP_GET_TJ320_DEV_ID,				// Get Tj320 PCI device ID

	// For audio capture
	TJIP_START_AUDIO_CAPTURE,			// Start audio capture on TJ USB device
	TJIP_STOP_AUDIO_CAPTURE,			// Stop audio capture on TJ USB device

	// Audio playback
	TJIP_PLAY_AUDIO_DATA,				// Play audio data to TJ USB device

	// Tj560C related
	TJIP_TJ560C_APP,					// Notify DLL that TJ560C app is running
};

typedef struct _TJ_CALLBACK_INFO
{
	PTJIPSYS_CALLBACK fpCallback;		// Callback function
	void* context;						// Callback context
}TJ_CALLBACK_INFO, *PTJ_CALLBACK_INFO;

typedef struct _MSG_TO_APP			// Message struct so send to applications such as RingCenter
{
	HWND	hwnd;
	WORD	msg;
	int		len;
	PVOID	pdata;
	BOOL	wait;
}MSG_TO_APP, *PMSG_TO_APP;

// MESSAGE between IP Center and TjIpSys.dll
#define WM_TJIPSYS_BASE				(WM_USER+400)
enum TJIpSysCommand{
	WM_TJIP_DEV_ADDED = WM_TJIPSYS_BASE,
	WM_TJIP_DEV_REMOVED,
	WM_TJIP_HID_NEW_KEY,
	WM_TJIP_SHOW,
	WM_TJIP_DECT_EVENT,
	WM_TJIP_AUDIO_DATA,
	WM_TJIP_HID_KEY_RELEASED,
};

typedef enum _TjIpProductID
{
	TJIP_NONE,											  // None

	//
	// Tj320 PCI devices
	//
	TJIP_TJ320PHONE,									  // TigerJet 320 PCI Phone

	//
	// Tj560 USB devices 
	//
	TJIP_TJ560PHONE								= 0x831C, // TigerJet 560 USB Phone
	TJIP_TJ560PHONE_CUSTOM						= 0x831E, // TigerJet 560 USB Phone custom
	TJIP_TJ560PHONE_SWITCH						= 0x831D, // TigerJet 560 USB Phone/Switch
	TJIP_TJ560PHONE_ECHO						= 0x825C, // TigerJet 560 USB Phone with Echo Canceller

	TJIP_TJ560HANDSET							= 0x7210, // TigerJet 560 USB Handset
	TJIP_TJ560HANDSET_COMP						= 0x8250, // TigerJet 560 USB Handset/comp
	TJIP_TJ560HANDSET_KEYPAD					= 0xB210, // TigerJet 560 USB Handset/Keypad
	TJIP_TJ560HANDSET_SWITCH					= 0xB211, // TigerJet 560 USB Handset/Switch
	TJIP_TJ560HANDSET_ID						= 0xB212, // TigerJet 560 USB Handset/ID

	//
	// Tj560B USB devices
	//
	TJIP_TJ560BPHONE_HID						= 0xC31C, // TigerJet 560B USB Phone/HID
	TJIP_TJ560BPHONE_SWITCH_HID					= 0xC31D, // TigerJet 560B USB Phone/Switch using HID
	TJIP_TJ560BPHONE_CUSTOM_HID					= 0xC31E, // TigerJet 560B USB Phone custom using HID
	TJIP_TJ560BPHONE_ECHO_HID					= 0xC25C, // TigerJet 560B USB Phone with external hardware Echo Canceller

	TJIP_TJ560BHANDSET_HID						= 0xF310, // TigerJet 560B USB Handset using HID
	TJIP_TJ560BHANDSET_KEYPAD_HID				= 0xF210, // TigerJet 560B USB Handset/Keypad using HID

	TJIP_TJ560BHANDSET_HID_OKI					= 0xF350, // TigerJet 560B USB Handset(Oki codec) using HID
	TJIP_TJ560BHANDSET_KEYPAD_HID_OKI			= 0xF250, // TigerJet 560B USB Handset/Keypad(Oki codec) using HID
	TJIP_TJ560BHANDSET_KEYPAD_HID_OKI1			= 0xF260, // TigerJet 560B USB Handset/Keypad(Oki codec) using HID

	TJIP_TJ560BPPG								= 0xC290, // TigerJet 560B USB PPG
	TJIP_TJ560BPPG_NO_EC						= 0xC291, // TigerJet 560B USB PPG without Echo Canceller
	TJIP_TJ560BPPGPROSLIC						= 0xC39C, // TigerJet 560B USB PPG + Proslic
	TJIP_TJ560BPPGPROSLIC_NO_EC					= 0xC11C, // TigerJet 560B USB PPG + Proslic without Echo Canceller
	TJIP_TJ560CPPG_NO_EC						= 0xC211, // TigerJet 560C USB PPG without Echo Canceller

	TJIP_TJ560CCOMBO							= 0xC712, // TigerJet 560C USB Combo (Phone/Switch + PPG)

	TJIP_TJ560BDECT_SPI							= 0xF212, // TigerJet 560B USB DECT/SPI interface

}TjIpProductID;

typedef struct _VENDOR_CMD
{
    BYTE    bRequest;		// Vendor request
    WORD    wValue;			// Not used
    WORD    wIndex;			// Reg index
    WORD    wLength;		// Data length
    BYTE    direction;		// 0: write, 1: read
    BYTE    bData;			// Not used
} VENDOR_CMD, *PVENDOR_CMD;

typedef struct _TJ_SEND_VENDOR_CMD
{
    VENDOR_CMD		vcCmd;		// vendor command		 
    DWORD			dwDataSize;	// size of data buffer
    PVOID			pDataBuf;	// data buffer
} TJ_SEND_VENDOR_CMD, *PTJ_SEND_VENDOR_CMD;

typedef struct _TJ_ECC_OPTIONS
{
	BOOL bEnableEEC;	// TRUE to enable EEC
	int BufSize;		// Size of Buffer and number of bytes processed 
						// The source buffer is stored in an array with 16 entries.
						// Each entry has size of BufSize (80 ~ 240). 
	int RefIndex;		// Index of the array for the source location (0 to 15)
	int RefOffset;		// Offset of the starting point inside the entry (0 to BufferSize)
}TJ_ECC_OPTIONS, *PTJ_ECC_OPTIONS;

typedef struct _TJ_MIXER_OPEN
{
	char szMixerName[256];		// Mixer name to open (could be a partial name)
	HWND hCallbackWnd;			// A callback window to receive the MM_MIXM_CONTROL_CHANGE message
}TJ_MIXER_OPEN;

typedef struct _AUDIO_CAPTURE_INFO			// Audio capture info used with TJIP_START_AUDIO_CAPTURE command
{
	char				devName[64];		// TJ USB audio device name
	TJ_CALLBACK_INFO	tjCallback;			// Callback info
}AUDIO_CAPTURE_INFO, *PAUDIO_CAPTURE_INFO;

__declspec(dllexport) BOOL TjIpSysCall(int id, int param, void * ex);

#ifdef __cplusplus
  }
#endif
#endif  // end of TJIPSYS_H
