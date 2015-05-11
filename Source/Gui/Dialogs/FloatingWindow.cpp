/*

    Image Uploader -  free application for uploading images/files to the Internet

    Copyright 2007-2015 Sergey Svistunov (zenden2k@gmail.com)

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

*/

// This file was generated by WTL subclass control wizard
// FloatingWindow.cpp : Implementation of FloatingWindow

#include "FloatingWindow.h"

#include "ResultsPanel.h"
#include "ScreenshotDlg.h"
#include "Core/Settings.h"
#include "LogWindow.h"
#include "Core/ServiceLocator.h"
#include "Core/HistoryManager.h"
#include "Core/Utils/CoreTypes.h"
#include "Func/WebUtils.h"
#include "Func/WinUtils.h"
#include "Core/Upload/UrlShorteningTask.h"
#include "Func/IuCommonFunctions.h"
#include "Gui/GuiTools.h"
#include "Core/Upload/FileUploadTask.h"
#include "Func/myutils.h"

// FloatingWindow
CFloatingWindow::CFloatingWindow()
{
    m_bFromHotkey = false;
    m_ActiveWindow = 0;
    EnableClicks = true;
    hMutex = NULL;
    m_PrevActiveWindow = 0;
    m_bStopCapturingWindows = false;
    WM_TASKBARCREATED = RegisterWindowMessage(_T("TaskbarCreated"));
    m_bIsUploading = 0;
    uploadEngineManager_ = 0;
    lastUploadedItem_ = 0;
}

CFloatingWindow::~CFloatingWindow()
{
    CloseHandle(hMutex);
    DeleteObject(m_hIconSmall);
    m_hWnd = 0;
}

void CFloatingWindow::setWizardDlg(CWizardDlg* wizardDlg) {
    wizardDlg_ = wizardDlg;
}

LRESULT CFloatingWindow::OnClose(void)
{
    return 0;
}

bool MyInsertMenu(HMENU hMenu, int pos, UINT id, const LPCTSTR szTitle,  HBITMAP bm = NULL)
{
    MENUITEMINFO MenuItem;

    MenuItem.cbSize = sizeof(MenuItem);
    if (szTitle)
        MenuItem.fType = MFT_STRING;
    else
        MenuItem.fType = MFT_SEPARATOR;
    MenuItem.fMask = MIIM_TYPE | MIIM_ID | MIIM_DATA;
    if (bm)
        MenuItem.fMask |= MIIM_CHECKMARKS;
    MenuItem.wID = id;
    MenuItem.hbmpChecked = bm;
    MenuItem.hbmpUnchecked = bm;
    MenuItem.dwTypeData = (LPWSTR)szTitle;
    return InsertMenuItem(hMenu, pos, TRUE, &MenuItem) != 0;
}

LRESULT CFloatingWindow::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    /*int w = ::GetSystemMetrics(SM_CXSMICON);
    if ( w > 32 ) {
        w = 48;
    } else if ( w > 16 ) {
        w = 32;
    }
    int h = w;*/
    m_hIconSmall = GuiTools::LoadSmallIcon(IDR_MAINFRAME);
    SetIcon(m_hIconSmall, FALSE);

    RegisterHotkeys();
    InstallIcon(APPNAME, m_hIconSmall, /*TrayMenu*/ 0);
    NOTIFYICONDATA nid;
    ZeroMemory(&nid, sizeof(nid));
    nid.cbSize = NOTIFYICONDATA_V2_SIZE;
    nid.hWnd = m_hWnd;
    nid.uVersion = NOTIFYICON_VERSION;
    Shell_NotifyIcon(NIM_SETVERSION, &nid);
    return 0;
}

LRESULT CFloatingWindow::OnExit(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    wizardDlg_->CloseWizard();
    return 0;
}

LRESULT CFloatingWindow::OnTrayIcon(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    if (!EnableClicks )
        return 0;

    if (LOWORD(lParam) == WM_LBUTTONDOWN)
    {
        m_bStopCapturingWindows = true;
    }
    if (LOWORD(lParam) == WM_MOUSEMOVE)
    {
        if (!m_bStopCapturingWindows)
        {
            HWND wnd =  GetForegroundWindow();
            if (wnd != m_hWnd)
                m_PrevActiveWindow = GetForegroundWindow();
        }
    }
    if (LOWORD(lParam) == WM_RBUTTONUP)
    {
        if (m_bIsUploading && Settings.Hotkeys[Settings.TrayIconSettings.RightClickCommand].commandId != IDM_CONTEXTMENU)
            return 0;
        SendMessage(WM_COMMAND, MAKEWPARAM(Settings.Hotkeys[Settings.TrayIconSettings.RightClickCommand].commandId, 0));
    }
    else if (LOWORD(lParam) == WM_LBUTTONDBLCLK)
    {
        EnableClicks = false;
        KillTimer(1);
        SetTimer(2, GetDoubleClickTime());
        if (m_bIsUploading && Settings.Hotkeys[Settings.TrayIconSettings.LeftDoubleClickCommand].commandId !=
            IDM_CONTEXTMENU)
            return 0;
        SendMessage(WM_COMMAND,
                    MAKEWPARAM(Settings.Hotkeys[Settings.TrayIconSettings.LeftDoubleClickCommand].commandId, 0));
    }
    else if (LOWORD(lParam) == WM_LBUTTONUP)
    {
        m_bStopCapturingWindows = false;
        if (m_bIsUploading && Settings.Hotkeys[Settings.TrayIconSettings.LeftDoubleClickCommand].commandId !=
            IDM_CONTEXTMENU)
            return 0;

        if (!Settings.Hotkeys[Settings.TrayIconSettings.LeftDoubleClickCommand].commandId)
            SendMessage(WM_COMMAND, MAKEWPARAM(Settings.Hotkeys[Settings.TrayIconSettings.LeftClickCommand].commandId, 0));
        else
            SetTimer(1, (UINT) (1.2 * GetDoubleClickTime()));
    }
    else if (LOWORD(lParam) == WM_MBUTTONUP)
    {
        if (m_bIsUploading && Settings.Hotkeys[Settings.TrayIconSettings.MiddleClickCommand].commandId != IDM_CONTEXTMENU)
            return 0;

        SendMessage(WM_COMMAND, MAKEWPARAM(Settings.Hotkeys[Settings.TrayIconSettings.MiddleClickCommand].commandId, 0));
    }
    else if (LOWORD(lParam) == NIN_BALLOONUSERCLICK)
    {
        std::vector<CUrlListItem> items;
        CUrlListItem it;
        if (lastUploadedItem_) {
            UploadResult* uploadResult = lastUploadedItem_->uploadResult();
            it.ImageUrl = Utf8ToWstring(uploadResult->directUrl).c_str();
            it.ImageUrlShortened = Utf8ToWstring(uploadResult->directUrlShortened).c_str();
            it.ThumbUrl = Utf8ToWstring(uploadResult->thumbUrl).c_str();
            it.DownloadUrl = Utf8ToWstring(uploadResult->downloadUrl).c_str();
            it.DownloadUrlShortened = Utf8ToWCstring(uploadResult->downloadUrlShortened);
            items.push_back(it);
            if (it.ImageUrl.IsEmpty() && it.DownloadUrl.IsEmpty())
                return 0;
            CResultsWindow rp(wizardDlg_, items, false);
            rp.DoModal(m_hWnd);
        }
    }
    return 0;
}

LRESULT CFloatingWindow::OnMenuSettings(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (!wizardDlg_->IsWindowEnabled())
    {
        HWND childModalDialog = wizardDlg_->GetLastActivePopup();
        if (childModalDialog && ::IsWindowVisible(childModalDialog))
            SetForegroundWindow(childModalDialog);
        return 0;
    }
    HWND hParent  = wizardDlg_->m_hWnd; // wizardDlg_->IsWindowEnabled()?  : 0;
    CSettingsDlg dlg(CSettingsDlg::spTrayIcon, uploadEngineManager_);
    dlg.DoModal(hParent);
    return 0;
}

LRESULT CFloatingWindow::OnCloseTray(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    ShowWindow(SW_HIDE);
    wizardDlg_->ShowWindow(SW_SHOW);
    wizardDlg_->SetDlgItemText(IDCANCEL, TR("�����"));
    CloseHandle(hMutex);
    RemoveIcon();
    UnRegisterHotkeys();
    DestroyWindow();
    hMutex = NULL;
    m_hWnd = 0;
    return 0;
}

LRESULT CFloatingWindow::OnReloadSettings(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!lParam)
        UnRegisterHotkeys();

    if (!wParam)
        Settings.LoadSettings();

    if (!lParam)
        RegisterHotkeys();
    return 0;
}

LRESULT CFloatingWindow::OnImportvideo(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (wizardDlg_->executeFunc(_T("importvideo,1")))
        wizardDlg_->ShowWindow(SW_SHOW);
    return 0;
}

LRESULT CFloatingWindow::OnUploadFiles(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (wizardDlg_->executeFunc(_T("addfiles,1")))
        wizardDlg_->ShowWindow(SW_SHOW);
    return 0;
}

LRESULT CFloatingWindow::OnReUploadImages(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (wizardDlg_->executeFunc(_T("reuploadimages,1"))) {
        wizardDlg_->ShowWindow(SW_SHOW);
    }
    return 0;
}


LRESULT CFloatingWindow::OnUploadImages(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (wizardDlg_->executeFunc(_T("addimages,1")))
        wizardDlg_->ShowWindow(SW_SHOW);
    return 0;
}

LRESULT CFloatingWindow::OnPasteFromWeb(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (wizardDlg_->executeFunc(_T("downloadimages,1")))
        wizardDlg_->ShowWindow(SW_SHOW);
    return 0;
}

LRESULT CFloatingWindow::OnScreenshotDlg(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    wizardDlg_->executeFunc(_T("screenshotdlg,2"));
    return 0;
}

LRESULT CFloatingWindow::OnRegionScreenshot(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    wizardDlg_->executeFunc(_T("regionscreenshot_dontshow,") + (m_bFromHotkey ? CString(_T("1")) : CString(_T("2"))));
    return 0;
}

LRESULT CFloatingWindow::OnFullScreenshot(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    wizardDlg_->executeFunc(_T("fullscreenshot,") + (m_bFromHotkey ? CString(_T("1")) : CString(_T("2"))));
    return 0;
}

LRESULT CFloatingWindow::OnWindowHandleScreenshot(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    wizardDlg_->executeFunc(_T("windowhandlescreenshot,") + (m_bFromHotkey ? CString(_T("1")) : CString(_T("2"))));
    return 0;
}

LRESULT CFloatingWindow::OnFreeformScreenshot(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    wizardDlg_->executeFunc(_T("freeformscreenshot,") + (m_bFromHotkey ? CString(_T("1")) : CString(_T("2"))));
    return 0;
}

LRESULT CFloatingWindow::OnShortenUrlClipboard(WORD wNotifyCode, WORD wID, HWND hWndCtl) {
    if (lastUrlShorteningTask_ && lastUrlShorteningTask_->isRunning()) {
        return false;
    }

     CString url;
    WinUtils::GetClipboardText(url);
    if ( !url.IsEmpty() && !WebUtils::DoesTextLookLikeUrl(url) ) {
        return false;
    }

    lastUrlShorteningTask_.reset(new UrlShorteningTask(WCstringToUtf8(url)));
    lastUrlShorteningTask_->setServerProfile(Settings.urlShorteningServer);
    lastUrlShorteningTask_->addTaskFinishedCallback(UploadTask::TaskFinishedCallback(this, &CFloatingWindow::OnFileFinished));
    uploadManager_->addTask(lastUrlShorteningTask_);
    uploadManager_->start();

    CString msg;
    msg.Format(TR("C������� ������ \"%s\" � ������� %s"), (LPCTSTR)url,
        (LPCTSTR)Utf8ToWstring(Settings.urlShorteningServer.serverName()).c_str());
    ShowBaloonTip(msg, _T("Image Uploader"));
    return 0;
}

LRESULT CFloatingWindow::OnWindowScreenshot(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (m_PrevActiveWindow)
        SetForegroundWindow(m_PrevActiveWindow);
    wizardDlg_->executeFunc(_T("windowscreenshot_delayed,") + (m_bFromHotkey ? CString(_T("1")) : CString(_T("2"))));

    return 0;
}

LRESULT CFloatingWindow::OnAddFolder(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (wizardDlg_->executeFunc(_T("addfolder")))
        wizardDlg_->ShowWindow(SW_SHOW);
    return 0;
}

LRESULT CFloatingWindow::OnShortenUrl(WORD wNotifyCode, WORD wID, HWND hWndCtl) {
    if (wizardDlg_->executeFunc(_T("shortenurl")))
        wizardDlg_->ShowWindow(SW_SHOW);
    return 0;
}


LRESULT CFloatingWindow::OnShowAppWindow(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (wizardDlg_->IsWindowEnabled())
    {
        wizardDlg_->ShowWindow(SW_SHOWNORMAL);
        if (wizardDlg_->IsWindowVisible())
        {
            // SetForegroundWindow(m_hWnd);
            wizardDlg_->SetActiveWindow();
            SetForegroundWindow(wizardDlg_->m_hWnd);
        }
    }
    else
    {
        // Activating some child modal dialog if exists one
        HWND childModalDialog = wizardDlg_->GetLastActivePopup();
        if (childModalDialog && ::IsWindowVisible(childModalDialog))
            SetForegroundWindow(childModalDialog);
    }

    return 0;
}

LRESULT CFloatingWindow::OnContextMenu(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (!IsWindowEnabled())
        return 0;

    CMenu TrayMenu;
    TrayMenu.CreatePopupMenu();

    if (!m_bIsUploading)
    {
        // Inserting menu items
        int i = 0;
        MyInsertMenu(TrayMenu, i++, IDM_UPLOADFILES, TR("��������� �����") + CString(_T("...")));
        MyInsertMenu(TrayMenu, i++, IDM_ADDFOLDERS, TR("��������� �����") + CString(_T("...")));
        MyInsertMenu(TrayMenu, i++, 0, 0);
        bool IsClipboard = false;

        if (OpenClipboard())
        {
            IsClipboard = IsClipboardFormatAvailable(CF_BITMAP) != 0;
            CloseClipboard();
        }
        if (IsClipboard)
        {
            MyInsertMenu(TrayMenu, i++, IDM_PASTEFROMCLIPBOARD, TR("�������� �� ������"));
            MyInsertMenu(TrayMenu, i++, 0, 0);
        }
        MyInsertMenu(TrayMenu, i++, IDM_IMPORTVIDEO, TR("������ �����"));
        MyInsertMenu(TrayMenu, i++, 0, 0);
        MyInsertMenu(TrayMenu, i++, IDM_SCREENSHOTDLG, TR("��������") + CString(_T("...")));
        MyInsertMenu(TrayMenu, i++, IDM_REGIONSCREENSHOT, TR("������ ���������� �������"));
        MyInsertMenu(TrayMenu, i++, IDM_FULLSCREENSHOT, TR("������ ����� ������"));
        MyInsertMenu(TrayMenu, i++, IDM_WINDOWSCREENSHOT, TR("������ ��������� ����"));
        MyInsertMenu(TrayMenu, i++, IDM_WINDOWHANDLESCREENSHOT, TR("������ ���������� ��������"));
        MyInsertMenu(TrayMenu, i++, IDM_FREEFORMSCREENSHOT, TR("������ ������������ �����"));

        CMenu SubMenu;
        SubMenu.CreatePopupMenu();
        SubMenu.InsertMenu(
            0, MFT_STRING | MFT_RADIOCHECK |
            (Settings.TrayIconSettings.TrayScreenshotAction == TRAY_SCREENSHOT_OPENINEDITOR ? MFS_CHECKED : 0),
            IDM_SCREENTSHOTACTION_OPENINEDITOR, TR("������� � ���������"));
        SubMenu.InsertMenu(
           0, MFT_STRING | MFT_RADIOCHECK |
           (Settings.TrayIconSettings.TrayScreenshotAction == TRAY_SCREENSHOT_UPLOAD ? MFS_CHECKED : 0),
           IDM_SCREENTSHOTACTION_UPLOAD, TR("��������� �� ������"));
        SubMenu.InsertMenu(
           1, MFT_STRING | MFT_RADIOCHECK |
           (Settings.TrayIconSettings.TrayScreenshotAction == TRAY_SCREENSHOT_CLIPBOARD ? MFS_CHECKED : 0),
           IDM_SCREENTSHOTACTION_TOCLIPBOARD, TR("���������� � ����� ������"));
        SubMenu.InsertMenu(
           2, MFT_STRING | MFT_RADIOCHECK |
           (Settings.TrayIconSettings.TrayScreenshotAction == TRAY_SCREENSHOT_SHOWWIZARD ? MFS_CHECKED : 0),
           IDM_SCREENTSHOTACTION_SHOWWIZARD, TR("������� � ���� �������"));
        SubMenu.InsertMenu(
           3, MFT_STRING | MFT_RADIOCHECK |
           (Settings.TrayIconSettings.TrayScreenshotAction == TRAY_SCREENSHOT_ADDTOWIZARD ? MFS_CHECKED : 0),
           IDM_SCREENTSHOTACTION_ADDTOWIZARD, TR("�������� � ������"));

        MENUITEMINFO mi;
        mi.cbSize = sizeof(mi);
        mi.fMask = MIIM_TYPE | MIIM_ID | MIIM_SUBMENU;
        mi.fType = MFT_STRING;
        mi.hSubMenu = SubMenu;
        mi.wID = 10000;
        mi.dwTypeData  = TR("�������� �� �������");
        TrayMenu.InsertMenuItem(i++, true, &mi);

        SubMenu.Detach();
        MyInsertMenu(TrayMenu, i++, 0, 0);
        MyInsertMenu(TrayMenu, i++, IDM_SHORTENURL, TR("��������� ������"));
        MyInsertMenu(TrayMenu, i++, 0, 0);
        MyInsertMenu(TrayMenu, i++, IDM_SHOWAPPWINDOW, TR("�������� ���� ���������"));
        MyInsertMenu(TrayMenu, i++, 0, 0);
        MyInsertMenu(TrayMenu, i++, IDM_SETTINGS, TR("���������") + CString(_T("...")));
        MyInsertMenu(TrayMenu, i++, 0, 0);
        MyInsertMenu(TrayMenu, i++, IDM_EXIT, TR("�����"));
        if (Settings.Hotkeys[Settings.TrayIconSettings.LeftDoubleClickCommand].commandId)
        {
            SetMenuDefaultItem(TrayMenu, Settings.Hotkeys[Settings.TrayIconSettings.LeftDoubleClickCommand].commandId,
                               FALSE);
        }
    }
    else
        MyInsertMenu(TrayMenu, 0, IDM_STOPUPLOAD, TR("�������� ��������"));
    m_hTrayIconMenu = TrayMenu;
    CMenuHandle oPopup(m_hTrayIconMenu);
    PrepareMenu(oPopup);
    CPoint pos;
    GetCursorPos(&pos);
    SetForegroundWindow(m_hWnd);
    oPopup.TrackPopupMenu(TPM_LEFTALIGN, pos.x, pos.y, m_hWnd);
    // BUGFIX: See "PRB: Menus for Notification Icons Don't Work Correctly"
    PostMessage(WM_NULL);
    return 0;
}

LRESULT CFloatingWindow::OnTimer(UINT id)
{
    if (id == 1)
    {
        KillTimer(1);
        SendMessage(WM_COMMAND, MAKEWPARAM(Settings.Hotkeys[Settings.TrayIconSettings.LeftClickCommand].commandId, 0));
    }
    if (id == 2)
        EnableClicks = true;

    KillTimer(id);
    return 0;
}

inline BOOL SetOneInstance(LPCTSTR szName)
{
    HANDLE hMutex = NULL;
    BOOL bFound = FALSE;
    hMutex = ::CreateMutex(NULL, TRUE, szName);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
        bFound = TRUE;
    if (hMutex)
        ::ReleaseMutex(hMutex);
    return bFound;
}

CFloatingWindow floatWnd;

void CFloatingWindow::CreateTrayIcon()
{
    BOOL bFound = FALSE;
    hMutex = ::CreateMutex(NULL, TRUE, _T("ImageUploader_TrayWnd_Mutex"));
    if (GetLastError() == ERROR_ALREADY_EXISTS)
        bFound = TRUE;
    if (hMutex)
        ::ReleaseMutex(hMutex);

    if (!bFound)
    {
        CRect r(100, 100, 400, 400);
        floatWnd.Create(0, r, _T("ImageUploader_TrayWnd"), WS_OVERLAPPED | WS_POPUP | WS_CAPTION );
        floatWnd.ShowWindow(SW_HIDE);
    }
}

BOOL IsRunningFloatingWnd()
{
    HANDLE hMutex = NULL;
    BOOL bFound = FALSE;
    hMutex = ::CreateMutex(NULL, TRUE, _T("ImageUploader_TrayWnd_Mutex"));
    if (GetLastError() == ERROR_ALREADY_EXISTS)
        bFound = TRUE;
    if (hMutex)
    {
        ::ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
    return bFound;
}

void CFloatingWindow::RegisterHotkeys()
{
    m_hotkeys = Settings.Hotkeys;

    for (size_t i = 0; i < m_hotkeys.size(); i++)
    {
        if (m_hotkeys[i].globalKey.keyCode)
        {
            if (!RegisterHotKey(m_hWnd, i, m_hotkeys[i].globalKey.keyModifier, m_hotkeys[i].globalKey.keyCode))
            {
                CString msg;
                msg.Format(TR("���������� ���������������� ���������� ��������� ������\n%s.\n ��������, ��� ������ ������ ����������."),
                           (LPCTSTR)m_hotkeys[i].globalKey.toString());
                ServiceLocator::instance()->logger()->write(logWarning, _T("Hotkeys"), msg);
            }
        }
    }
}

LRESULT CFloatingWindow::OnHotKey(int HotKeyID, UINT flags, UINT vk)
{
    if (HotKeyID < 0 || HotKeyID > int(m_hotkeys.size()) - 1)
        return 0;
    if (m_bIsUploading)
        return 0;

    if (m_hotkeys[HotKeyID].func == _T("windowscreenshot"))
    {
        wizardDlg_->executeFunc(_T("windowscreenshot,1"));
    }
    else
    {
        m_bFromHotkey = true;
        SetActiveWindow();
        SetForegroundWindow(m_hWnd);
        SendMessage(WM_COMMAND, MAKEWPARAM(m_hotkeys[HotKeyID].commandId, 0));
        m_bFromHotkey = false;
    }
    return 0;
}

void CFloatingWindow::UnRegisterHotkeys()
{
    for (size_t i = 0; i < m_hotkeys.size(); i++)
    {
        if (m_hotkeys[i].globalKey.keyCode)
            UnregisterHotKey(m_hWnd, i);
    }
    m_hotkeys.clear();
}

LRESULT CFloatingWindow::OnPaste(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (wizardDlg_->executeFunc(_T("paste")))
        wizardDlg_->ShowWindow(SW_SHOW);
    return 0;
}

LRESULT CFloatingWindow::OnMediaInfo(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (wizardDlg_->executeFunc(_T("mediainfo")))
        wizardDlg_->ShowWindow(SW_SHOW);
    return 0;
}

LRESULT CFloatingWindow::OnTaskbarCreated(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    InstallIcon(APPNAME, m_hIconSmall, 0);
    return 0;
}

LRESULT CFloatingWindow::OnScreenshotActionChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    Settings.TrayIconSettings.TrayScreenshotAction = wID - IDM_SCREENTSHOTACTION_UPLOAD;
    Settings.SaveSettings();
    return 0;
}


void CFloatingWindow::ShowBaloonTip(const CString& text, const CString& title)
{
    NOTIFYICONDATA nid;
    ZeroMemory(&nid, sizeof(nid));
    nid.cbSize = NOTIFYICONDATA_V2_SIZE;
    nid.hWnd = m_hWnd;
    nid.uTimeout = 5500;
    nid.uFlags = NIF_INFO;
    nid.dwInfoFlags = NIIF_INFO;
    lstrcpyn(nid.szInfo, text, ARRAY_SIZE(nid.szInfo) - 1);
    lstrcpyn(nid.szInfoTitle, title, ARRAY_SIZE(nid.szInfoTitle) - 1);
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void CFloatingWindow::UploadScreenshot(const CString& realName, const CString& displayName)
{
    FileUploadTask *  task(new FileUploadTask(IuCoreUtils::WstringToUtf8((LPCTSTR)realName), IuCoreUtils::WstringToUtf8((LPCTSTR)displayName)));
    //std::shared_ptr<UploadSession> uploadSession(new UploadSession());
    task->setServerProfile(Settings.quickScreenshotServer);
    task->addTaskFinishedCallback(UploadTask::TaskFinishedCallback(this, &CFloatingWindow::OnFileFinished));
    task->setUrlShorteningServer(Settings.urlShorteningServer);
    //uploadSession->
    //uploadSession-

    uploadManager_->addTask(std::shared_ptr<UploadTask>(task));
    uploadManager_->start();

    CString msg;
    msg.Format(TR("���� �������� \"%s\" �� ������ %s"), (LPCTSTR) GetOnlyFileName(displayName),
        (LPCTSTR)Utf8ToWstring(Settings.quickScreenshotServer.serverName()).c_str());
    ShowBaloonTip(msg, TR("�������� ������"));
}

void CFloatingWindow::setUploadManager(UploadManager* manager)
{
    uploadManager_ = manager;
}

void CFloatingWindow::setUploadEngineManager(UploadEngineManager* manager)
{
    uploadEngineManager_ = manager;
}

/*
bool CFloatingWindow::OnQueueFinished(CFileQueueUploader*) {
    m_bIsUploading = false;
    bool usedDirectLink = true;

    if ( uploadType_ == utImage ) {
        CString url;
        if ((Settings.UseDirectLinks || lastUploadedItem_.fileListItem.downloadUrl.empty()) && !lastUploadedItem_.fileListItem.imageUrl.empty() )
            url = Utf8ToWstring(lastUploadedItem_.fileListItem.imageUrl).c_str();
        else if ((!Settings.UseDirectLinks || lastUploadedItem_.fileListItem.imageUrl.empty()) && !lastUploadedItem_.fileListItem.downloadUrl.empty() ) {
            url = Utf8ToWstring(lastUploadedItem_.fileListItem.downloadUrl).c_str();
            usedDirectLink = false;
        }


        if (url.IsEmpty())
        {
            ShowBaloonTip(TR("�� ������� ��������� ������ :("), _T("Image Uploader"));
            return true;
        }

        CHistoryManager* mgr = ZBase::get()->historyManager();
        std::shared_ptr<CHistorySession> session = mgr->newSession();
        HistoryItem hi;
        hi.localFilePath = source_file_name_;
        hi.serverName = server_name_;
        hi.directUrl =  (lastUploadedItem_.fileListItem.imageUrl);
        hi.thumbUrl = (lastUploadedItem_.fileListItem.thumbUrl);
        hi.viewUrl = (lastUploadedItem_.fileListItem.downloadUrl);
        hi.uploadFileSize = lastUploadedItem_.fileListItem.fileSize; // IuCoreUtils::getFileSize(WCstringToUtf8(ImageFileName));
        session->AddItem(hi);

        if ( Settings.TrayIconSettings.ShortenLinks ) {
            std::shared_ptr<UrlShorteningTask> task(new UrlShorteningTask(WCstringToUtf8(url)));

            CUploadEngineData *ue = Settings.urlShorteningServer.uploadEngineData();
            if ( !ue ) {
                ShowImageUploadedMessage(url);
                return false;

            }
            CAbstractUploadEngine * e = _EngineList->getUploadEngine(ue,Settings.urlShorteningServer.serverSettings());
            if ( !e ) {
                ShowImageUploadedMessage(url);
                return false;
            }
            e->setUploadData(ue);
            ServerSettingsStruct& settings = Settings.urlShorteningServer.serverSettings();
            e->setServerSettings(settings);
            e->setUploadData(ue);
            uploadType_ = utShorteningImageUrl;
            UploadTaskUserData* uploadTaskUserData = new UploadTaskUserData;
            uploadTaskUserData->linkTypeToShorten = usedDirectLink ? _T("ImageUrl") : _T("DownloadUrl");
            m_FileQueueUploader->AddUploadTask(task, reinterpret_cast<UploadTaskUserData*>(uploadTaskUserData), e);
            m_FileQueueUploader->start();
        } else {
            ShowImageUploadedMessage(url);
        }
    }
    return true;
}
*/
void CFloatingWindow::OnFileFinished(UploadTask* task, bool ok)
{
    if (task->type() == UploadTask::TypeUrl ) {
        if ( ok ) {
            CString url = Utf8ToWCstring(task->uploadResult()->directUrl);
            IU_CopyTextToClipboard(url);
            ShowBaloonTip( TrimString(url, 70) + CString("\r\n")
                + TR("(����� ��� ������������� ������� � ����� ������)"), TR("�������� ������"));
        } else {
            ShowBaloonTip( TR("��� ������������ �������� ���."), TR("�� ������� ��������� ������...") );
        }
    } else {
        CString url;
        UploadResult* uploadResult = task->uploadResult();
        bool usedDirectLink = true;
        if ((Settings.UseDirectLinks || uploadResult->downloadUrl.empty()) && !uploadResult->directUrl.empty()) {

            url = Utf8ToWstring(!uploadResult->directUrlShortened.empty() ? uploadResult->directUrlShortened: uploadResult->directUrl).c_str();
        }
        else if ((!Settings.UseDirectLinks || uploadResult->directUrl.empty()) && !uploadResult->downloadUrl.empty()) {
            url = Utf8ToWstring(!uploadResult->downloadUrlShortened.empty() ? uploadResult->downloadUrlShortened : uploadResult->downloadUrl).c_str();
            usedDirectLink = false;
        }
        lastUploadedItem_ = task;
        ShowImageUploadedMessage(url);
    }
    return;
}

LRESULT CFloatingWindow::OnStopUpload(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (uploadManager_)
        uploadManager_->stop();
    return 0;
}

void CFloatingWindow::ShowImageUploadedMessage(const CString& url) {
    IU_CopyTextToClipboard(url);
    ShowBaloonTip(TrimString(url, 70) + CString("\r\n") 
        + TR("(����� ��� ������������� ������� � ����� ������)")+ + CString("\r\n") + TR("������� �� ��� ��������� ��� �������� ���� � �����...") , TR("������ ������� ��������"));
}