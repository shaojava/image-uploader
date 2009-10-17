/*
    Image Uploader - program for uploading images/files to Internet
    Copyright (C) 2007-2009 ZendeN <zenden2k@gmail.com>
	 
    HomePage:    http://zenden.ws/imageuploader

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"

#include <gdiplus.h>
#include <shellapi.h>
#include "resource.h"
#include "aboutdlg.h"
//#include "uploader.h"
#include "MainDlg.h"
#include "wizarddlg.h"
#include "floatingwindow.h"


int Run(LPTSTR lpstrCmdLine = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CreateTempFolder();
	GdiplusStartupInput gdiplusStartupInput;
	
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CWizardDlg dlgMain;
	

	DWORD DlgCreationResult = 0;
	bool ShowMainWindow= true;
	Settings.LoadSettings();

	bool BecomeTray = false;
	if(Settings.ShowTrayIcon && !CmdLine.IsOption(_T("tray")))
	{
		if(!IsRunningFloatingWnd())
		{
			BecomeTray = true;
			CmdLine.AddParam(_T("/tray"));
		}	
	}


	if(CmdLine.IsOption(_T("tray")) || BecomeTray)
	{
		if(!IsRunningFloatingWnd())
		{
			ShowMainWindow = BecomeTray;
			floatWnd.CreateTrayIcon();
			//CreateFloatWindow();
		}
		else return 0;
	}
	//else
	{
		pWizardDlg = &dlgMain;
		if(dlgMain.Create(0,(LPARAM)&DlgCreationResult) == NULL)
		{
			ATLTRACE(_T("Main dialog creation failed!  :( sorry\n"));
			dlgMain.m_hWnd = 0;
			return 0;
		}
		
		if(DlgCreationResult != 0) 
		{
			dlgMain.m_hWnd = 0;
			return 0;
		}
		
		if((CmdLine.GetCount()>1 && CmdLine.IsOption(_T("quickshot")))|| CmdLine.IsOption(_T("mediainfo")) || !ShowMainWindow || !dlgMain.m_bShowWindow)
		{
			dlgMain.ShowWindow(SW_HIDE);
		}
		else dlgMain.ShowWindow(nCmdShow);
	}
	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();

	GdiplusShutdown(gdiplusToken);
	
	ClearTempFolder(); // ������� ��������� �����
	
	return /*nRet*/0;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{		
	OleInitialize(NULL);
	HRESULT hRes ;
	if(CmdLine.IsOption(_T("integration"))) // for Windows Vista+
	{
		Settings.LoadSettings();		
		Settings.ApplyRegSettingsRightNow();
		return 0;
	}

	//::CoUninitialize();
	//OleUninitialize();

	

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	OleUninitialize();
	//::CoUninitialize();

	return nRet;
}
