# Note

This is now in progress updated code, to move to Omega release (80% fixed for older devices)

it will crash, need more check, I guess because of the ffmpeg stuff (now it's 6 instead of 5, not sure if that compatible with the older builds)


# Build

Kodi has specific way to build,

if you have different build tools than mine will give you a lot of errors

to learn how to make this working for older devices follow these steps:

## Basic
- The current compatible versions are Matrix, Nexus (Omega still in test)
- Use Kode official build instructions [Click here](https://github.com/xbmc/xbmc/blob/master/docs/README.Windows.md)
- Hunt everything related to HDMI and XBOX and disable it
- Ensure there are no `FromApp` APIs used those don't work on any build below 16299
- You can skip build errors but keeping the target at 19041 and min build at 14393
- Using any older build than 19041 will give you a hell or build errors

## Specific (Audio legacy bug)
- This took me a lot of time to catch without debug
- At `AESinkXAudio.cpp` ensure `Microsoft::WRL::ComPtr<IXAudio2> xaudio2;` replaced by `IXAudio2* xaudio2 = nullptr;`
- It was causing direct crash after the app will start for 2 sec

## Addons
- Many addons that exists in the official ARM `msix` package can load without any problem


With those steps you will be able to get it compiled for and running for older builds.