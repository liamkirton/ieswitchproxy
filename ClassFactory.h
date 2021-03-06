////////////////////////////////////////////////////////////////////////////////////////////////////
// IeSwitchProxy
//
// Copyright �2007-2008 Liam Kirton <liam@int3.ws>
////////////////////////////////////////////////////////////////////////////////////////////////////
// ClassFactory.h
//
// Created: 26/09/2007
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <shlobj.h>

#include "IeSwitchProxy.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

class ClassFactory : public IClassFactory
{
public:
	ClassFactory(CLSID clsid);
	virtual ~ClassFactory();

	// IUnknown
	STDMETHODIMP QueryInterface(REFIID iid, void **ppvObject);
	STDMETHODIMP_(DWORD) AddRef();
	STDMETHODIMP_(DWORD) Release();

	// IClassFactory
	STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject);
	STDMETHODIMP LockServer(BOOL fLock);

protected:
	CLSID clsid_;
	DWORD dwObjRefCount_;
};

////////////////////////////////////////////////////////////////////////////////////////////////////