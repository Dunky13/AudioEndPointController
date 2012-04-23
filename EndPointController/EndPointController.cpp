// EndPointController.cpp : Defines the entry point for the console application.
//
#include <stdio.h>
#include <wchar.h>
#include <tchar.h>
#include "windows.h"
#include "Mmdeviceapi.h"
#include "PolicyConfig.h"
#include "Propidl.h"
#include "Functiondiscoverykeys_devpkey.h"

// Format string for outputing a device entry. The following parameters will be used in the following order:
// Index, Device Friendly Name
#define DEVICE_OUTPUT_FORMAT "Audio Device %d: %ws"

typedef struct TGlobalState
{
	HRESULT hr;
	int option;
	IMMDeviceEnumerator *pEnum;
	IMMDeviceCollection *pDevices;
	IMMDevice *pCurrentDevice;
	TCHAR* pDeviceFormatStr;
} TGlobalState;

void createDeviceEnumerator(TGlobalState* state);
void prepareDeviceEnumerator(TGlobalState* state);
void enumerateOutputDevices(TGlobalState* state);
HRESULT printDeviceInfo(IMMDevice* pDevice, int index);
HRESULT SetDefaultAudioPlaybackDevice(LPCWSTR devID);

// EndPointController.exe [NewDefaultDeviceID]
int _tmain(int argc, _TCHAR* argv[])
{
	TGlobalState state;

	// Process command line arguments
	state.option = 0; // 0 indicates list devices.
	if (argc > 1) 
	{
		if (_tcscmp(argv[1], _T("--help")) == 0)
		{
			printf("Lists audio end-point devices or sets default audio end-point device.\n\n");
			printf("USAGE\n");
			printf("  EndPointController.exe [-f format_str]  Lists audio end-point devices that\n");
			printf("                                          are enabled.\n");
			printf("  EndPointController.exe device_index     Sets the default device with the\n");
			printf("                                          given index.\n");
			printf("\n");
			printf("OPTIONS\n");
			printf("  -f format_str  Outputs the details of each device using the given format\n");
			printf("                 string. If this parameter is ommitted the format string\n");
			printf("                 defaults to: \"%s\"\n\n", DEVICE_OUTPUT_FORMAT);
			printf("                 Parameters that are passed to the 'printf' function are\n");
			printf("                 ordered as follows:\n");
			printf("                   - Device index (int)\n");
			printf("                   - Device friendly name (string)\n");
			exit(0);
		}
	}
	
	if (argc == 2) state.option = _ttoi(argv[1]);

	state.hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (SUCCEEDED(state.hr))
	{
		createDeviceEnumerator(&state);
	}
	return state.hr;
}

// Create a multimedia device enumerator.
void createDeviceEnumerator(TGlobalState* state)
{
	state->pEnum = NULL;
	state->hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
		(void**)&state->pEnum);
	if (SUCCEEDED(state->hr))
	{
		prepareDeviceEnumerator(state);
	}
}

// Prepare the device enumerator
void prepareDeviceEnumerator(TGlobalState* state)
{
	state->hr = state->pEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &state->pDevices);
	if SUCCEEDED(state->hr)
	{
		enumerateOutputDevices(state);
	}
	state->pEnum->Release();
}

// Enumerate the output devices
void enumerateOutputDevices(TGlobalState* state)
{
	UINT count;
	state->pDevices->GetCount(&count);

	// If option is less than 1, list devices
	if (state->option < 1) 
	{

		for (int i = 1; i <= (int)count; i++)
		{
			state->hr = state->pDevices->Item(i - 1, &state->pCurrentDevice);
			if (SUCCEEDED(state->hr))
			{
				TCHAR* strID = NULL;
				state->hr = state->pCurrentDevice->GetId(&strID);
				if (SUCCEEDED(state->hr))
				{
					state->hr = printDeviceInfo(state->pCurrentDevice, i);
				}
				state->pCurrentDevice->Release();
			}
		}
	}
	// If option corresponds with the index of an audio device, set it to default
	else if (state->option <= (int)count)
	{
		state->hr = state->pDevices->Item(state->option - 1, &state->pCurrentDevice);
		if (SUCCEEDED(state->hr))
		{
			TCHAR* strID = NULL;
			state->hr = state->pCurrentDevice->GetId(&strID);
			if (SUCCEEDED(state->hr))
			{
				state->hr = SetDefaultAudioPlaybackDevice(strID);
			}
			state->pCurrentDevice->Release();
		}
	}
	// Otherwise inform user than option doesn't correspond with a device
	else
	{
		printf("Error: No audio end-point device with the index '%d%'\n", state->option);
	}
	
	state->pDevices->Release();
}

HRESULT printDeviceInfo(IMMDevice* pDevice, int index)
{
	IPropertyStore *pStore;
	HRESULT hr = pDevice->OpenPropertyStore(STGM_READ, &pStore);

	if (SUCCEEDED(hr))
	{
		PROPVARIANT friendlyName;
		PropVariantInit(&friendlyName);
		hr = pStore->GetValue(PKEY_Device_FriendlyName, &friendlyName);
		if (SUCCEEDED(hr))
		{
			printf(DEVICE_OUTPUT_FORMAT, index, friendlyName.pwszVal);
			printf("\n");
			PropVariantClear(&friendlyName);
		}
		pStore->Release();
	}
	return hr;
}

HRESULT SetDefaultAudioPlaybackDevice(LPCWSTR devID)
{	
	IPolicyConfigVista *pPolicyConfig;
	ERole reserved = eConsole;

    HRESULT hr = CoCreateInstance(__uuidof(CPolicyConfigVistaClient), 
		NULL, CLSCTX_ALL, __uuidof(IPolicyConfigVista), (LPVOID *)&pPolicyConfig);
	if (SUCCEEDED(hr))
	{
		hr = pPolicyConfig->SetDefaultEndpoint(devID, reserved);
		pPolicyConfig->Release();
	}
	return hr;
}
