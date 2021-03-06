// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseGit
// Copyright (C) 2003-2006, 2008, 2013-2015 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
#include <assert.h>
#include "LangDll.h"
#include "..\version.h"
#include <memory>

#pragma comment(lib, "Version.lib")

CLangDll::CLangDll()
{
	m_hInstance = nullptr;
}

CLangDll::~CLangDll()
{
}

HINSTANCE CLangDll::Init(LPCTSTR appname, unsigned long langID)
{
	TCHAR langpath[MAX_PATH] = {0};
	TCHAR langdllpath[MAX_PATH] = {0};
	TCHAR sVer[MAX_PATH] = {0};
	_tcscpy_s(sVer, MAX_PATH, _T(STRPRODUCTVER));
	GetModuleFileName(nullptr, langpath, MAX_PATH);
	TCHAR * pSlash = _tcsrchr(langpath, '\\');
	if (!pSlash)
		return m_hInstance;

	*pSlash = 0;
	pSlash = _tcsrchr(langpath, '\\');
	if (!pSlash)
		return m_hInstance;

	*pSlash = 0;
	_tcscat_s(langpath, MAX_PATH, _T("\\Languages\\"));
	assert(m_hInstance == nullptr);
	do
	{
		_stprintf_s(langdllpath, MAX_PATH, _T("%s%s%lu.dll"), langpath, appname, langID);

		m_hInstance = LoadLibrary(langdllpath);

		if (!DoVersionStringsMatch(sVer, langdllpath))
		{
			FreeLibrary(m_hInstance);
			m_hInstance = nullptr;
		}
		if (!m_hInstance)
		{
			DWORD lid = SUBLANGID(langID);
			lid--;
			if (lid > 0)
				langID = MAKELANGID(PRIMARYLANGID(langID), lid);
			else
				langID = 0;
		}
	} while (!m_hInstance && (langID != 0));

	return m_hInstance;
}

void CLangDll::Close()
{
	if (!m_hInstance)
		return;

	FreeLibrary(m_hInstance);
	m_hInstance = nullptr;
}

bool CLangDll::DoVersionStringsMatch(LPCTSTR sVer, LPCTSTR langDll) const
{
	struct TRANSARRAY
	{
		WORD wLanguageID;
		WORD wCharacterSet;
	};

	DWORD dwReserved = 0;
	DWORD dwBufferSize = GetFileVersionInfoSize((LPTSTR)langDll,&dwReserved);

	if (dwBufferSize <= 0)
		return false;

	auto pBuffer = std::make_unique<BYTE[]>(dwBufferSize);

	if (!pBuffer)
		return false;

	UINT        nInfoSize = 0, nFixedLength = 0;
	LPSTR       lpVersion = nullptr;
	VOID*       lpFixedPointer;
	TRANSARRAY* lpTransArray;
	TCHAR       strLangProductVersion[MAX_PATH] = { 0 };

	if (!GetFileVersionInfo((LPTSTR)langDll, dwReserved, dwBufferSize, pBuffer.get()))
		return false;

	VerQueryValue(pBuffer.get(), _T("\\VarFileInfo\\Translation"), &lpFixedPointer, &nFixedLength);
	lpTransArray = (TRANSARRAY*)lpFixedPointer;

	_stprintf_s(strLangProductVersion, MAX_PATH, _T("\\StringFileInfo\\%04x%04x\\ProductVersion"), lpTransArray[0].wLanguageID, lpTransArray[0].wCharacterSet);

	VerQueryValue(pBuffer.get(), (LPTSTR)strLangProductVersion, (LPVOID*)&lpVersion, &nInfoSize);
	if (lpVersion && nInfoSize)
		return (_tcscmp(sVer, (LPCTSTR)lpVersion) == 0);

	return false;
}
