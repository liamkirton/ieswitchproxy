////////////////////////////////////////////////////////////////////////////////////////////////////
// IeSwitchProxy
//
// Copyright ©2007-2008 Liam Kirton <liam@int3.ws>
////////////////////////////////////////////////////////////////////////////////////////////////////
// IeSwitchProxyBand.h
//
// Created: 26/09/2007
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <shlobj.h>
#include <commctrl.h>

#include <sstream>
#include <string>
#include <vector>

#include "IeSwitchProxy.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

struct ProxySetting
{
	std::wstring Name;
	std::wstring Value;
	std::wstring Exclusions;

	std::wstring Display;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

class IeSwitchProxyBand : public IDeskBand, public IInputObject, public IObjectWithSite, public IPersistStream
{
public:
	IeSwitchProxyBand();
	virtual ~IeSwitchProxyBand();

	// IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppReturn);
	STDMETHODIMP_(DWORD) AddRef();
	STDMETHODIMP_(DWORD) Release();

	// IOleWindow
	STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);
	STDMETHODIMP GetWindow(HWND *phwnd);

	// IDockingWindow
	STDMETHODIMP CloseDW(DWORD dwReserved);
	STDMETHODIMP ResizeBorderDW(LPCRECT prcBorder, IUnknown* punkToolbarSite, BOOL fReserved);
	STDMETHODIMP ShowDW(BOOL bShow);

	// IDeskBand
	STDMETHODIMP GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO* pdbi);

	// IInputObject
	STDMETHODIMP HasFocusIO();
	STDMETHODIMP TranslateAcceleratorIO(LPMSG lpMsg);
	STDMETHODIMP UIActivateIO(BOOL fActivate, LPMSG lpMsg);

	// IObjectWithSite
	STDMETHODIMP SetSite(IUnknown* pUnkSite);
	STDMETHODIMP GetSite(REFIID riid, void** ppvSite);

	// IPersist
	STDMETHODIMP GetClassID(CLSID *pClassID);

	// IPersistStream
	STDMETHODIMP IsDirty();
	STDMETHODIMP Load(IStream *pStm);
	STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty);
	STDMETHODIMP GetSizeMax(ULARGE_INTEGER *pcbSize);

protected:
	static INT_PTR CALLBACK s_IeSwitchProxyDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK s_IeSwitchProxyWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	static BOOL CALLBACK s_EnumWindowsProc(HWND hWnd, LPARAM lParam);
	static BOOL CALLBACK s_EnumChildWindowsProc(HWND hWnd, LPARAM lParam);

	void LoadSettings();
	void SaveSettings();

	void GetProxy();
	void SetProxy(ProxySetting &proxySetting);
	void UpdateBroadcast(UINT code);

	void CloseIeSwitchProxyWindow();
	HRESULT CreateIeSwitchProxyWindow();

	LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
	LRESULT OnEraseBackground(WPARAM wParam, LPARAM lParam);
	LRESULT OnKillFocus(WPARAM wParam, LPARAM lParam);
	LRESULT OnMove(WPARAM wParam, LPARAM lParam);
	LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
	LRESULT OnPaint(WPARAM wParam, LPARAM lParam);
	LRESULT OnSetFocus(WPARAM wParam, LPARAM lParam);
	
protected:
	DWORD dwBandID_;
	DWORD dwObjRefCount_;

	IInputObjectSite *site_;

	HWND hWndParent_;
	HWND hWnd_;
	HWND hWndToolbar_;

	HTHEME hTheme_;
	
	HIMAGELIST hImageList_;
	TBBUTTON tbButtons_[3];

	std::vector<ProxySetting> proxySettings_;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
