// This file was generated by WTL Dialog wizard 
// HotkeyEditor.h : Declaration of the CHotkeyEditor

#pragma once

#include "resource.h"       // main symbols
#include <atlddx.h>

// CHotkeyEditor
struct MYHOTKEY
{
	MYHOTKEY()
	{
		keyCode = 0;
		keyModifier = 0;
	}
	CString Serialize()
	{
		DWORD data = MAKELONG(keyCode, keyModifier);
		CString res;
		res.Format(_T("%i"), data);
		return res;
	}

	bool DeSerialize(const CString data)
	{
		DWORD key = _tstoi(data);
		keyCode = LOWORD(key);
		keyModifier = HIWORD(key);
		return true;
	}
	bool operator!=( const MYHOTKEY& c)
	{
		return (keyCode!=c.keyCode || keyModifier!=c.keyModifier);
	}

	CString getKeyName(UINT vk, BOOL fExtended)
    {
        LONG lScan = MapVirtualKey(vk, 0) << 16;

        // if it's an extended key, add the extended flag
        if (fExtended)
            lScan |= 0x01000000L;

        int nBufferLen = 64;
        //CString str;
		  TCHAR buf[20];
        int nLen;
        //do
        {
            //nBufferLen *= 2;
				//str.AllocBuffer(nBufferLen);
            nLen = ::GetKeyNameTextW(lScan, buf, sizeof(buf));
        }
        //w/hile (nLen == nBufferLen);
        return buf;//str.c_str();
    }

	ACCEL toAccel()
	{
		ACCEL result;
		result.fVirt =0;
		result.cmd = 0;
		if(keyModifier&MOD_ALT)result.fVirt|=FALT	;
		if(keyModifier&MOD_CONTROL)result.fVirt|=FCONTROL	;
		if(keyModifier&MOD_SHIFT)result.fVirt|=FSHIFT	;
result.fVirt|=FVIRTKEY	;
		//if(keyModifier&MOD_W;IN)result.fVirt|=MOD_WIN;
		result.key = keyCode;
		return result;

	}
    CString toString()
    {

        CString strKeyName;
		  WORD wCode = keyCode;
		  WORD wModifiers = keyModifier;
		if(!wCode) return strKeyName;
        if (wCode != 0 || wModifiers != 0)
        {

			  if (wModifiers & MOD_WIN)
            {
               // strKeyName += getKeyName(VK_MENU, FALSE);
					strKeyName += _T("Win");
                strKeyName += L"+";
            }
            if (wModifiers & MOD_CONTROL)
            {
                strKeyName += getKeyName(VK_CONTROL, FALSE);
                strKeyName += L"+";
            }

            if (wModifiers & MOD_SHIFT)
            {
                strKeyName += getKeyName(VK_SHIFT, FALSE);
                strKeyName += L"+";
            }

            if (wModifiers & MOD_ALT)
            {
                strKeyName += getKeyName(VK_MENU, FALSE);
                strKeyName += L"+";
            }

				

            strKeyName += getKeyName(wCode, wModifiers & HOTKEYF_EXT);

        }
        return strKeyName;
    }

		
			WORD keyCode;
			WORD keyModifier;
		
	
	
};

class CHotkeyItem
{
public:
	MYHOTKEY localKey;
	MYHOTKEY globalKey;
	CString func;
	DWORD commandId;
	CString name;
};
class CHotkeyEditor : 
	public CDialogImpl<CHotkeyEditor>,
	public CWinDataExchange<CHotkeyEditor>	
{
public:
	CHotkeyItem m_data;
	CHotkeyEditor();
	~CHotkeyEditor();
	enum { IDD = IDD_HOTKEYEDITOR };

    BEGIN_MSG_MAP(CHotkeyEditor)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
        COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
    END_MSG_MAP()

	BEGIN_DDX_MAP(CHotkeyEditor)
	END_DDX_MAP()
    // Handler prototypes:
    //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};


