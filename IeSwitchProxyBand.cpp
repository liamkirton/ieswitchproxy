////////////////////////////////////////////////////////////////////////////////////////////////////
// IeSwitchProxy
//
// Copyright ©2007-2008 Liam Kirton <liam@int3.ws>
////////////////////////////////////////////////////////////////////////////////////////////////////
// IeSwitchProxyBand.cpp
//
// Created: 26/09/2007
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "IeSwitchProxyBand.h"

#include <shlwapi.h>
#include <strsafe.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <wininet.h>

#include "Resources.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

IeSwitchProxyBand::IeSwitchProxyBand()
{
	InterlockedIncrement(reinterpret_cast<volatile LONG *>(&g_DllRefCount));

	dwBandID_ = -1;
	dwObjRefCount_ = 1;

	site_ = NULL;

	hWndParent_ = NULL;
	hWnd_ = NULL;
	hWndToolbar_ = NULL;

	hTheme_ = NULL;
	hImageList_ = NULL;

	LoadSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IeSwitchProxyBand::~IeSwitchProxyBand()
{
	InterlockedDecrement(reinterpret_cast<volatile LONG *>(&g_DllRefCount));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IeSwitchProxyBand::QueryInterface(REFIID riid, LPVOID *ppReturn)
{
	*ppReturn = NULL;

	HRESULT hResult = E_NOINTERFACE;

	if(IsEqualIID(riid, IID_IUnknown))
	{
		*ppReturn = this;
	}
	else if (IsEqualIID(riid, IID_IOleWindow))
	{
		*ppReturn = dynamic_cast<IOleWindow *>(this);
	}
	else if (IsEqualIID(riid, IID_IDockingWindow))
	{
		*ppReturn = dynamic_cast<IDockingWindow *>(this);
	}
	else if (IsEqualIID(riid, IID_IDeskBand))
	{
		*ppReturn = dynamic_cast<IDeskBand *>(this);
	}
	else if (IsEqualIID(riid, IID_IInputObject))
	{
		*ppReturn = dynamic_cast<IInputObject *>(this);
	}
	else if (IsEqualIID(riid, IID_IObjectWithSite))
	{
		*ppReturn = dynamic_cast<IObjectWithSite *>(this);
	}
	else if (IsEqualIID(riid, IID_IPersist))
	{
		*ppReturn = dynamic_cast<IPersist *>(this);
	}
	else if (IsEqualIID(riid, IID_IPersistStream))
	{
		*ppReturn = dynamic_cast<IPersistStream *>(this);
	}

	if(*ppReturn != NULL)
	{
		AddRef();
		hResult = S_OK;
	}
	
	return hResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(DWORD) IeSwitchProxyBand::AddRef()
{
	return InterlockedIncrement(reinterpret_cast<volatile LONG *>(&dwObjRefCount_));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(DWORD) IeSwitchProxyBand::Release()
{
	if(InterlockedDecrement(reinterpret_cast<volatile LONG *>(&dwObjRefCount_)) == 0)
	{
		delete this;
		return 0;
	}
	return dwObjRefCount_;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IeSwitchProxyBand::ContextSensitiveHelp(BOOL fEnterMode)
{
	return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IeSwitchProxyBand::GetWindow(HWND *phwnd)
{
	*phwnd = hWnd_;
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IeSwitchProxyBand::CloseDW(DWORD dwReserved)
{
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IeSwitchProxyBand::ResizeBorderDW(LPCRECT prcBorder, IUnknown* punkToolbarSite, BOOL fReserved)
{
	return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IeSwitchProxyBand::ShowDW(BOOL bShow)
{
	if(bShow)
	{
		ShowWindow(hWnd_, SW_SHOW);
	}
	else
	{
		ShowWindow(hWnd_, SW_HIDE);
	}
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IeSwitchProxyBand::GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO* pdbi)
{
	HRESULT hResult = E_INVALIDARG;

	dwBandID_ = dwBandID;
	
	if(pdbi != NULL)
	{
		if(pdbi->dwMask & DBIM_TITLE)
		{
			StringCchCopy(pdbi->wszTitle, sizeof(pdbi->wszTitle) / sizeof(WCHAR), L"Proxy");
		}
		if(pdbi->dwMask & DBIM_MINSIZE)
		{
			pdbi->ptMinSize.x = 0;
			pdbi->ptMinSize.y = 22;
		}
		if(pdbi->dwMask & DBIM_MAXSIZE)
		{
			pdbi->ptMaxSize.x = -1;
			pdbi->ptMaxSize.y = 22;
		}
		if(pdbi->dwMask & DBIM_INTEGRAL)
		{
			pdbi->ptIntegral.x = 1;
			pdbi->ptIntegral.y = 1;
		}
		if(pdbi->dwMask & DBIM_ACTUAL)
		{
			pdbi->ptActual.x = 0;
			pdbi->ptActual.y = 0;
		}
		if(pdbi->dwMask & DBIM_MODEFLAGS)
		{
			pdbi->dwModeFlags = DBIMF_NORMAL;
		}
		if(pdbi->dwMask & DBIM_BKCOLOR)
		{
			pdbi->dwMask &= ~DBIM_BKCOLOR;
		}
		hResult = S_OK;
	}

	return hResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IeSwitchProxyBand::HasFocusIO()
{
	return S_FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IeSwitchProxyBand::TranslateAcceleratorIO(LPMSG lpMsg)
{
	return S_FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IeSwitchProxyBand::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
	if(hWnd_ != NULL)
	{
		SetFocus(hWnd_);
	}
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IeSwitchProxyBand::SetSite(IUnknown* pUnkSite)
{
	HRESULT hResult = S_OK;

	if(site_ != NULL)
	{
		site_->Release();
		site_ = NULL;
	}

	if(pUnkSite != NULL)
	{
		hWndParent_ = NULL;

		IOleWindow *pParentOleWindow;
		if(SUCCEEDED(pUnkSite->QueryInterface(IID_IOleWindow, reinterpret_cast<LPVOID *>(&pParentOleWindow))))
		{
			pParentOleWindow->GetWindow(&hWndParent_);
			pParentOleWindow->Release();
			pParentOleWindow = NULL;
		}

		if(!SUCCEEDED(pUnkSite->QueryInterface(IID_IInputObjectSite,
											   reinterpret_cast<LPVOID *>(&site_))))
		{
			hResult = E_FAIL;
		}

		hResult = CreateIeSwitchProxyWindow();
	}

	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IeSwitchProxyBand::GetSite(REFIID riid, void** ppvSite)
{
	HRESULT hResult = E_FAIL;

	*ppvSite = NULL;

	if(site_ != NULL)
	{
		hResult = site_->QueryInterface(riid, ppvSite);
	}

	return hResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IeSwitchProxyBand::GetClassID(CLSID *pClassID)
{
	*pClassID = CLSID_IeSwitchProxyBand;
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IeSwitchProxyBand::IsDirty()
{
	return S_FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IeSwitchProxyBand::Load(IStream *pStm)
{
	return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IeSwitchProxyBand::Save(IStream *pStm, BOOL fClearDirty)
{
	return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IeSwitchProxyBand::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
	return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void IeSwitchProxyBand::LoadSettings()
{
	proxySettings_.clear();

	HKEY hKey;
	DWORD dwDisp;

	if(RegCreateKeyEx(HKEY_CURRENT_USER,
					  L"Software\\int3.ws\\IeSwitchProxy",
					  0,
					  NULL,
					  REG_OPTION_NON_VOLATILE,
					  KEY_ALL_ACCESS,
					  NULL,
					  &hKey,
					  &dwDisp) != ERROR_SUCCESS)
	{
		return;
	}

	wchar_t *name = new wchar_t[1024];
	wchar_t *value = new wchar_t[1024];
	DWORD dwNameSize = 1024;
	DWORD dwValueSize = 1024;

	name[0] = L'\0';
	value[0] = L'\0';

	DWORD dwIndex = 0;
	while(RegEnumValue(hKey,
					  dwIndex++,
					  name,
					  &dwNameSize,
					  NULL,
					  NULL,
					  reinterpret_cast<LPBYTE>(value),
					  &dwValueSize) == ERROR_SUCCESS)
	{
		std::wstring proxyName = name;
		std::wstring proxyValue = value;
		std::wstring proxyExc = L"";

		size_t idDelim = proxyName.find(L"_");
		if(idDelim != std::string::npos)
		{
			proxyName = proxyName.substr(idDelim + 1);
		}

		size_t valDelim = proxyValue.find(L" :: ");
		if(valDelim != std::string::npos)
		{
			if(valDelim + 4 < proxyValue.length())
			{
				proxyExc = proxyValue.substr(valDelim + 4);
			}
			proxyValue = proxyValue.substr(0, valDelim);
		}

		ProxySetting newSetting;
		newSetting.Name = proxyName;
		newSetting.Value = proxyValue;
		newSetting.Exclusions = proxyExc;
		proxySettings_.push_back(newSetting);

		name[0] = L'\0';
		value[0] = L'\0';

		dwNameSize = 1024;
		dwValueSize = 1024;		
	}

	RegCloseKey(hKey);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void IeSwitchProxyBand::SaveSettings()
{
	HKEY hKey;
	DWORD dwDisp;

	if(RegCreateKeyEx(HKEY_CURRENT_USER,
					  L"Software\\int3.ws",
					  0,
					  NULL,
					  REG_OPTION_NON_VOLATILE,
					  KEY_ALL_ACCESS,
					  NULL,
					  &hKey,
					  &dwDisp) != ERROR_SUCCESS)
	{
		return;
	}

	SHDeleteKey(hKey, L"IeSwitchProxy");
	RegCloseKey(hKey);

	if(RegCreateKeyEx(HKEY_CURRENT_USER,
					  L"Software\\int3.ws\\IeSwitchProxy",
					  0,
					  NULL,
					  REG_OPTION_NON_VOLATILE,
					  KEY_ALL_ACCESS,
					  NULL,
					  &hKey,
					  &dwDisp) != ERROR_SUCCESS)
	{
		return;
	}

	DWORD dwId = 0;
	for(std::vector<ProxySetting>::iterator i = proxySettings_.begin(); i != proxySettings_.end(); ++i)
	{
		ProxySetting proxySetting = (*i);

		wchar_t cnvt[12];
		_itow_s(dwId++, cnvt, sizeof(cnvt) / sizeof(wchar_t), 10);

		std::wstring name = cnvt;
		name += L"_";
		name += proxySetting.Name;

		std::wstring value = proxySetting.Value + L" :: " + proxySetting.Exclusions;

		DWORD dwLength = static_cast<DWORD>(value.length()) * sizeof(wchar_t);
		RegSetValueEx(hKey,
					  name.c_str(),
					  0,
					  REG_SZ,
					  reinterpret_cast<const BYTE *>(value.c_str()),
					  dwLength);
	}

	RegCloseKey(hKey);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void IeSwitchProxyBand::GetProxy()
{
	INTERNET_PER_CONN_OPTION_LIST internetPerConnOptionList;
	INTERNET_PER_CONN_OPTION internetPerConnOptions[1];
	
	internetPerConnOptions[0].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
	internetPerConnOptions[0].Value.pszValue = NULL;

	internetPerConnOptionList.dwSize = sizeof(INTERNET_PER_CONN_OPTION_LIST);
	internetPerConnOptionList.pszConnection = NULL;
	internetPerConnOptionList.dwOptionCount = 1;
	internetPerConnOptionList.dwOptionError = 0;
	internetPerConnOptionList.pOptions = reinterpret_cast<LPINTERNET_PER_CONN_OPTION>(&internetPerConnOptions);
	
	DWORD dwBufferLength = sizeof(INTERNET_PER_CONN_OPTION_LIST);
	InternetQueryOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &internetPerConnOptionList, &dwBufferLength);

	for(std::vector<ProxySetting>::iterator i = proxySettings_.begin(); i != proxySettings_.end(); ++i)
	{
		ProxySetting proxySetting = (*i);
		if(proxySetting.Value == ((internetPerConnOptions[0].Value.pszValue != NULL) ? internetPerConnOptions[0].Value.pszValue : L""))
		{
			TBBUTTONINFO tbButtonInfo;
			SecureZeroMemory(&tbButtonInfo, sizeof(TBBUTTONINFO));
			tbButtonInfo.cbSize = sizeof(TBBUTTONINFO);
			tbButtonInfo.dwMask = TBIF_TEXT;
			tbButtonInfo.pszText = const_cast<LPWSTR>(proxySetting.Name.c_str());

			SendMessage(hWndToolbar_, TB_SETBUTTONINFO, 0, reinterpret_cast<LPARAM>(&tbButtonInfo));
			break;
		}
	}

	GlobalFree(internetPerConnOptions[0].Value.pszValue);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void IeSwitchProxyBand::SetProxy(ProxySetting &proxySetting)
{
	INTERNET_PER_CONN_OPTION_LIST internetPerConnOptionList;
	INTERNET_PER_CONN_OPTION internetPerConnOptions[3];
	
	internetPerConnOptions[0].dwOption = INTERNET_PER_CONN_FLAGS;
	internetPerConnOptions[0].Value.dwValue = 0;
	
	if(proxySetting.Value.length() > 0)
	{
		internetPerConnOptions[0].Value.dwValue |= PROXY_TYPE_PROXY;
	}
	if((proxySetting.Value.length() == 0) || (proxySetting.Exclusions.length() > 0))
	{
		internetPerConnOptions[0].Value.dwValue |= PROXY_TYPE_DIRECT;
	}

	internetPerConnOptions[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
	internetPerConnOptions[1].Value.pszValue = const_cast<LPWSTR>(proxySetting.Value.c_str());

	internetPerConnOptions[2].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;
	internetPerConnOptions[2].Value.pszValue = const_cast<LPWSTR>(proxySetting.Exclusions.c_str());

	internetPerConnOptionList.dwSize = sizeof(INTERNET_PER_CONN_OPTION_LIST);
	internetPerConnOptionList.pszConnection = NULL;
	internetPerConnOptionList.dwOptionCount = 3;
	internetPerConnOptionList.dwOptionError = 0;
	internetPerConnOptionList.pOptions = reinterpret_cast<LPINTERNET_PER_CONN_OPTION>(&internetPerConnOptions);
	
	InternetSetOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &internetPerConnOptionList, sizeof(INTERNET_PER_CONN_OPTION_LIST));
	InternetSetOption(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void IeSwitchProxyBand::UpdateBroadcast(UINT code)
{
	EnumWindows(s_EnumWindowsProc, static_cast<LRESULT>(code));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void IeSwitchProxyBand::CloseIeSwitchProxyWindow()
{
	if(hWndToolbar_ != NULL)
	{
		DestroyWindow(hWndToolbar_);
		hWndToolbar_ = NULL;
	}
	if(hWnd_ != NULL)
	{
		SetWindowLongPtr(hWnd_, GWLP_USERDATA, NULL);
		DestroyWindow(hWnd_);
		hWnd_ = NULL;
	}
	if(hTheme_ != NULL)
	{
		CloseThemeData(hTheme_);
		hTheme_ = NULL;
	}
	if(hImageList_ != NULL)
	{
		ImageList_Destroy(hImageList_);
		hImageList_ = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT IeSwitchProxyBand::CreateIeSwitchProxyWindow()
{
	HRESULT hResult = S_OK;

	WNDCLASS wndClass;
	SecureZeroMemory(&wndClass, sizeof(WNDCLASS));

	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hIcon = NULL;
	wndClass.hInstance = g_hInstance;
	wndClass.lpfnWndProc = IeSwitchProxyBand::s_IeSwitchProxyWindowProc;
	wndClass.lpszClassName = L"IeSwitchProxy";
	wndClass.lpszMenuName = NULL;
	wndClass.style = CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
	
	if(!RegisterClass(&wndClass))
	{
		hResult = E_FAIL;
	}

	RECT parentRect;
	GetClientRect(hWndParent_, &parentRect);

	if(!CreateWindowEx(0,
					   L"IeSwitchProxy",
					   NULL,
					   WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
					   parentRect.left,
					   parentRect.top,
					   parentRect.right - parentRect.left,
					   parentRect.bottom - parentRect.top,
					   hWndParent_,
					   NULL,
					   g_hInstance,
					   reinterpret_cast<void *>(this)))
	{
		hResult = E_FAIL;
	}

	hTheme_ = OpenThemeData(hWnd_, L"ReBar");

	if((hWndToolbar_ = CreateWindowEx(0,
									  TOOLBARCLASSNAME,
									  NULL,
									  WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
									  TBSTYLE_FLAT | TBSTYLE_LIST |
									  CCS_NODIVIDER | CCS_NORESIZE,
									  parentRect.left,
									  parentRect.top,
									  parentRect.right - parentRect.left,
									  parentRect.bottom - parentRect.top,
									  hWnd_,
									  0,
									  g_hInstance,
									  NULL)) != NULL)
	{
		SendMessage(hWndToolbar_, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
		ShowWindow(hWndToolbar_, SW_SHOWNORMAL);
	}
	else
	{
		hResult = E_FAIL;
	}

	hImageList_ = ImageList_Create(16, 16, ILC_COLOR24 | ILC_MASK, 2, 2);

	SecureZeroMemory(&tbButtons_, sizeof(tbButtons_));

	HBITMAP hBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(TBI_NETWORK));
	tbButtons_[0].iBitmap = ImageList_AddMasked(hImageList_, hBitmap, RGB(255, 0, 255));;
	tbButtons_[0].idCommand = 0;
	tbButtons_[0].fsState = TBSTATE_ENABLED;
	tbButtons_[0].fsStyle = BTNS_BUTTON | BTNS_WHOLEDROPDOWN | BTNS_AUTOSIZE;
	tbButtons_[0].dwData = 0;
	tbButtons_[0].iString = reinterpret_cast<INT_PTR>(L"???");

	hBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(TBI_SETTINGS));
	tbButtons_[1].iBitmap = ImageList_AddMasked(hImageList_, hBitmap, RGB(255, 0, 255));
	tbButtons_[1].idCommand = 1;
	tbButtons_[1].fsState = TBSTATE_ENABLED;
	tbButtons_[1].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
	tbButtons_[1].dwData = 0;
	tbButtons_[1].iString = reinterpret_cast<INT_PTR>(L"Settings");
	DeleteObject(hBitmap);

	hBitmap = LoadBitmap(g_hInstance, MAKEINTRESOURCE(TBI_HELP));
	tbButtons_[2].iBitmap = ImageList_AddMasked(hImageList_, hBitmap, RGB(255, 0, 255));
	tbButtons_[2].idCommand = 2;
	tbButtons_[2].fsState = TBSTATE_ENABLED;
	tbButtons_[2].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
	tbButtons_[2].dwData = 0;
	tbButtons_[2].iString = reinterpret_cast<INT_PTR>(L"About");
	DeleteObject(hBitmap);

	SendMessage(hWndToolbar_, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(hImageList_));
	SendMessage(hWndToolbar_, TB_ADDBUTTONS, 3, reinterpret_cast<LPARAM>(&tbButtons_));
	SendMessage(hWndToolbar_, TB_AUTOSIZE, 0, 0);

	GetProxy();

	return hResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT IeSwitchProxyBand::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if(reinterpret_cast<HWND>(lParam) == hWndToolbar_)
	{
		switch(LOWORD(wParam))
		{
			case 1:
				DialogBoxParam(g_hInstance,
							   MAKEINTRESOURCE(DLG_SETTINGS),
							   hWnd_,
							   s_IeSwitchProxyDlgProc,
							   reinterpret_cast<LPARAM>(this));
				break;

			case 2:
				MessageBox(NULL,
						   L"IeSwitchProxy 0.2.3 "
#ifdef _WIN64
						   L"(x64)"
#else
						   L"(x86)"				   
#endif
						   L"\nCopyright ©2007-2008 Liam Kirton <liam@int3.ws>\n\nhttp://int3.ws/",
						   L"IeSwitchProxy",
						   MB_ICONINFORMATION);
				break;

			default:
				break;
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT IeSwitchProxyBand::OnEraseBackground(WPARAM wParam, LPARAM lParam)
{
	RECT clientRect;
	GetClientRect(hWnd_, &clientRect);

	HDC hDC = reinterpret_cast<HDC>(wParam);
	
	DrawThemeParentBackground(hWnd_, hDC, &clientRect);
	
	if(hTheme_ != NULL)
	{
		DrawThemeBackground(hTheme_, hDC, RP_BAND, 0, &clientRect, NULL);
	}
	
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT IeSwitchProxyBand::OnKillFocus(WPARAM wParam, LPARAM lParam)
{
	if(site_ != NULL)
	{
		site_->OnFocusChangeIS(dynamic_cast<IDockingWindow *>(this), false);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT IeSwitchProxyBand::OnMove(WPARAM wParam, LPARAM lParam)
{
	InvalidateRect(hWnd_, NULL, TRUE);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT IeSwitchProxyBand::OnNotify(WPARAM wParam, LPARAM lParam)
{
	switch (reinterpret_cast<LPNMHDR>(lParam)->code)
	{
		case TBN_DROPDOWN:
			{
				HMENU hMenu = CreatePopupMenu();

				DWORD dwId = 1;
				for(std::vector<ProxySetting>::iterator i = proxySettings_.begin(); i != proxySettings_.end(); ++i)
				{
					ProxySetting proxySetting = (*i);

					MENUITEMINFO menuItemInfo;
					SecureZeroMemory(&menuItemInfo, sizeof(MENUITEMINFO));

					menuItemInfo.cbSize = sizeof(MENUITEMINFO);
					menuItemInfo.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID;
					menuItemInfo.fType = MFT_STRING;
					menuItemInfo.fState = MFS_ENABLED;
					menuItemInfo.wID = dwId++;

					std::wstringstream menuItemTextStream;
					menuItemTextStream << "&" << dwId - 1 << L". " << proxySetting.Name;
					proxySetting.Display = menuItemTextStream.str();
					menuItemInfo.dwTypeData = const_cast<LPWSTR>(proxySetting.Display.c_str());

					InsertMenuItem(hMenu, menuItemInfo.wID, false, &menuItemInfo);
				}

				POINT cursorPt;
				GetCursorPos(&cursorPt);

				UINT menuId = TrackPopupMenu(hMenu,
										 TPM_RIGHTALIGN | TPM_NONOTIFY | TPM_RETURNCMD,
										 cursorPt.x,
										 cursorPt.y,
										 0,
										 hWnd_,
										 NULL);

				if(menuId > 0)
				{
					dwId = 1;
					for(std::vector<ProxySetting>::iterator i = proxySettings_.begin(); i != proxySettings_.end(); ++i)
					{
						ProxySetting proxySetting = (*i);

						if(dwId++ == menuId)
						{
							SetProxy(proxySetting);
							UpdateBroadcast(1);
							break;
						}
					}
				}

				DestroyMenu(hMenu);
			}
			return TBDDRET_DEFAULT;

		default:
			break;
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT IeSwitchProxyBand::OnPaint(WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT paintStruct;
	HDC hDC;
	
	hDC = BeginPaint(hWnd_, &paintStruct);
	EndPaint(hWnd_, &paintStruct);
	
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT IeSwitchProxyBand::OnSetFocus(WPARAM wParam, LPARAM lParam)
{
	if(site_ != NULL)
    {
		site_->OnFocusChangeIS(dynamic_cast<IDockingWindow *>(this), true);
    }
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK IeSwitchProxyBand::s_IeSwitchProxyDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	IeSwitchProxyBand *pIeSwitchProxyBand = reinterpret_cast<IeSwitchProxyBand *>(GetWindowLongPtr(hwndDlg, GWLP_USERDATA));

	switch(uMsg)
	{
		case WM_INITDIALOG:
			pIeSwitchProxyBand = reinterpret_cast<IeSwitchProxyBand *>(lParam);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pIeSwitchProxyBand));
			{
				wchar_t *proxyName = new wchar_t[1024];
				wchar_t *proxyUrl = new wchar_t[1024];
				wchar_t *proxyExc = new wchar_t[1024];
				wchar_t *proxyString = new wchar_t[4096];

				for(std::vector<ProxySetting>::iterator i = pIeSwitchProxyBand->proxySettings_.begin(); i != pIeSwitchProxyBand->proxySettings_.end(); ++i)
				{
					ProxySetting proxySetting = (*i);

					StringCchCopy(proxyString, 4096, proxySetting.Name.c_str());
					StringCchCat(proxyString, 4096, L" :: ");
					StringCchCat(proxyString, 4096, proxySetting.Value.c_str());
					StringCchCat(proxyString, 4096, L" :: ");
					StringCchCat(proxyString, 4096, proxySetting.Exclusions.c_str());

					if(SendDlgItemMessage(hwndDlg, IDC_PROXY_LIST, LB_FINDSTRING, 0, reinterpret_cast<LPARAM>(proxyString)) == LB_ERR)
					{
						SendDlgItemMessage(hwndDlg, IDC_PROXY_LIST, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(proxyString));
					}
				}

				delete [] proxyName;
				delete [] proxyUrl;
				delete [] proxyExc;
				delete [] proxyString;
			}
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) 
			{ 
				case IDOK:
					{
						pIeSwitchProxyBand->proxySettings_.clear();
						
						wchar_t *proxyName = new wchar_t[1024];
						wchar_t *proxyUrl = new wchar_t[1024];
						wchar_t *proxyExc = new wchar_t[1024];
						wchar_t *proxyString = new wchar_t[4096];

						size_t proxyStringsCount = SendDlgItemMessage(hwndDlg, IDC_PROXY_LIST, LB_GETCOUNT, 0, 0);
						if(proxyStringsCount != LB_ERR)
						{
							for(size_t i = 0; i < proxyStringsCount; ++i)
							{
								proxyName[0] = L'\0';
								proxyUrl[0] = L'\0';
								proxyExc[0] = L'\0';

								if(SendDlgItemMessage(hwndDlg, IDC_PROXY_LIST, LB_GETTEXT, i, reinterpret_cast<LPARAM>(proxyString)) != LB_ERR)
								{
									size_t proxyStringLength = 0;
									StringCchLength(proxyString, 2048, &proxyStringLength);

									for(size_t i = 0; i < proxyStringLength; ++i)
									{
										if((proxyString[i] == L' ') &&
										   (proxyString[i + 1] == L':') &&
										   (proxyString[i + 2] == L':') &&
										   (proxyString[i + 3] == L' '))
										{
											for(size_t j = i + 4; j < proxyStringLength; ++j)
											{
												if((proxyString[j] == L' ') &&
												   (proxyString[j + 1] == L':') &&
												   (proxyString[j + 2] == L':') &&
												   (proxyString[j + 3] == L' '))
												{
													proxyString[i] = L'\0';
													proxyString[j] = L'\0';

													StringCchCopy(proxyName, 1024, proxyString);
													StringCchCopy(proxyUrl, 1024, proxyString + (i + 4));
													StringCchCopy(proxyExc, 1024, proxyString + (j + 4));

													std::wstring proxyStringName = proxyName;
													std::wstring proxyStringUrl = proxyUrl;
													std::wstring proxyStringExc = proxyExc;
													
													ProxySetting proxySetting;
													proxySetting.Name = proxyStringName;
													proxySetting.Value = proxyStringUrl;
													proxySetting.Exclusions = proxyStringExc;

													pIeSwitchProxyBand->proxySettings_.push_back(proxySetting);
													break;
												}
											}
											break;
										}
									}
								}
							}
						}

						delete [] proxyName;
						delete [] proxyUrl;
						delete [] proxyExc;
						delete [] proxyString;

						pIeSwitchProxyBand->SaveSettings();
						pIeSwitchProxyBand->UpdateBroadcast(2);
					}

				case IDCANCEL:
					EndDialog(hwndDlg, 0);
					break;

				case IDC_PROXY_ADD:
				{
					wchar_t *proxyName = new wchar_t[1024];
					wchar_t *proxyUrl = new wchar_t[1024];
					wchar_t *proxyExc = new wchar_t[1024];
					wchar_t *proxyString = new wchar_t[4096];

					proxyName[0] = L'\0';
					proxyUrl[0] = L'\0';
					proxyExc[0] = L'\0';
					
					if(SendDlgItemMessage(hwndDlg, IDC_PROXY_NAME_EDIT, WM_GETTEXT, 1024, reinterpret_cast<LPARAM>(proxyName)) != LB_ERR)
					{
						if(SendDlgItemMessage(hwndDlg, IDC_PROXY_URL_EDIT, WM_GETTEXT, 1024, reinterpret_cast<LPARAM>(proxyUrl)) != LB_ERR)
						{
							if(SendDlgItemMessage(hwndDlg, IDC_PROXY_EXC_EDIT, WM_GETTEXT, 1024, reinterpret_cast<LPARAM>(proxyExc)) != LB_ERR)
							{
								StringCchCopy(proxyString, 4096, proxyName);
								StringCchCat(proxyString, 4096, L" :: ");
								StringCchCat(proxyString, 4096, proxyUrl);
								StringCchCat(proxyString, 4096, L" :: ");
								StringCchCat(proxyString, 4096, proxyExc);

								if(SendDlgItemMessage(hwndDlg, IDC_PROXY_LIST, LB_FINDSTRING, 0, reinterpret_cast<LPARAM>(proxyString)) == LB_ERR)
								{
									SendDlgItemMessage(hwndDlg, IDC_PROXY_LIST, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(proxyString));

									wchar_t nullText = L'\0';
									SendDlgItemMessage(hwndDlg, IDC_PROXY_NAME_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(&nullText));
									SendDlgItemMessage(hwndDlg, IDC_PROXY_URL_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(&nullText));
									SendDlgItemMessage(hwndDlg, IDC_PROXY_EXC_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(&nullText));
								}
							}
						}
					}

					delete [] proxyName;
					delete [] proxyUrl;
					delete [] proxyExc;
					delete [] proxyString;
				}
				return TRUE;

			case IDC_PROXY_UPD:
				{
					wchar_t *proxyName = new wchar_t[1024];
					wchar_t *proxyUrl = new wchar_t[1024];
					wchar_t *proxyExc = new wchar_t[1024];
					wchar_t *proxyString = new wchar_t[4096];

					proxyName[0] = L'\0';
					proxyUrl[0] = L'\0';
					proxyExc[0] = L'\0';
					
					if(SendDlgItemMessage(hwndDlg, IDC_PROXY_NAME_EDIT, WM_GETTEXT, 1024, reinterpret_cast<LPARAM>(proxyName)) != LB_ERR)
					{
						if(SendDlgItemMessage(hwndDlg, IDC_PROXY_URL_EDIT, WM_GETTEXT, 1024, reinterpret_cast<LPARAM>(proxyUrl)) != LB_ERR)
						{
							if(SendDlgItemMessage(hwndDlg, IDC_PROXY_EXC_EDIT, WM_GETTEXT, 1024, reinterpret_cast<LPARAM>(proxyExc)) != LB_ERR)
							{
								StringCchCopy(proxyString, 4096, proxyName);
								StringCchCat(proxyString, 4096, L" :: ");
								StringCchCat(proxyString, 4096, proxyUrl);
								StringCchCat(proxyString, 4096, L" :: ");
								StringCchCat(proxyString, 4096, proxyExc);

								LRESULT curSel = 0;
								if((curSel = SendDlgItemMessage(hwndDlg, IDC_PROXY_LIST, LB_GETCURSEL, 0, 0)) != LB_ERR)
								{
									if(SendDlgItemMessage(hwndDlg, IDC_PROXY_LIST, LB_DELETESTRING, curSel, 0) != LB_ERR)
									{
										if(SendDlgItemMessage(hwndDlg, IDC_PROXY_LIST, LB_INSERTSTRING, curSel, reinterpret_cast<LPARAM>(proxyString)) != LB_ERR)
										{
											SendDlgItemMessage(hwndDlg, IDC_PROXY_LIST, LB_SETCURSEL, curSel, 0);
										}
									}
								}
							}
						}
					}

					delete [] proxyName;
					delete [] proxyUrl;
					delete [] proxyExc;
					delete [] proxyString;
				}
				return TRUE;

			case IDC_PROXY_DEL:
				{
					LRESULT curSel = 0;
					if((curSel = SendDlgItemMessage(hwndDlg, IDC_PROXY_LIST, LB_GETCURSEL, 0, 0)) != LB_ERR)
					{
						SendDlgItemMessage(hwndDlg, IDC_PROXY_LIST, LB_DELETESTRING, curSel, 0);
					}
				}

				return TRUE;

			case IDC_PROXY_UP:
				{
					wchar_t *proxyString = new wchar_t[4096];

					LRESULT curSel = 0;
					if(((curSel = SendDlgItemMessage(hwndDlg, IDC_PROXY_LIST, LB_GETCURSEL, 0, 0)) != LB_ERR) &&
					   (curSel > 0))
					{
						if(SendDlgItemMessage(hwndDlg, IDC_PROXY_LIST, LB_GETTEXT, curSel, reinterpret_cast<LPARAM>(proxyString)) != 0)
						{
							if(SendDlgItemMessage(hwndDlg, IDC_PROXY_LIST, LB_DELETESTRING, curSel, 0) != LB_ERR)
							{
								if(SendDlgItemMessage(hwndDlg, IDC_PROXY_LIST, LB_INSERTSTRING, curSel - 1, reinterpret_cast<LPARAM>(proxyString)) != LB_ERR)
								{
									SendDlgItemMessage(hwndDlg, IDC_PROXY_LIST, LB_SETCURSEL, curSel - 1, 0);
								}
							}
						}
					}

					delete [] proxyString;
				}
				
				return TRUE;

			case IDC_PROXY_DOWN:
				{
					wchar_t *proxyString = new wchar_t[4096];

					LRESULT curSel = 0;
					if((curSel = SendDlgItemMessage(hwndDlg, IDC_PROXY_LIST, LB_GETCURSEL, 0, 0)) != LB_ERR)
					{
						LRESULT curCount = 0;
						if(((curCount = SendDlgItemMessage(hwndDlg, IDC_PROXY_LIST, LB_GETCOUNT, 0, 0)) != LB_ERR) &&
						   (curSel < (curCount - 1)))
						{
							if(SendDlgItemMessage(hwndDlg, IDC_PROXY_LIST, LB_GETTEXT, curSel, reinterpret_cast<LPARAM>(proxyString)) != 0)
							{
								if(SendDlgItemMessage(hwndDlg, IDC_PROXY_LIST, LB_DELETESTRING, curSel, 0) != LB_ERR)
								{
									if(SendDlgItemMessage(hwndDlg, IDC_PROXY_LIST, LB_INSERTSTRING, curSel + 1, reinterpret_cast<LPARAM>(proxyString)) != LB_ERR)
									{
										SendDlgItemMessage(hwndDlg, IDC_PROXY_LIST, LB_SETCURSEL, curSel + 1, 0);
									}
								}
							}
						}
					}

					delete [] proxyString;
				}
				
				return TRUE;

			case IDC_PROXY_LIST:
				switch (HIWORD(wParam))
				{
					case LBN_SELCHANGE:
						{
							wchar_t *proxyName = new wchar_t[1024];
							wchar_t *proxyUrl = new wchar_t[1024];
							wchar_t *proxyExc = new wchar_t[1024];
							wchar_t *proxyString = new wchar_t[2048];

							proxyName[0] = L'\0';
							proxyUrl[0] = L'\0';
							proxyExc[0] = L'\0';
							
							if(SendDlgItemMessage(hwndDlg, IDC_PROXY_LIST, LB_GETTEXT, SendDlgItemMessage(hwndDlg, IDC_PROXY_LIST, LB_GETCURSEL, 0, 0), reinterpret_cast<LPARAM>(proxyString)) != 0)
							{
								size_t proxyStringLength = 0;
								StringCchLength(proxyString, 2048, &proxyStringLength);

								for(size_t i = 0; i < proxyStringLength; ++i)
								{
									if((proxyString[i] == L' ') &&
									   (proxyString[i + 1] == L':') &&
									   (proxyString[i + 2] == L':') &&
									   (proxyString[i + 3] == L' '))
									{
										for(size_t j = i + 4; j < proxyStringLength; ++j)
										{
											if((proxyString[j] == L' ') &&
											   (proxyString[j + 1] == L':') &&
											   (proxyString[j + 2] == L':') &&
											   (proxyString[j + 3] == L' '))
											{
												proxyString[i] = L'\0';
												proxyString[j] = L'\0';

												StringCchCopy(proxyName, 1024, proxyString);
												StringCchCopy(proxyUrl, 1024, proxyString + (i + 4));
												StringCchCopy(proxyExc, 1024, proxyString + (j + 4));

												SendDlgItemMessage(hwndDlg, IDC_PROXY_NAME_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(proxyName));
												SendDlgItemMessage(hwndDlg, IDC_PROXY_URL_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(proxyUrl));
												SendDlgItemMessage(hwndDlg, IDC_PROXY_EXC_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(proxyExc));
												break;
											}
										}
										break;
									}
								}
							}

							delete [] proxyName;
							delete [] proxyUrl;
							delete [] proxyExc;
							delete [] proxyString;
						}
						break;
				}
				return TRUE;
			}
			return TRUE;

		default:
			break;
	}

	return FALSE; 
}

////////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK IeSwitchProxyBand::s_IeSwitchProxyWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	IeSwitchProxyBand *pIeSwitchProxyBand = reinterpret_cast<IeSwitchProxyBand *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch(uMsg)
	{
		case WM_NCCREATE:
			{
				LPCREATESTRUCT lpCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
				pIeSwitchProxyBand = reinterpret_cast<IeSwitchProxyBand *>(lpCreateStruct->lpCreateParams);
				SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pIeSwitchProxyBand));
				pIeSwitchProxyBand->hWnd_ = hWnd;
			}
			break;

		case WM_COMMAND:
			if(pIeSwitchProxyBand != NULL)
			{
				return pIeSwitchProxyBand->OnCommand(wParam, lParam);
			}
			break;

		case WM_ERASEBKGND:
			if(pIeSwitchProxyBand != NULL)
			{
				return pIeSwitchProxyBand->OnEraseBackground(wParam, lParam);
			}
			break;

		case WM_KILLFOCUS:
			if(pIeSwitchProxyBand != NULL)
			{
				return pIeSwitchProxyBand->OnKillFocus(wParam, lParam);
			}
			break;

		case WM_MOVE:
			if(pIeSwitchProxyBand != NULL)
			{
				return pIeSwitchProxyBand->OnMove(wParam, lParam);
			}
			break;

		case WM_NOTIFY:
			if(pIeSwitchProxyBand != NULL)
			{
				return pIeSwitchProxyBand->OnNotify(wParam, lParam);
			}
			break;

		case WM_PAINT:
			if(pIeSwitchProxyBand != NULL)
			{
				return pIeSwitchProxyBand->OnPaint(wParam, lParam);
			}
			break;

		case WM_SETFOCUS:
			if(pIeSwitchProxyBand != NULL)
			{
				return pIeSwitchProxyBand->OnSetFocus(wParam, lParam);
			}
			break;

		case WM_SIZE:
			if((pIeSwitchProxyBand != NULL) && (pIeSwitchProxyBand->hWndToolbar_ != NULL))
			{
				SetWindowPos(pIeSwitchProxyBand->hWndToolbar_, NULL, 0, 0, LOWORD(lParam), HIWORD(lParam), 0);
			}
			break;

		case WM_THEMECHANGED:
			if(pIeSwitchProxyBand != NULL)
			{
				if(pIeSwitchProxyBand->hTheme_ != NULL)
				{
					CloseThemeData(pIeSwitchProxyBand->hTheme_);
					pIeSwitchProxyBand->hTheme_ = NULL;
				}
				pIeSwitchProxyBand->hTheme_ = OpenThemeData(pIeSwitchProxyBand->hWnd_, L"ReBar");
			}
			break;

		case WM_USER + 1:
			pIeSwitchProxyBand->GetProxy();
			break;

		case WM_USER + 2:
			pIeSwitchProxyBand->LoadSettings();
			break;

		default:
			break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK IeSwitchProxyBand::s_EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
	EnumChildWindows(hWnd, s_EnumChildWindowsProc, lParam);
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK IeSwitchProxyBand::s_EnumChildWindowsProc(HWND hWnd, LPARAM lParam)
{
	wchar_t wndClassName[1024];
	GetClassName(hWnd, wndClassName, 1023);
	
	if(lstrcmpW(L"IeSwitchProxy", wndClassName) == 0)
	{
		PostMessage(hWnd, WM_USER + static_cast<UINT>(lParam), 0, 0);
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
