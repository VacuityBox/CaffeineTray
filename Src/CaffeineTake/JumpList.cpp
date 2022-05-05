// The MIT License (MIT)
// 
// Copyright (c) Microsoft Corporation
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.
// 
// Portions of this repo are provided under the SIL Open Font License.
// See the LICENSE file in individual samples for additional details.

#include "JumpList.hpp"

#define NTDDI_VERSION NTDDI_WIN7  // Specifies that the minimum required platform is Windows 7.
#define WIN32_LEAN_AND_MEAN       // Exclude rarely-used stuff from Windows headers
#define STRICT_TYPED_ITEMIDS      // Utilize strictly typed IDLists

// Windows Header Files:
#include <windows.h>
#include <psapi.h>
#include <shlwapi.h>

// Header Files for Jump List features
#include <objectarray.h>
#include <shobjidl.h>
#include <propkey.h>
#include <propvarutil.h>
#include <knownfolders.h>
#include <shlobj.h>

// Creates a CLSID_ShellLink to insert into the Tasks section of the Jump List.  This type of Jump
// List item allows the specification of an explicit command line to execute the task.
HRESULT CreateShellLink(PCWSTR pszTargetExecutable, PCWSTR pszArguments, PCWSTR pszTitle, PCWSTR pszIconPath, int iIcon, IShellLink **ppsl)
{
    IShellLink *psl;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&psl));
    if (SUCCEEDED(hr))
    {
        hr = psl->SetPath(pszTargetExecutable);
        if (SUCCEEDED(hr))
        {
            hr = psl->SetArguments(pszArguments);
            if (SUCCEEDED(hr))
            {
                hr = psl->SetIconLocation(pszIconPath, iIcon);
                if (SUCCEEDED(hr))
                {
                    // The title property is required on Jump List items provided as an IShellLink
                    // instance.  This value is used as the display name in the Jump List.
                    IPropertyStore *pps;
                    hr = psl->QueryInterface(IID_PPV_ARGS(&pps));
                    if (SUCCEEDED(hr))
                    {
                        PROPVARIANT propvar;
                        hr = InitPropVariantFromString(pszTitle, &propvar);
                        if (SUCCEEDED(hr))
                        {
                            hr = pps->SetValue(PKEY_Title, propvar);
                            if (SUCCEEDED(hr))
                            {
                                hr = pps->Commit();
                                if (SUCCEEDED(hr))
                                {
                                    hr = psl->QueryInterface(IID_PPV_ARGS(ppsl));
                                }
                            }
                            PropVariantClear(&propvar);
                        }
                        pps->Release();
                    }
                }
            }
        }
        psl->Release();
    }
    return hr;
}

// The Tasks category of Jump Lists supports separator items.  These are simply IShellLink instances
// that have the PKEY_AppUserModel_IsDestListSeparator property set to TRUE.  All other values are
// ignored when this property is set.
HRESULT CreateSeparatorLink(IShellLink **ppsl)
{
    IPropertyStore *pps;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pps));
    if (SUCCEEDED(hr))
    {
        PROPVARIANT propvar;
        hr = InitPropVariantFromBoolean(TRUE, &propvar);
        if (SUCCEEDED(hr))
        {
            hr = pps->SetValue(PKEY_AppUserModel_IsDestListSeparator, propvar);
            if (SUCCEEDED(hr))
            {
                hr = pps->Commit();
                if (SUCCEEDED(hr))
                {
                    hr = pps->QueryInterface(IID_PPV_ARGS(ppsl));
                }
            }
            PropVariantClear(&propvar);
        }
        pps->Release();
    }
    return hr;
}

namespace CaffeineTake {

auto JumpList::Update (const fs::path& target, const std::vector<JumpListEntry>& jumpList) -> bool
{
    const auto targetPath = target.wstring();

    // Create the custom Jump List object.
    ICustomDestinationList *pcdl;
    HRESULT hr = CoCreateInstance(CLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pcdl));
    if (SUCCEEDED(hr))
    {
        UINT cMinSlots;
        IObjectArray *poaRemoved;
        hr = pcdl->BeginList(&cMinSlots, IID_PPV_ARGS(&poaRemoved));
        if (SUCCEEDED(hr))
        {
            IObjectCollection *poc;
            HRESULT hr = CoCreateInstance(CLSID_EnumerableObjectCollection, NULL, CLSCTX_INPROC, IID_PPV_ARGS(&poc));
            if (SUCCEEDED(hr))
            {
                for (const auto& task : jumpList)
                {
                    IShellLink * psl = nullptr;
                    switch (task.Type)
                    {
                    case JumpListEntry::Type::Normal:
                        hr = CreateShellLink(targetPath.c_str(), task.CmdArguments.data(), task.TaskName.data(), task.IconPath.data(), task.IconId, &psl);
                        break;

                    case JumpListEntry::Type::Separator:
                        hr = CreateSeparatorLink(&psl);
                        break;
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = poc->AddObject(psl);
                        psl->Release();
                    }
                }

                if (SUCCEEDED(hr))
                {
                    IObjectArray * poa;
                    hr = poc->QueryInterface(IID_PPV_ARGS(&poa));
                    if (SUCCEEDED(hr))
                    {
                        // Add the tasks to the Jump List. Tasks always appear in the canonical "Tasks"
                        // category that is displayed at the bottom of the Jump List, after all other
                        // categories.
                        hr = pcdl->AddUserTasks(poa);
                        poa->Release();
                    }
                }
                poc->Release();
            }

            if (SUCCEEDED(hr))
            {
                // Commit the list-building transaction.
                hr = pcdl->CommitList();
            }
        }
        pcdl->Release();
    }

    return SUCCEEDED(hr);
}

auto JumpList::Clear () -> bool
{
    ICustomDestinationList *pcdl;
    HRESULT hr = CoCreateInstance(CLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pcdl));
    if (SUCCEEDED(hr))
    {
        hr = pcdl->DeleteList(NULL);
        pcdl->Release();
    }

    return SUCCEEDED(hr); 
}

} // namespace CaffeineTake
