#include "audiomanagerwindows.h"
#include <exception>
#include "../../logging.h"
#include "../AudioTabController.h"

#include <locale>
#include <codecvt>


// application namespace
namespace advsettings {

AudioManagerWindows::~AudioManagerWindows() {
	audioDeviceEnumerator->UnregisterEndpointNotificationCallback((IMMNotificationClient*)this);
	if (mirrorAudioEndpointVolume) {
		mirrorAudioEndpointVolume->Release();
	}
	if (micAudioEndpointVolume) {
		micAudioEndpointVolume->Release();
	}
	if (mirrorAudioDevice) {
		mirrorAudioDevice->Release();
	}
	if (micAudioDevice) {
		micAudioDevice->Release();
	}
	if (playbackAudioDevice) {
		playbackAudioDevice->Release();
	}
	audioDeviceEnumerator->Release();
}

void AudioManagerWindows::init(AudioTabController* controller) {
	audioDeviceEnumerator = getAudioDeviceEnumerator();
	if (!audioDeviceEnumerator) {
		throw std::exception("Could not create audio device enumerator");
	}
	playbackAudioDevice = getDefaultPlaybackDevice(audioDeviceEnumerator);
	if (!playbackAudioDevice) {
		LOG(WARNING) << "Could not find a default recording device.";
	}
	micAudioDevice = getDefaultRecordingDevice(audioDeviceEnumerator);
	if (micAudioDevice) {
		micAudioEndpointVolume = getAudioEndpointVolume(micAudioDevice);
	} else {
		LOG(WARNING) << "Could not find a default recording device.";
	}
	this->controller = controller;
	audioDeviceEnumerator->RegisterEndpointNotificationCallback((IMMNotificationClient*)this);
	policyConfig = getPolicyConfig();
	if (!policyConfig) {
		LOG(ERROR) << "Could not find PolicyConfig interface";
	}
}

void AudioManagerWindows::setPlaybackDevice(const std::string & id, bool notify) {
	if (!id.empty()) {
		auto dev = getDevice(audioDeviceEnumerator, id);
		if (!dev) {
			LOG(WARNING) << "Could not find playback device \"" << id << "\".";
		} else {
			playbackAudioDevice = dev;
			if (policyConfig) {
				LPWSTR devId;
				if (dev->GetId(&devId) >= 0) {
					policyConfig->SetDefaultEndpoint(devId, eConsole);
				}
			}
		}
	}
	if (notify) {
		controller->onNewPlaybackDevice();
	}
}

std::string AudioManagerWindows::getPlaybackDevName() {
	if (playbackAudioDevice) {
		return getDeviceName(playbackAudioDevice);
	}
	return "";
}

std::string AudioManagerWindows::getPlaybackDevId() {
	if (playbackAudioDevice) {
		return getDeviceId(playbackAudioDevice);
	}
	return "";
}


void AudioManagerWindows::setMirrorDevice(const std::string& id, bool notify) {
	if (id.empty()) {
		deleteMirrorDevice();
	} else {
		auto dev = getDevice(audioDeviceEnumerator, id);
		if (dev) {
			mirrorAudioDevice = dev;
			mirrorAudioEndpointVolume = getAudioEndpointVolume(mirrorAudioDevice);
		} else {
			LOG(WARNING) << "Could not find mirror device \"" << id << "\".";
		}
	}
	if (notify) {
		controller->onNewMirrorDevice();
	}
}

void AudioManagerWindows::deleteMirrorDevice() {
	if (mirrorAudioEndpointVolume) {
		mirrorAudioEndpointVolume->Release();
		mirrorAudioEndpointVolume = nullptr;
	}
	if (mirrorAudioDevice) {
		mirrorAudioDevice->Release();
		mirrorAudioDevice = nullptr;
	}
}

bool AudioManagerWindows::isMirrorValid() {
	return mirrorAudioEndpointVolume != nullptr;
}

std::string AudioManagerWindows::getMirrorDevName() {
	if (mirrorAudioDevice) {
		return getDeviceName(mirrorAudioDevice);
	}
	return "";
}

std::string AudioManagerWindows::getMirrorDevId() {
	if (mirrorAudioDevice) {
		return getDeviceId(mirrorAudioDevice);
	}
	return "";
}

float AudioManagerWindows::getMirrorVolume() {
	float value;
	if (mirrorAudioEndpointVolume && mirrorAudioEndpointVolume->GetMasterVolumeLevelScalar(&value) >= 0) {
		return value;
	} else {
		return 0.0;
	}
}

bool AudioManagerWindows::setMirrorVolume(float value) {
	if (mirrorAudioEndpointVolume) {
		if (mirrorAudioEndpointVolume->SetMasterVolumeLevelScalar(value, nullptr) >= 0) {
			return true;
		}
	}
	return false;
}

bool AudioManagerWindows::getMirrorMuted() {
	BOOL value;
	if (mirrorAudioEndpointVolume && mirrorAudioEndpointVolume->GetMute(&value) >= 0) {
		return value;
	} else {
		return false;
	}
}

bool AudioManagerWindows::setMirrorMuted(bool value) {
	if (mirrorAudioEndpointVolume) {
		if (mirrorAudioEndpointVolume->SetMute(value, nullptr) >= 0) {
			return true;
		}
	}
	return false;
}


bool AudioManagerWindows::isMicValid() {
	return micAudioEndpointVolume != nullptr;
}

void AudioManagerWindows::setMicDevice(const std::string & id, bool notify) {
	if (!id.empty()) {
		auto dev = getDevice(audioDeviceEnumerator, id);
		if (!dev) {
			LOG(WARNING) << "Could not find recording device \"" << id << "\".";
		} else {
			micAudioDevice = dev;
			if (policyConfig) {
				LPWSTR devId;
				if (dev->GetId(&devId) >= 0) {
					policyConfig->SetDefaultEndpoint(devId, eConsole);
				}
			}
		}
	}
	if (notify) {
		controller->onNewRecordingDevice();
	}
}

std::string AudioManagerWindows::getMicDevName() {
	if (micAudioDevice) {
		return getDeviceName(micAudioDevice);
	}
	return "";
}

std::string AudioManagerWindows::getMicDevId() {
	if (micAudioDevice) {
		return getDeviceId(micAudioDevice);
	}
	return "";
}

float AudioManagerWindows::getMicVolume() {
	float value;
	if (micAudioEndpointVolume && micAudioEndpointVolume->GetMasterVolumeLevelScalar(&value) >= 0) {
		return value;
	} else {
		return 0.0;
	}
}

bool AudioManagerWindows::setMicVolume(float value) {
	if (micAudioEndpointVolume) {
		if (micAudioEndpointVolume->SetMasterVolumeLevelScalar(value, nullptr) >= 0) {
			return true;
		}
	}
	return false;
}

bool AudioManagerWindows::getMicMuted() {
	BOOL value;
	if (micAudioEndpointVolume && micAudioEndpointVolume->GetMute(&value) >= 0) {
		return value;
	} else {
		return false;
	}
}

bool AudioManagerWindows::setMicMuted(bool value) {
	if (micAudioEndpointVolume) {
		if (micAudioEndpointVolume->SetMute(value, nullptr) >= 0) {
			return true;
		}
	}
	return false;
}

std::vector<std::pair<std::string, std::string>> AudioManagerWindows::getRecordingDevices() {
	return getDevices(audioDeviceEnumerator, eCapture);
}

std::vector<std::pair<std::string, std::string>> AudioManagerWindows::getPlaybackDevices() {
	return getDevices(audioDeviceEnumerator, eRender);
}


IMMDeviceEnumerator* AudioManagerWindows::getAudioDeviceEnumerator() {
	IMMDeviceEnumerator* pEnumerator;
	if (CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator) < 0) {
		return nullptr;
	}
	return pEnumerator;
}

IPolicyConfig * AudioManagerWindows::getPolicyConfig() {
	IPolicyConfig* policyConfig = nullptr;
	// for Win 10
	auto hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_INPROC, IID_IPolicyConfig2, (LPVOID *)&policyConfig);
	if (hr != S_OK) {
		hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_INPROC, IID_IPolicyConfig1, (LPVOID *)&policyConfig);
	}
	// for Win Vista, 7, 8, 8.1
	if (hr != S_OK) {
		hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_INPROC, IID_IPolicyConfig0, (LPVOID *)&policyConfig);
	}
	if (hr != S_OK) {
		return nullptr;
	} else {
		return policyConfig;
	}
}

IMMDevice* AudioManagerWindows::getDefaultRecordingDevice(IMMDeviceEnumerator* deviceEnumerator) {
	IMMDevice* pDevice;
	if (deviceEnumerator->GetDefaultAudioEndpoint(eCapture, eCommunications, &pDevice) < 0) {
		return nullptr;
	}
	return pDevice;
}

IMMDevice * AudioManagerWindows::getDefaultPlaybackDevice(IMMDeviceEnumerator * deviceEnumerator) {
	IMMDevice* pDevice;
	if (deviceEnumerator->GetDefaultAudioEndpoint(eRender, eCommunications, &pDevice) < 0) {
		return nullptr;
	}
	return pDevice;
}

IMMDevice* AudioManagerWindows::getDevice(IMMDeviceEnumerator* deviceEnumerator, const std::string& id) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wid = converter.from_bytes(id);
	return getDevice(deviceEnumerator, wid.c_str());
}

IMMDevice * AudioManagerWindows::getDevice(IMMDeviceEnumerator * deviceEnumerator, LPCWSTR id) {
	IMMDevice* pDevice;
	if (deviceEnumerator->GetDevice(id, &pDevice) < 0) {
		return nullptr;
	}
	return pDevice;
}

IAudioEndpointVolume * AudioManagerWindows::getAudioEndpointVolume(IMMDevice* device) {
	IAudioEndpointVolume * pEndpointVolume;
	if (device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (void**)&pEndpointVolume) < 0) {
		return nullptr;
	}
	return pEndpointVolume;
}

std::string AudioManagerWindows::getDeviceName(IMMDevice* device) {
	IPropertyStore *pProps = nullptr;
	PROPVARIANT varString;
	if (device->OpenPropertyStore(STGM_READ, &pProps) >= 0 && pProps->GetValue(PKEY_Device_FriendlyName, &varString) >= 0) {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::string name = converter.to_bytes(varString.pwszVal);
		return name;
	}
	return "";
}

std::string AudioManagerWindows::getDeviceId(IMMDevice* device) {
	LPWSTR ppstrId;
	if (device->GetId(&ppstrId) >= 0) {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		return converter.to_bytes(ppstrId);
	}
	return "";
}

std::vector<std::pair<std::string, std::string>> AudioManagerWindows::getDevices(IMMDeviceEnumerator * deviceEnumerator, EDataFlow dataFlow) {
	std::vector<std::pair<std::string, std::string>> retval;
	IMMDeviceCollection *ppDevices = nullptr;
	if (deviceEnumerator->EnumAudioEndpoints(dataFlow, DEVICE_STATE_ACTIVE, &ppDevices) >= 0) {
		UINT count;
		ppDevices->GetCount(&count);
		for (UINT i = 0; i < count; ++i) {
			IMMDevice *device = nullptr;
			ppDevices->Item(i, &device);
			if (device) {
				retval.emplace_back(getDeviceId(device), getDeviceName(device));
			}
		}
		ppDevices->Release();
	}
	return retval;
}


HRESULT AudioManagerWindows::QueryInterface(REFIID riid, void ** ppvObject) {
	if (IID_IUnknown == riid) {
		AddRef();
		*ppvObject = (IUnknown*)this;
	} else if (__uuidof(IMMNotificationClient) == riid) {
		AddRef();
		*ppvObject = (IMMNotificationClient*)this;
	} else {
		*ppvObject = NULL;
		return E_NOINTERFACE;
	}
	return S_OK;
}

ULONG AudioManagerWindows::AddRef(void) {
	return 1; // We don't need reference counting
}

ULONG AudioManagerWindows::Release(void) {
	return 1; // We don't need reference counting
}

HRESULT AudioManagerWindows::OnDeviceStateChanged(LPCWSTR, DWORD) {
	return S_OK;
}

HRESULT AudioManagerWindows::OnDeviceAdded(LPCWSTR) {
	controller->onDeviceAdded();
	return S_OK;
}

HRESULT AudioManagerWindows::OnDeviceRemoved(LPCWSTR) {
	controller->onDeviceRemoved();
	return S_OK;
}

HRESULT AudioManagerWindows::OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId) {
	if (flow == eCapture && role == eMultimedia) {
		if (audioDeviceEnumerator) {
			auto device = getDevice(audioDeviceEnumerator, pwstrDefaultDeviceId);
			if (device != micAudioDevice) {
				if (micAudioDevice) {
					micAudioDevice->Release();
				}
				micAudioDevice = device;
				if (micAudioDevice) {
					micAudioEndpointVolume = getAudioEndpointVolume(micAudioDevice);
				} else {
					std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
					std::string name = converter.to_bytes(pwstrDefaultDeviceId);
					LOG(WARNING) << "Could not find recording device \"" << name << "\".";
				}
				controller->onNewRecordingDevice();
			}
		}
	} else if (flow == eRender && role == eMultimedia) {
		if (audioDeviceEnumerator) {
			auto device = getDevice(audioDeviceEnumerator, pwstrDefaultDeviceId);
			if (device != playbackAudioDevice) {
				if (playbackAudioDevice) {
					playbackAudioDevice->Release();
				}
				playbackAudioDevice = device;
				if (!playbackAudioDevice) {
					std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
					std::string name = converter.to_bytes(pwstrDefaultDeviceId);
					LOG(WARNING) << "Could not find playback device \"" << name << "\".";
				}
				controller->onNewPlaybackDevice();
			}
		}
	}
	/*char* pszFlow = (char*)"?????";
	char* pszRole = (char*)"?????";

	switch (flow) {
	case eRender:
		pszFlow = (char*)"eRender";
		break;
	case eCapture:
		pszFlow = (char*)"eCapture";
		break;
	}

	switch (role) {
	case eConsole:
		pszRole = (char*)"eConsole";
		break;
	case eMultimedia:
		pszRole = (char*)"eMultimedia";
		break;
	case eCommunications:
		pszRole = (char*)"eCommunications";
		break;
	}

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::string name = converter.to_bytes(pwstrDefaultDeviceId);
	printf(" %s -->New default device: flow = %s, role = %s\n", name.c_str(), pszFlow, pszRole);
	fflush(stdout);*/
	return S_OK;
}

HRESULT AudioManagerWindows::OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) {
	return S_OK;
}

}
