# Note

This is now in progress updated code, to move to Omega release (80% fixed for older devices)

it will crash, need more check, I guess because of the ffmpeg stuff (now it's 6 instead of 5, not sure if that compatible with the older builds)

for proper build use Nexus, I didn't implement any of the Nexus changes into Omega yet.

# Build

Kodi has specific way to build,

to learn how to make this working for older devices follow these steps:

## Basic
- The current compatible versions are Matrix, Nexus (Omega still in test)
- Use Kodi's official build instructions [Click here](https://github.com/xbmc/xbmc/blob/master/docs/README.Windows.md)
- Hunt everything related to HDMI and XBOX and disable it
- Ensure there are no `FromApp` APIs used those don't work on any build below 16299
- You can skip build errors but keeping the target at 19041 and min build at 14393
- Using any older build than 19041 will give you a hell of build errors

## Specific (Audio legacy bug)
- This took me a lot of time to catch without debug
- At `AESinkXAudio.cpp` ensure `Microsoft::WRL::ComPtr<IXAudio2> xaudio2;` replaced by `IXAudio2* xaudio2 = nullptr;`
- It was causing direct crash after the app will start for 2 sec

## Addons
- Many addons that exists in the official ARM `msix` package can load without any problem


With those steps you will be able to get it compiled for and running for older builds.

____


# Nexus

- You need to use the build project as-is
- if you have different build tools than mine will give you a lot of errors
- Be careful, any change on CMake files will get some imports out
- If you can fix it then do it, otherwise don't change CMake files
- This was a test build and some changes were made quickly

## Requirements

- SDKs: 18362 and 14393
- The ffmpeg btw don't support 14393 
- but we're selecting the lowest possible for future tests
- Visual Studio 2022 (build tools v143)
- You must have `VC/Tools/MSVC/14.41.33901`
- This build was made using msvc `14.41.33901`
- Test made only on `Release` (ARM)
- Some projects may fail on first build, just try one more time
- Once you start getting weird errors for CMake build..ARM..etc means you don't have the exact msvc I mentioned

## Note

This was provided as is for users want to build on their side

it's not a proper way to build, always refer to the method above, 

but I don't and will never have time to make it in proper way

You can sync the updates to your own build, or try to wrap your head with it.

## Steps

- Put `kodi` and `kodi-build-wp` in `C:\`
- Any change you want to make it should be done in `C:\kodi`
- Kodi main code is at `C:\kodi\xbmc`
- Don't use or copy files to `C:\kodi-build-wp` this will get reset each build
- Open `C:\kodi-build-wp\kodi.sln`
- If requirements above full-filled, you can build directly
- otherwise you missed something

## Misc

- If you rebuild, project `python_binding` at (Build Utilities) may not build again
- To solve that there is folder called `swig` copy the files from it to `C:\kodi-build-wp\build\swig`
- Try to build again

## More 

This was based on `20230118-5715cb0ca6-dirty`, nothing was synced since commit `5715cb0ca6`.

use compare tools such as 


## Advice

I strongly prefer to do this from start and not get into this ready-build

You can spend sometime to sync the changes from my build to yours and link them properly, it's better