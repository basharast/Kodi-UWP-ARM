// UWP STORAGE MANAGER
// Copyright (c) 2023 Bashar Astifan.
// Email: bashar@astifan.online
// Telegram: @basharastifan

// This code must keep support for lower builds (15063+)
// Try always to find possible way to keep that support

// Functions:
// ChooseFolder()
// ChooseFile(std::vector<std::string> exts)

#include "pch.h"

#include "StorageAccess.h"
#include "StorageAsync.h"
#include "StorageConfig.h"
#include "StorageExtensions.h"
#include "StorageLog.h"

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Windows.Storage.h>
#include <windows/GUIWindowFileManager.h>

using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Foundation;

extern void AddItemToFutureList(IStorageItem folder);
extern void AddItemToFutureList(IStorageItem item, winrt::hstring key);
HWND hwnd;

// Call folder picker (the selected folder will be added to future list)
winrt::hstring PickSingleFolder()
{
  auto folderPicker{Pickers::FolderPicker()};
  folderPicker.SuggestedStartLocation(Pickers::PickerLocationId::Desktop);
  folderPicker.FileTypeFilter().Append(L"*");

  auto folder = WaitTask(folderPicker.PickSingleFolderAsync());
  winrt::hstring path = {};
  if (folder != nullptr)
  {
    AddItemToFutureList(folder);
    path = folder.Path();
  }
  return path;
}

struct __declspec(uuid("45D64A29-A63E-4CB6-B498-5781D298CB4F")) __declspec(novtable)
    ICoreWindowInterop : public ::IUnknown
{
  virtual HRESULT STDMETHODCALLTYPE get_WindowHandle(HWND* hwnd) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_MessageHandled(unsigned char value) = 0;
};

typedef interface IInitializeWithWindow IInitializeWithWindow;
#ifdef __cplusplus
extern "C"
{
#endif
EXTERN_C const IID IID_IInitializeWithWindow;
MIDL_INTERFACE("3E68D4BD-7135-4D10-8018-9FB6D9F33FA1")
IInitializeWithWindow : public ::IUnknown
{
      public:
        virtual HRESULT STDMETHODCALLTYPE Initialize(
            /* [in] */ __RPC__in HWND hwnd) = 0;
};
#ifdef __cplusplus
}
#endif
bool isPickerInProgress = false;
extern void FinalizeSharedFolder(StorageFolder folder, CGUIWindowFileManager* instance, int iList);
void PickPublicSharedFolder(CGUIWindowFileManager* instance, int iList)
{
  CLog::LogF(LOGDEBUG, "Picker called");
  if (isPickerInProgress)
  {
    CLog::LogF(LOGDEBUG, "Picker skipped, duplicated call");
    return;
  }
  isPickerInProgress = true;
  auto folderPicker{Pickers::FolderPicker()};
  
  /*auto coreWindow = CoreWindow::GetForCurrentThread();
  winrt::com_ptr<ICoreWindowInterop> interop;
  winrt::check_hresult(winrt::get_unknown(coreWindow)->QueryInterface(interop.put()));
  winrt::check_hresult(interop->get_WindowHandle(&hwnd));
  folderPicker.as<::IInitializeWithWindow>() ->Initialize(hwnd);*/

  folderPicker.SuggestedStartLocation(Pickers::PickerLocationId::Downloads);
  folderPicker.ViewMode(winrt::Windows::Storage::Pickers::PickerViewMode::List);
  folderPicker.FileTypeFilter().Append(L"*");

  CLog::LogF(LOGDEBUG, "Picker initial");
  StorageFolder folder = WaitTask(folderPicker.PickSingleFolderAsync());
  CLog::LogF(LOGDEBUG, "Picker done");

  AddItemToFutureList(folder, L"public_shared");
  isPickerInProgress = false;
  FinalizeSharedFolder(folder, instance, iList);
  return;
}

// Call file picker (the selected file will be added to future list)
winrt::hstring PickSingleFile(std::vector<std::string> exts)
{
  auto filePicker{Pickers::FileOpenPicker()};
  filePicker.SuggestedStartLocation(Pickers::PickerLocationId::Desktop);
  filePicker.ViewMode(Pickers::PickerViewMode::List);

  if (exts.size() > 0)
  {
    for (auto ext : exts)
    {
      filePicker.FileTypeFilter().Append(convert(ext));
    }
  }
  else
  {
    filePicker.FileTypeFilter().Append(L"*");
  }
  auto file = WaitTask(filePicker.PickSingleFileAsync());

  winrt::hstring path = {};
  if (file != nullptr)
  {
    AddItemToFutureList(file);
    path = file.Path();
  }
  return path;
}

std::string ChooseFile(std::vector<std::string> exts)
{
  return convert(PickSingleFile(exts));
}

std::string ChooseFolder()
{
  return convert(PickSingleFolder());
}
