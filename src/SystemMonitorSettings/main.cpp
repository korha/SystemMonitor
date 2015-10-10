#include <cstdio>
#include <windows.h>
#include <commctrl.h>
#include <cassert>

static const wchar_t *const g_wGuidClass = L"App::2e5f0c91-fb12-ac77-869f-dfb342870253",
*const g_wGuidClassMainApp = L"App::ec19dd9c-939a-4b05-c2fb-309ff78bcf32",
#ifdef _WIN64
*const g_wMainApp = L"SystemMonitor64.ini";
#else
*const g_wMainApp = L"SystemMonitor32.ini";
#endif

struct TagCreateParams
{
    LOGFONT *pLogFont;
    wchar_t *pBufPath,
    *pDelimPath;
    COLORREF *pColorRefCust;
};

//-------------------------------------------------------------------------------------------------
inline bool fOk(const wchar_t *const wOk)
{
    return !(*wOk || errno);
}

//-------------------------------------------------------------------------------------------------
void fRunMainApp(wchar_t *const pBufPath, wchar_t *const pDelimPath)
{
    if (FindWindow(g_wGuidClassMainApp, 0))
    {
        PROCESS_INFORMATION pi;
        STARTUPINFO si;
        memset(&si, 0, sizeof(STARTUPINFO));
        si.cb = sizeof(STARTUPINFO);
        wcscpy(pDelimPath, L"exe");
        if (CreateProcess(0, pBufPath, 0, 0, FALSE, CREATE_UNICODE_ENVIRONMENT, 0, 0, &si, &pi))
        {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
        wcscpy(pDelimPath, L"ini");
    }
}

//-------------------------------------------------------------------------------------------------
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    enum
    {
        eBtnRunApp = 1,
        eBtnColor,
        eBtnFont,
        eBtnRestoreDefaults,
        eBtnApply
    };

    static int iStgDx, iStgDy;
    static HWND hWndUpDownUpdateInterval,
            hWndUpDownOpacity,
            hWndEditRunApp,
            hWndUpDownThemeMargin,
            hWndUpDownThemeGap,
            hWndUpDownProgressbarWidth,
            hWndUpDownProgressbarHeight,
            hWndBtnColor,
            hWndUpDownLabelHeight;
    static HFONT hFont;
    static LOGFONT *pLogFont;
    static wchar_t *pBufPath,
            *pDelimPath;
    static COLORREF *pColorRefCust;

    switch (uMsg)
    {
    case WM_CREATE:
    {
        const CREATESTRUCT *const crStruct = reinterpret_cast<const CREATESTRUCT*>(lParam);
        const HINSTANCE hInst = crStruct->hInstance;
        const TagCreateParams *const tgCreateParams = static_cast<const TagCreateParams*>(crStruct->lpCreateParams);
        pLogFont = tgCreateParams->pLogFont;
        pBufPath = tgCreateParams->pBufPath;
        pDelimPath = tgCreateParams->pDelimPath;
        pColorRefCust = tgCreateParams->pColorRefCust;
        *pColorRefCust = RGB(6, 176, 37);
        for (int i = 1; i < 17; ++i)
            pColorRefCust[i] = RGB(255, 255, 255);
        if (const HWND hWndGbBase = CreateWindowEx(0, WC_BUTTON, L"Base", WS_CHILD | WS_VISIBLE | BS_GROUPBOX | BS_CENTER, 5, 5, 342, 87, hWnd, 0, hInst, 0))
            if (const HWND hWndUpdateInterval = CreateWindowEx(0, WC_STATIC, L"Update Interval (msecs):", WS_CHILD | WS_VISIBLE, 5, 21, 135, 13, hWndGbBase, 0, hInst, 0))
                if (const HWND hWndEditUpdateInterval = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, 0, WS_CHILD | WS_VISIBLE | ES_RIGHT | ES_NUMBER, 145, 18, 61, 20, hWndGbBase, 0, hInst, 0))
                    if ((hWndUpDownUpdateInterval = CreateWindowEx(0, UPDOWN_CLASS, 0, WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_AUTOBUDDY | UDS_ARROWKEYS | UDS_NOTHOUSANDS, 0, 0, 0, 0, hWndGbBase, 0, hInst, 0)))
                        if (const HWND hWndOpacity = CreateWindowEx(0, WC_STATIC, L"Opacity:", WS_CHILD | WS_VISIBLE, 5, 43, 135, 13, hWndGbBase, 0, hInst, 0))
                            if (const HWND hWndEditOpacity = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, 0, WS_CHILD | WS_VISIBLE | ES_RIGHT | ES_NUMBER, 145, 40, 61, 20, hWndGbBase, 0, hInst, 0))
                                if ((hWndUpDownOpacity = CreateWindowEx(0, UPDOWN_CLASS, 0, WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_AUTOBUDDY | UDS_ARROWKEYS | UDS_NOTHOUSANDS, 0, 0, 0, 0, hWndGbBase, 0, hInst, 0)))
                                    if (const HWND hWndRunApp = CreateWindowEx(0, WC_STATIC, L"Run Application:", WS_CHILD | WS_VISIBLE, 5, 65, 135, 13, hWndGbBase, 0, hInst, 0))
                                        if ((hWndEditRunApp = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, 0, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 145, 62, 170, 20, hWndGbBase, 0, hInst, 0)))
                                            if (const HWND hWndBtnRunApp = CreateWindowEx(0, WC_BUTTON, L"*", WS_CHILD | WS_VISIBLE | BS_CENTER, 321, 66, 22, 22, hWnd, reinterpret_cast<HMENU>(eBtnRunApp), hInst, 0))
                                                if (const HWND hWndGbTheme = CreateWindowEx(0, WC_BUTTON, L"Theme", WS_CHILD | WS_VISIBLE | BS_GROUPBOX | BS_CENTER, 5, 97, 342, 175, hWnd, 0, hInst, 0))
                                                    if (const HWND hWndThemeMargin = CreateWindowEx(0, WC_STATIC, L"Theme Margin:", WS_CHILD | WS_VISIBLE, 5, 21, 135, 13, hWndGbTheme, 0, hInst, 0))
                                                        if (const HWND hWndEditThemeMargin = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, 0, WS_CHILD | WS_VISIBLE | ES_RIGHT | ES_NUMBER, 145, 18, 61, 20, hWndGbTheme, 0, hInst, 0))
                                                            if ((hWndUpDownThemeMargin = CreateWindowEx(0, UPDOWN_CLASS, 0, WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_AUTOBUDDY | UDS_ARROWKEYS | UDS_NOTHOUSANDS, 0, 0, 0, 0, hWndGbTheme, 0, hInst, 0)))
                                                                if (const HWND hWndThemeGap = CreateWindowEx(0, WC_STATIC, L"Theme Gap:", WS_CHILD | WS_VISIBLE, 5, 43, 135, 13, hWndGbTheme, 0, hInst, 0))
                                                                    if (const HWND hWndEditThemeGap = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, 0, WS_CHILD | WS_VISIBLE | ES_RIGHT | ES_NUMBER, 145, 40, 61, 20, hWndGbTheme, 0, hInst, 0))
                                                                        if ((hWndUpDownThemeGap = CreateWindowEx(0, UPDOWN_CLASS, 0, WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_AUTOBUDDY | UDS_ARROWKEYS | UDS_NOTHOUSANDS, 0, 0, 0, 0, hWndGbTheme, 0, hInst, 0)))
                                                                            if (const HWND hWndProgressbarWidth = CreateWindowEx(0, WC_STATIC, L"Progressbar Width:", WS_CHILD | WS_VISIBLE, 5, 65, 135, 13, hWndGbTheme, 0, hInst, 0))
                                                                                if (const HWND hWndEditProgressbarWidth = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, 0, WS_CHILD | WS_VISIBLE | ES_RIGHT | ES_NUMBER, 145, 62, 61, 20, hWndGbTheme, 0, hInst, 0))
                                                                                    if ((hWndUpDownProgressbarWidth = CreateWindowEx(0, UPDOWN_CLASS, 0, WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_AUTOBUDDY | UDS_ARROWKEYS | UDS_NOTHOUSANDS, 0, 0, 0, 0, hWndGbTheme, 0, hInst, 0)))
                                                                                        if (const HWND hWndProgressbarHeight = CreateWindowEx(0, WC_STATIC, L"Progressbar Height:", WS_CHILD | WS_VISIBLE, 5, 87, 135, 13, hWndGbTheme, 0, hInst, 0))
                                                                                            if (const HWND hWndEditProgressbarHeight = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, 0, WS_CHILD | WS_VISIBLE | ES_RIGHT | ES_NUMBER, 145, 84, 61, 20, hWndGbTheme, 0, hInst, 0))
                                                                                                if ((hWndUpDownProgressbarHeight = CreateWindowEx(0, UPDOWN_CLASS, 0, WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_AUTOBUDDY | UDS_ARROWKEYS | UDS_NOTHOUSANDS, 0, 0, 0, 0, hWndGbTheme, 0, hInst, 0)))
                                                                                                    if (const HWND hWndProgressbarColor = CreateWindowEx(0, WC_STATIC, L"Progressbar Color:", WS_CHILD | WS_VISIBLE, 5, 109, 135, 13, hWndGbTheme, 0, hInst, 0))
                                                                                                        if ((hWndBtnColor = CreateWindowEx(0, WC_BUTTON, 0, WS_CHILD | WS_VISIBLE | BS_CENTER | BS_BITMAP, 149, 202, 22, 22, hWnd, reinterpret_cast<HMENU>(eBtnColor), hInst, 0)))
                                                                                                            if (const HWND hWndLabelHeight = CreateWindowEx(0, WC_STATIC, L"Label Height:", WS_CHILD | WS_VISIBLE, 5, 131, 135, 13, hWndGbTheme, 0, hInst, 0))
                                                                                                                if (const HWND hWndEditLabelHeight = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, 0, WS_CHILD | WS_VISIBLE | ES_RIGHT | ES_NUMBER, 145, 128, 61, 20, hWndGbTheme, 0, hInst, 0))
                                                                                                                    if ((hWndUpDownLabelHeight = CreateWindowEx(0, UPDOWN_CLASS, 0, WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_AUTOBUDDY | UDS_ARROWKEYS | UDS_NOTHOUSANDS, 0, 0, 0, 0, hWndGbTheme, 0, hInst, 0)))
                                                                                                                        if (const HWND hWndLabelFont = CreateWindowEx(0, WC_STATIC, L"Label Font:", WS_CHILD | WS_VISIBLE, 5, 153, 135, 13, hWndGbTheme, 0, hInst, 0))
                                                                                                                            if (const HWND hWndBtnFont = CreateWindowEx(0, WC_BUTTON, L"*", WS_CHILD | WS_VISIBLE | BS_CENTER, 149, 246, 22, 22, hWnd, reinterpret_cast<HMENU>(eBtnFont), hInst, 0))
                                                                                                                                if (const HWND hWndBtnRestoreDefault = CreateWindowEx(0, WC_BUTTON, L"Restore Defaults", WS_CHILD | WS_VISIBLE, 5, 282, 100, 23, hWnd, reinterpret_cast<HMENU>(eBtnRestoreDefaults), hInst, 0))
                                                                                                                                    if (const HWND hWndBtnApply = CreateWindowEx(0, WC_BUTTON, L"Apply", WS_CHILD | WS_VISIBLE, 149, 282, 75, 23, hWnd, reinterpret_cast<HMENU>(eBtnApply), hInst, 0))
                                                                                                                                    {
                                                                                                                                        NONCLIENTMETRICS nonClientMetrics;
                                                                                                                                        nonClientMetrics.cbSize = sizeof(NONCLIENTMETRICS);
                                                                                                                                        if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &nonClientMetrics, 0) && (hFont = CreateFontIndirect(&nonClientMetrics.lfMessageFont)))
                                                                                                                                        {
                                                                                                                                            SendMessage(hWndUpDownUpdateInterval, UDM_SETRANGE, 0, MAKELPARAM(30000, 1000));
                                                                                                                                            SendMessage(hWndUpDownUpdateInterval, UDM_SETPOS, 0, 5000);
                                                                                                                                            SendMessage(hWndUpDownOpacity, UDM_SETRANGE, 0, MAKELPARAM(255, 1));
                                                                                                                                            SendMessage(hWndUpDownOpacity, UDM_SETPOS, 0, 255);
                                                                                                                                            SetWindowText(hWndEditRunApp, L"taskmgr.exe");
                                                                                                                                            SendMessage(hWndUpDownThemeMargin, UDM_SETRANGE, 0, MAKELPARAM(1000, 0));
                                                                                                                                            SendMessage(hWndUpDownThemeMargin, UDM_SETPOS, 0, 5);
                                                                                                                                            SendMessage(hWndUpDownThemeGap, UDM_SETRANGE, 0, MAKELPARAM(1000, 0));
                                                                                                                                            SendMessage(hWndUpDownThemeGap, UDM_SETPOS, 0, 15);
                                                                                                                                            SendMessage(hWndUpDownProgressbarWidth, UDM_SETRANGE, 0, MAKELPARAM(10000, 80));
                                                                                                                                            SendMessage(hWndUpDownProgressbarWidth, UDM_SETPOS, 0, 150);
                                                                                                                                            SendMessage(hWndUpDownProgressbarHeight, UDM_SETRANGE, 0, MAKELPARAM(1000, 0));
                                                                                                                                            SendMessage(hWndUpDownProgressbarHeight, UDM_SETPOS, 0, 15);
                                                                                                                                            SendMessage(hWndUpDownLabelHeight, UDM_SETRANGE, 0, MAKELPARAM(1000, 0));
                                                                                                                                            SendMessage(hWndUpDownLabelHeight, UDM_SETPOS, 0, 13);

                                                                                                                                            SendMessage(hWndGbBase, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndUpdateInterval, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndEditUpdateInterval, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndUpDownUpdateInterval, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndOpacity, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndEditOpacity, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndUpDownOpacity, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndRunApp, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndEditRunApp, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndBtnRunApp, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndGbTheme, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndThemeMargin, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndEditThemeMargin, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndUpDownThemeMargin, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndThemeGap, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndEditThemeGap, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndUpDownThemeGap, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndProgressbarWidth, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndEditProgressbarWidth, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndUpDownProgressbarWidth, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndProgressbarHeight, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndEditProgressbarHeight, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndUpDownProgressbarHeight, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndProgressbarColor, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndBtnColor, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndLabelHeight, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndEditLabelHeight, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndUpDownLabelHeight, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndLabelFont, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndBtnFont, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndBtnRestoreDefault, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessage(hWndBtnApply, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);

                                                                                                                                            HFONT hFontNew = 0;
                                                                                                                                            if (FILE *file = _wfopen(pBufPath, L"rb"))
                                                                                                                                            {
                                                                                                                                                fseek(file, 0, SEEK_END);
                                                                                                                                                size_t szCount = ftell(file);
                                                                                                                                                if (szCount >= 16/*[Settings]nDx=1n*/ *sizeof(wchar_t) && szCount <= 1600*sizeof(wchar_t) && szCount%sizeof(wchar_t) == 0)
                                                                                                                                                    if (wchar_t *const wFile = static_cast<wchar_t*>(malloc(szCount + sizeof(wchar_t))))
                                                                                                                                                    {
                                                                                                                                                        rewind(file);
                                                                                                                                                        size_t szCountAll = fread(wFile, 1, szCount, file);
                                                                                                                                                        fclose(file);
                                                                                                                                                        file = 0;
                                                                                                                                                        if (szCount == szCountAll && wcsncmp(wFile, L"[Settings]\n", 11) == 0 && (szCountAll /= sizeof(wchar_t), wFile[szCountAll-1] == L'\n'))
                                                                                                                                                        {
                                                                                                                                                            wFile[szCountAll] = L'\0';
                                                                                                                                                            const wchar_t *const wFileFind = wFile+10;
                                                                                                                                                            wchar_t *pForLine = wcsstr(wFileFind, L"\nDx="),
                                                                                                                                                                    *pDelim,
                                                                                                                                                                    *wOk;
                                                                                                                                                            ULONG iValue;
                                                                                                                                                            if (pForLine)
                                                                                                                                                            {
                                                                                                                                                                pForLine += 4;
                                                                                                                                                                if ((pDelim = wcschr(pForLine, L'\n')))
                                                                                                                                                                {
                                                                                                                                                                    *pDelim = L'\0';
                                                                                                                                                                    iValue = wcstoul(pForLine, &wOk, 10);
                                                                                                                                                                    if (fOk(wOk) && iValue <= 0x7FFF)
                                                                                                                                                                        iStgDx = iValue;
                                                                                                                                                                    *pDelim = L'\n';
                                                                                                                                                                }
                                                                                                                                                            }
                                                                                                                                                            if ((pForLine = wcsstr(wFileFind, L"\nDy=")))
                                                                                                                                                            {
                                                                                                                                                                pForLine += 4;
                                                                                                                                                                if ((pDelim = wcschr(pForLine, L'\n')))
                                                                                                                                                                {
                                                                                                                                                                    *pDelim = L'\0';
                                                                                                                                                                    iValue = wcstoul(pForLine, &wOk, 10);
                                                                                                                                                                    if (fOk(wOk) && iValue <= 0x7FFF)
                                                                                                                                                                        iStgDy = iValue;
                                                                                                                                                                    *pDelim = L'\n';
                                                                                                                                                                }
                                                                                                                                                            }
                                                                                                                                                            if ((pForLine = wcsstr(wFileFind, L"\nUpdateInterval=")))
                                                                                                                                                            {
                                                                                                                                                                pForLine += 16;
                                                                                                                                                                if ((pDelim = wcschr(pForLine, L'\n')))
                                                                                                                                                                {
                                                                                                                                                                    *pDelim = L'\0';
                                                                                                                                                                    iValue = wcstoul(pForLine, &wOk, 10);
                                                                                                                                                                    if (fOk(wOk) && iValue >= 1000 && iValue <= 30000)
                                                                                                                                                                        SendMessage(hWndUpDownUpdateInterval, UDM_SETPOS, 0, iValue);
                                                                                                                                                                    *pDelim = L'\n';
                                                                                                                                                                }
                                                                                                                                                            }
                                                                                                                                                            if ((pForLine = wcsstr(wFileFind, L"\nOpacity=")))
                                                                                                                                                            {
                                                                                                                                                                pForLine += 9;
                                                                                                                                                                if ((pDelim = wcschr(pForLine, L'\n')))
                                                                                                                                                                {
                                                                                                                                                                    *pDelim = L'\0';
                                                                                                                                                                    iValue = wcstoul(pForLine, &wOk, 10);
                                                                                                                                                                    if (fOk(wOk) && iValue > 1 && iValue < 255)
                                                                                                                                                                        SendMessage(hWndUpDownOpacity, UDM_SETPOS, 0, iValue);
                                                                                                                                                                    *pDelim = L'\n';
                                                                                                                                                                }
                                                                                                                                                            }
                                                                                                                                                            if ((pForLine = wcsstr(wFileFind, L"\nRunApp=")))
                                                                                                                                                            {
                                                                                                                                                                pForLine += 8;
                                                                                                                                                                if ((pDelim = wcschr(pForLine, L'\n')))
                                                                                                                                                                {
                                                                                                                                                                    *pDelim = L'\0';
                                                                                                                                                                    SetWindowText(hWndEditRunApp, pForLine);
                                                                                                                                                                    *pDelim = L'\n';
                                                                                                                                                                }
                                                                                                                                                            }
                                                                                                                                                            if ((pForLine = wcsstr(wFileFind, L"\nThemeMargin=")))
                                                                                                                                                            {
                                                                                                                                                                pForLine += 13;
                                                                                                                                                                if ((pDelim = wcschr(pForLine, L'\n')))
                                                                                                                                                                {
                                                                                                                                                                    *pDelim = L'\0';
                                                                                                                                                                    iValue = wcstoul(pForLine, &wOk, 10);
                                                                                                                                                                    if (fOk(wOk) && iValue <= 1000)
                                                                                                                                                                        SendMessage(hWndUpDownThemeMargin, UDM_SETPOS, 0, iValue);
                                                                                                                                                                    *pDelim = L'\n';
                                                                                                                                                                }
                                                                                                                                                            }
                                                                                                                                                            if ((pForLine = wcsstr(wFileFind, L"\nThemeGap=")))
                                                                                                                                                            {
                                                                                                                                                                pForLine += 10;
                                                                                                                                                                if ((pDelim = wcschr(pForLine, L'\n')))
                                                                                                                                                                {
                                                                                                                                                                    *pDelim = L'\0';
                                                                                                                                                                    iValue = wcstoul(pForLine, &wOk, 10);
                                                                                                                                                                    if (fOk(wOk) && iValue <= 1000)
                                                                                                                                                                        SendMessage(hWndUpDownThemeGap, UDM_SETPOS, 0, iValue);
                                                                                                                                                                    *pDelim = L'\n';
                                                                                                                                                                }
                                                                                                                                                            }
                                                                                                                                                            if ((pForLine = wcsstr(wFileFind, L"\nProgressbarWidth=")))
                                                                                                                                                            {
                                                                                                                                                                pForLine += 18;
                                                                                                                                                                if ((pDelim = wcschr(pForLine, L'\n')))
                                                                                                                                                                {
                                                                                                                                                                    *pDelim = L'\0';
                                                                                                                                                                    iValue = wcstoul(pForLine, &wOk, 10);
                                                                                                                                                                    if (fOk(wOk) && iValue >= 80 && iValue <= 10000)
                                                                                                                                                                        SendMessage(hWndUpDownProgressbarWidth, UDM_SETPOS, 0, iValue);
                                                                                                                                                                    *pDelim = L'\n';
                                                                                                                                                                }
                                                                                                                                                            }
                                                                                                                                                            if ((pForLine = wcsstr(wFileFind, L"\nProgressbarHeight=")))
                                                                                                                                                            {
                                                                                                                                                                pForLine += 19;
                                                                                                                                                                if ((pDelim = wcschr(pForLine, L'\n')))
                                                                                                                                                                {
                                                                                                                                                                    *pDelim = L'\0';
                                                                                                                                                                    iValue = wcstoul(pForLine, &wOk, 10);
                                                                                                                                                                    if (fOk(wOk) && iValue <= 1000)
                                                                                                                                                                        SendMessage(hWndUpDownProgressbarHeight, UDM_SETPOS, 0, iValue);
                                                                                                                                                                    *pDelim = L'\n';
                                                                                                                                                                }
                                                                                                                                                            }
                                                                                                                                                            if ((pForLine = wcsstr(wFileFind, L"\nProgressbarColor=")))
                                                                                                                                                            {
                                                                                                                                                                pForLine += 18;
                                                                                                                                                                if ((pDelim = wcschr(pForLine, L'\n')) && pDelim-pForLine == 6/*rrggbb*/)
                                                                                                                                                                {
                                                                                                                                                                    *pDelim = L'\0';
                                                                                                                                                                    wchar_t wTemp = *pForLine;
                                                                                                                                                                    *pForLine = pForLine[4];
                                                                                                                                                                    pForLine[4] = wTemp;
                                                                                                                                                                    wTemp = pForLine[1];
                                                                                                                                                                    pForLine[1] = pForLine[5];
                                                                                                                                                                    pForLine[5] = wTemp;
                                                                                                                                                                    iValue = wcstoul(pForLine, &wOk, 16);
                                                                                                                                                                    if (fOk(wOk))
                                                                                                                                                                    {
                                                                                                                                                                        assert(iValue <= 0xFFFFFF);
                                                                                                                                                                        *pColorRefCust = iValue;
                                                                                                                                                                    }
                                                                                                                                                                    *pDelim = L'\n';
                                                                                                                                                                }
                                                                                                                                                            }
                                                                                                                                                            if ((pForLine = wcsstr(wFileFind, L"\nLabelHeight=")))
                                                                                                                                                            {
                                                                                                                                                                pForLine += 13;
                                                                                                                                                                if ((pDelim = wcschr(pForLine, L'\n')))
                                                                                                                                                                {
                                                                                                                                                                    *pDelim = L'\0';
                                                                                                                                                                    iValue = wcstoul(pForLine, &wOk, 10);
                                                                                                                                                                    if (fOk(wOk) && iValue <= 1000)
                                                                                                                                                                        SendMessage(hWndUpDownLabelHeight, UDM_SETPOS, 0, iValue);
                                                                                                                                                                    *pDelim = L'\n';
                                                                                                                                                                }
                                                                                                                                                            }
                                                                                                                                                            if ((pForLine = wcsstr(wFileFind, L"\nLabelFont=")))
                                                                                                                                                            {
                                                                                                                                                                enum eFont
                                                                                                                                                                {
                                                                                                                                                                    eHeight,
                                                                                                                                                                    eWidth,
                                                                                                                                                                    eEscapement,
                                                                                                                                                                    eOrientation,
                                                                                                                                                                    eWeight,
                                                                                                                                                                    eItalic,
                                                                                                                                                                    eUnderline,
                                                                                                                                                                    eStrikeOut,
                                                                                                                                                                    eCharSet,
                                                                                                                                                                    eOutPrecision,
                                                                                                                                                                    eClipPrecision,
                                                                                                                                                                    eQuality,
                                                                                                                                                                    ePitchAndFamily,
                                                                                                                                                                    eFaceName,
                                                                                                                                                                    eFontNum
                                                                                                                                                                };

                                                                                                                                                                pForLine += 11;
                                                                                                                                                                wchar_t *wFontArray[eFontNum];
                                                                                                                                                                wFontArray[0] = pForLine;
                                                                                                                                                                wFontArray[eFontNum-1] = 0;
                                                                                                                                                                for (int i = 1; i < 14; ++i)
                                                                                                                                                                {
                                                                                                                                                                    if (!(pForLine = wcschr(pForLine, L',')))
                                                                                                                                                                        break;
                                                                                                                                                                    *pForLine = L'\0';
                                                                                                                                                                    ++pForLine;
                                                                                                                                                                    wFontArray[i] = pForLine;
                                                                                                                                                                }
                                                                                                                                                                if (wFontArray[eFontNum-1] && (pDelim = wcschr(pForLine, L'\n'), pDelim - wFontArray[eFontNum-1] < LF_FACESIZE))
                                                                                                                                                                {
                                                                                                                                                                    LONG iFontArray[eFontNum-1];
                                                                                                                                                                    for (int i = 0; i < eFontNum-1; ++i)
                                                                                                                                                                    {
                                                                                                                                                                        iFontArray[i] = wcstol(wFontArray[i], &wOk, 10);
                                                                                                                                                                        if (!fOk(wOk))
                                                                                                                                                                        {
                                                                                                                                                                            wOk = 0;
                                                                                                                                                                            break;
                                                                                                                                                                        }
                                                                                                                                                                    }

                                                                                                                                                                    if (wOk && iFontArray[eWeight] >= 0 && iFontArray[eWeight] <= 1000 &&
                                                                                                                                                                            (iFontArray[eItalic] | 1) == 1 && (iFontArray[eUnderline] | 1) == 1 && (iFontArray[eStrikeOut] | 1) == 1 &&
                                                                                                                                                                            (iFontArray[eCharSet] | 0xFF) == 0xFF &&
                                                                                                                                                                            (iFontArray[eOutPrecision] | 0xFF) == 0xFF &&
                                                                                                                                                                            (iFontArray[eClipPrecision] | 0xFF) == 0xFF &&
                                                                                                                                                                            (iFontArray[eQuality] | 0xFF) == 0xFF &&
                                                                                                                                                                            (iFontArray[ePitchAndFamily] | 0xFF) == 0xFF)
                                                                                                                                                                    {
                                                                                                                                                                        pLogFont->lfHeight = iFontArray[eHeight];
                                                                                                                                                                        pLogFont->lfWidth = iFontArray[eWidth];
                                                                                                                                                                        pLogFont->lfEscapement = iFontArray[eEscapement];
                                                                                                                                                                        pLogFont->lfOrientation = iFontArray[eOrientation];
                                                                                                                                                                        pLogFont->lfWeight = iFontArray[eWeight];
                                                                                                                                                                        pLogFont->lfItalic = iFontArray[eItalic];
                                                                                                                                                                        pLogFont->lfUnderline = iFontArray[eUnderline];
                                                                                                                                                                        pLogFont->lfStrikeOut = iFontArray[eStrikeOut];
                                                                                                                                                                        pLogFont->lfCharSet = iFontArray[eCharSet];
                                                                                                                                                                        pLogFont->lfOutPrecision = iFontArray[eOutPrecision];
                                                                                                                                                                        pLogFont->lfClipPrecision = iFontArray[eClipPrecision];
                                                                                                                                                                        pLogFont->lfQuality = iFontArray[eQuality];
                                                                                                                                                                        pLogFont->lfPitchAndFamily = iFontArray[ePitchAndFamily];
                                                                                                                                                                        *pDelim = L'\0';
                                                                                                                                                                        wcscpy(pLogFont->lfFaceName, wFontArray[eFaceName]);
                                                                                                                                                                        hFontNew = CreateFontIndirect(pLogFont);
                                                                                                                                                                    }
                                                                                                                                                                }
                                                                                                                                                            }
                                                                                                                                                        }
                                                                                                                                                        free(wFile);
                                                                                                                                                    }
                                                                                                                                                if (file)
                                                                                                                                                    fclose(file);
                                                                                                                                            }
                                                                                                                                            if (hFontNew)
                                                                                                                                                DeleteObject(hFontNew);
                                                                                                                                            else
                                                                                                                                                memcpy(pLogFont, &nonClientMetrics.lfMessageFont, sizeof(LOGFONT));

                                                                                                                                            const COLORREF colorRefNew = ((*pColorRefCust & 0xFF) << 16) | (*pColorRefCust & 0xFF00) | ((*pColorRefCust >> 16) & 0xFF);        //BGR->RGB
                                                                                                                                            for (int i = 1+16; i < 1+16+100; ++i)
                                                                                                                                                pColorRefCust[i] = colorRefNew;
                                                                                                                                            if (const HBITMAP hBitmapColor = CreateBitmap(10, 10, 1, 32, pColorRefCust+1+16))
                                                                                                                                            {
                                                                                                                                                SendMessage(hWndBtnColor, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(hBitmapColor));
                                                                                                                                                DeleteObject(hBitmapColor);
                                                                                                                                            }
                                                                                                                                            return 0;
                                                                                                                                        }
                                                                                                                                    }
        return -1;
    }
    case WM_COMMAND:
    {
        switch (wParam)
        {
        case eBtnRunApp:
        {
            wchar_t wBuf[MAX_PATH+2] = L"\"";
            OPENFILENAME openFileName;
            memset(&openFileName, 0, sizeof(OPENFILENAME));
            openFileName.lStructSize = sizeof(OPENFILENAME);
            openFileName.hwndOwner = hWnd;
            openFileName.lpstrFilter = L"Executable (*.exe)\0*.exe\0All (*.*)\0*.*\0";
            openFileName.nFilterIndex = 1;
            openFileName.lpstrFile = wBuf+1;
            openFileName.nMaxFile = MAX_PATH;
            openFileName.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
            if (GetOpenFileName(&openFileName))
            {
                if (wchar_t *const pDelim = wcschr(wBuf+1, L' '))
                {
                    wcscat(pDelim, L"\"");
                    SetWindowText(hWndEditRunApp, wBuf);
                }
                else
                    SetWindowText(hWndEditRunApp, wBuf+1);
            }
            break;
        }
        case eBtnColor:
        {
            CHOOSECOLOR chooseColor;
            memset(&chooseColor, 0, sizeof(CHOOSECOLOR));
            chooseColor.lStructSize = sizeof(CHOOSECOLOR);
            chooseColor.hwndOwner = hWnd;
            chooseColor.rgbResult = *pColorRefCust;
            chooseColor.lpCustColors = pColorRefCust+1;
            chooseColor.Flags = CC_FULLOPEN | CC_RGBINIT;
            if (ChooseColor(&chooseColor))
            {
                *pColorRefCust = chooseColor.rgbResult;
                const COLORREF colorRefNew = ((*pColorRefCust & 0xFF) << 16) | (*pColorRefCust & 0xFF00) | ((*pColorRefCust >> 16) & 0xFF);        //BGR->RGB
                for (int i = 1+16; i < 1+16+100; ++i)
                    pColorRefCust[i] = colorRefNew;
                if (const HBITMAP hBitmapColor = CreateBitmap(10, 10, 1, 32, pColorRefCust+1+16))
                {
                    SendMessage(hWndBtnColor, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(hBitmapColor));
                    DeleteObject(hBitmapColor);
                }
            }
            break;
        }
        case eBtnFont:
        {
            CHOOSEFONT chooseFont;
            memset(&chooseFont, 0, sizeof(CHOOSEFONT));
            chooseFont.lStructSize = sizeof(CHOOSEFONT);
            chooseFont.hwndOwner = hWnd;
            chooseFont.lpLogFont = pLogFont;
            chooseFont.Flags = CF_SCREENFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT;
            ChooseFont(&chooseFont);
            break;
        }
        case eBtnRestoreDefaults:
        {
            if (GetFileAttributes(pBufPath) == INVALID_FILE_ATTRIBUTES || DeleteFile(pBufPath))
            {
                fRunMainApp(pBufPath, pDelimPath);
                PostMessage(hWnd, WM_CLOSE, 0, 0);
            }
            break;
        }
        case eBtnApply:
        {
            if (FILE *file = _wfopen(pBufPath, L"rb"))
            {
                fseek(file, 0, SEEK_END);
                const size_t szCount = ftell(file);
                if (szCount >= 16/*[Settings]nDx=1n*/*sizeof(wchar_t) && szCount <= 1600*sizeof(wchar_t) && szCount%sizeof(wchar_t) == 0)
                    if (wchar_t *const wFile = static_cast<wchar_t*>(malloc(szCount + sizeof(wchar_t))))
                    {
                        rewind(file);
                        size_t szCountAll = fread(wFile, 1, szCount, file);
                        fclose(file);
                        file = 0;
                        if (szCount == szCountAll && wcsncmp(wFile, L"[Settings]\n", 11) == 0 && (szCountAll /= sizeof(wchar_t), wFile[szCountAll-1] == L'\n'))
                        {
                            wFile[szCountAll] = L'\0';
                            const wchar_t *const wFileFind = wFile+10;
                            wchar_t *pForLine = wcsstr(wFileFind, L"\nDx="),
                                    *pDelim,
                                    *wOk;
                            ULONG iValue;
                            if (pForLine)
                            {
                                pForLine += 4;
                                if ((pDelim = wcschr(pForLine, L'\n')))
                                {
                                    *pDelim = L'\0';
                                    iValue = wcstoul(pForLine, &wOk, 10);
                                    if (fOk(wOk) && iValue <= 0x7FFF)
                                        iStgDx = iValue;
                                    *pDelim = L'\n';
                                }
                            }
                            if ((pForLine = wcsstr(wFileFind, L"\nDy=")))
                            {
                                pForLine += 4;
                                if ((pDelim = wcschr(pForLine, L'\n')))
                                {
                                    *pDelim = L'\0';
                                    iValue = wcstoul(pForLine, &wOk, 10);
                                    if (fOk(wOk) && iValue <= 0x7FFF)
                                        iStgDy = iValue;
                                    //*pDelim = L'\n';
                                }
                            }
                        }
                        free(wFile);
                    }
                if (file)
                    fclose(file);
            }

            bool bOk = false;
            LRESULT iAppLen = SendMessage(hWndEditRunApp, EM_LINELENGTH, 0, 0);
            if (iAppLen || (iAppLen = 11, SetWindowText(hWndEditRunApp, L"taskmgr.exe")))        //***false
            {
                //[Settings]n
                //Dx=32767n
                //Dy=32767n
                //UpdateInterval=30000n
                //Opacity=255n
                //RunApp=n
                //ThemeMargin=100n
                //ThemeGap=100n
                //ProgressbarWidth=100n
                //ProgressbarHeight=100n
                //ProgressbarColor=d8d8d8n
                //LabelHeight=100n
                //LabelFont=-2147483648,-2147483648,-2147483648,-2147483648,1000,1,1,1,255,255,255,255,255,31n
                if (void *const pSave = malloc((274+iAppLen)*sizeof(wchar_t)))
                {
                    wchar_t *pForLine = static_cast<wchar_t*>(pSave);
                    wcscpy(pForLine, L"[Settings]\nDx=");
                    pForLine += 14;
                    _ltow(iStgDx, pForLine, 10);
                    pForLine = wcschr(pForLine, L'\0');
                    wcscpy(pForLine, L"\nDy=");
                    pForLine += 4;
                    _ltow(iStgDy, pForLine, 10);
                    LRESULT iValue = SendMessage(hWndUpDownUpdateInterval, UDM_GETPOS, 0, 0);
                    if (HIWORD(iValue) == 0)
                    {
                        pForLine = wcschr(pForLine, L'\0');
                        wcscpy(pForLine, L"\nUpdateInterval=");
                        pForLine += 16;
                        _ltow(iValue, pForLine, 10);
                        iValue = SendMessage(hWndUpDownOpacity, UDM_GETPOS, 0, 0);
                        if (HIWORD(iValue) == 0)
                        {
                            pForLine = wcschr(pForLine, L'\0');
                            wcscpy(pForLine, L"\nOpacity=");
                            pForLine += 9;
                            _ltow(iValue, pForLine, 10);
                            pForLine = wcschr(pForLine, L'\0');
                            wcscpy(pForLine, L"\nRunApp=");
                            pForLine += 8;
                            *static_cast<WORD*>(static_cast<void*>(pForLine)) = iAppLen+1;        //first word contains buffer size (+1 to check trim)
                            if (SendMessage(hWndEditRunApp, EM_GETLINE, 0, reinterpret_cast<LPARAM>(pForLine)) == iAppLen)
                            {
                                iValue = SendMessage(hWndUpDownThemeMargin, UDM_GETPOS, 0, 0);
                                if (HIWORD(iValue) == 0)
                                {
                                    pForLine += iAppLen;
                                    wcscpy(pForLine, L"\nThemeMargin=");
                                    pForLine += 13;
                                    _ltow(iValue, pForLine, 10);
                                    iValue = SendMessage(hWndUpDownThemeGap, UDM_GETPOS, 0, 0);
                                    if (HIWORD(iValue) == 0)
                                    {
                                        pForLine = wcschr(pForLine, L'\0');
                                        wcscpy(pForLine, L"\nThemeGap=");
                                        pForLine += 10;
                                        _ltow(iValue, pForLine, 10);
                                        iValue = SendMessage(hWndUpDownProgressbarWidth, UDM_GETPOS, 0, 0);
                                        if (HIWORD(iValue) == 0)
                                        {
                                            pForLine = wcschr(pForLine, L'\0');
                                            wcscpy(pForLine, L"\nProgressbarWidth=");
                                            pForLine += 18;
                                            _ltow(iValue, pForLine, 10);
                                            iValue = SendMessage(hWndUpDownProgressbarHeight, UDM_GETPOS, 0, 0);
                                            if (HIWORD(iValue) == 0)
                                            {
                                                pForLine = wcschr(pForLine, L'\0');
                                                wcscpy(pForLine, L"\nProgressbarHeight=");
                                                pForLine += 19;
                                                _ltow(iValue, pForLine, 10);
                                                iValue = SendMessage(hWndUpDownLabelHeight, UDM_GETPOS, 0, 0);
                                                if (HIWORD(iValue) == 0)
                                                {
                                                    pForLine = wcschr(pForLine, L'\0');
                                                    wcscpy(pForLine, L"\nProgressbarColor=");
                                                    pForLine += 18;
                                                    const char *const cHex = "0123456789abcdef";
                                                    *pForLine   = cHex[(*pColorRefCust >>  4) & 0xF];//R1
                                                    *++pForLine = cHex[(*pColorRefCust >>  0) & 0xF];//R2
                                                    *++pForLine = cHex[(*pColorRefCust >> 12) & 0xF];//G1
                                                    *++pForLine = cHex[(*pColorRefCust >>  8) & 0xF];//G2
                                                    *++pForLine = cHex[(*pColorRefCust >> 20) & 0xF];//B1
                                                    *++pForLine = cHex[(*pColorRefCust >> 16) & 0xF];//B2
                                                    ++pForLine;
                                                    wcscpy(pForLine, L"\nLabelHeight=");
                                                    pForLine += 13;
                                                    _ltow(iValue, pForLine, 10);
                                                    pForLine = wcschr(pForLine, L'\0');
                                                    wcscpy(pForLine, L"\nLabelFont=");
                                                    pForLine += 11;
                                                    _ltow(pLogFont->lfHeight, pForLine, 10);
                                                    *(pForLine = wcschr(pForLine, L'\0')) = L',';
                                                    ++pForLine;
                                                    _ltow(pLogFont->lfWidth, pForLine, 10);
                                                    *(pForLine = wcschr(pForLine, L'\0')) = L',';
                                                    ++pForLine;
                                                    _ltow(pLogFont->lfEscapement, pForLine, 10);
                                                    *(pForLine = wcschr(pForLine, L'\0')) = L',';
                                                    ++pForLine;
                                                    _ltow(pLogFont->lfOrientation, pForLine, 10);
                                                    *(pForLine = wcschr(pForLine, L'\0')) = L',';
                                                    ++pForLine;
                                                    _ltow(pLogFont->lfWeight, pForLine, 10);
                                                    *(pForLine = wcschr(pForLine, L'\0')) = L',';
                                                    ++pForLine;
                                                    *pForLine = pLogFont->lfItalic ? L'1' : L'0';
                                                    ++pForLine;
                                                    *pForLine = L',';
                                                    ++pForLine;
                                                    *pForLine = pLogFont->lfUnderline ? L'1' : L'0';
                                                    ++pForLine;
                                                    *pForLine = L',';
                                                    ++pForLine;
                                                    *pForLine = pLogFont->lfStrikeOut ? L'1' : L'0';
                                                    ++pForLine;
                                                    *pForLine = L',';
                                                    ++pForLine;
                                                    _ultow(pLogFont->lfCharSet, pForLine, 10);
                                                    *(pForLine = wcschr(pForLine, L'\0')) = L',';
                                                    ++pForLine;
                                                    _ultow(pLogFont->lfOutPrecision, pForLine, 10);
                                                    *(pForLine = wcschr(pForLine, L'\0')) = L',';
                                                    ++pForLine;
                                                    _ultow(pLogFont->lfClipPrecision, pForLine, 10);
                                                    *(pForLine = wcschr(pForLine, L'\0')) = L',';
                                                    ++pForLine;
                                                    _ultow(pLogFont->lfQuality, pForLine, 10);
                                                    *(pForLine = wcschr(pForLine, L'\0')) = L',';
                                                    ++pForLine;
                                                    _ultow(pLogFont->lfPitchAndFamily, pForLine, 10);
                                                    *(pForLine = wcschr(pForLine, L'\0')) = L',';
                                                    ++pForLine;
                                                    wcscpy(pForLine, pLogFont->lfFaceName);
                                                    pForLine = wcschr(pForLine, L'\0');
                                                    wcscpy(pForLine, L"\n");
                                                    ++pForLine;
                                                    if (FILE *const file = _wfopen(pBufPath, L"wb"))
                                                    {
                                                        fwrite(pSave, static_cast<const BYTE*>(static_cast<const void*>(pForLine)) - static_cast<const BYTE*>(pSave), 1, file);
                                                        fclose(file);
                                                        bOk = true;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    free(pSave);
                }
            }
            if (bOk)
                fRunMainApp(pBufPath, pDelimPath);
            else
                MessageBox(hWnd, L"Save settings failed", L"SystemMonitorSettings", MB_ICONEXCLAMATION);
            break;
        }
        }
        return 0;
    }
    case WM_DESTROY:
        if (hFont)
            DeleteObject(hFont);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//-------------------------------------------------------------------------------------------------
int main()
{
    if (const HWND hWnd = FindWindow(g_wGuidClass, 0))
        PostMessage(hWnd, WM_CLOSE, 0, 0);

    TagCreateParams tgCreateParams;
    wchar_t wBufPath[MAX_PATH+1];
    DWORD dwLen = GetModuleFileName(0, wBufPath, MAX_PATH+1);
    if (dwLen >= 8 && dwLen < MAX_PATH && (tgCreateParams.pDelimPath = wcsrchr(wBufPath, L'\\')) && tgCreateParams.pDelimPath <= wBufPath + MAX_PATH-1-20/*\SystemMonitor96.exe*/)
    {
        const HINSTANCE hInst = GetModuleHandle(0);
        WNDCLASSEX wndCl;
        wndCl.cbSize = sizeof(WNDCLASSEX);
        wndCl.style = 0;
        wndCl.lpfnWndProc = WindowProc;
        wndCl.cbClsExtra = 0;
        wndCl.cbWndExtra = 0;
        wndCl.hInstance = hInst;
        wndCl.hIcon = static_cast<HICON>(LoadImage(hInst, L"IDI_ICON1", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
        wndCl.hCursor = LoadCursor(0, IDC_ARROW);
        wndCl.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1);
        wndCl.lpszMenuName = 0;
        wndCl.lpszClassName = g_wGuidClass;
        wndCl.hIconSm = static_cast<HICON>(LoadImage(hInst, L"IDI_ICON1", IMAGE_ICON, 16, 16, 0));

        if (RegisterClassEx(&wndCl))
        {
            LOGFONT logFont;
            tgCreateParams.pLogFont = &logFont;        //***it's ok
            tgCreateParams.pBufPath = wBufPath;
            wcscpy(tgCreateParams.pDelimPath+1, g_wMainApp);
            tgCreateParams.pDelimPath += 17/*\SystemMonitor96.*/;
            COLORREF colorRefCust[1+16+100/*current+custom+image*/];
            tgCreateParams.pColorRefCust = colorRefCust;
            if (CreateWindowEx(0, g_wGuidClass, L"SystemMonitorSettings", (WS_OVERLAPPEDWINDOW | WS_VISIBLE) & ~(WS_MAXIMIZEBOX | WS_SIZEBOX), GetSystemMetrics(SM_CXSCREEN)/2-358/2, GetSystemMetrics(SM_CYSCREEN)/2-340/2, 358, 340, 0, 0, hInst, &tgCreateParams))
            {
                MSG msg;
                while (GetMessage(&msg, 0, 0, 0) > 0)
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
            UnregisterClass(g_wGuidClass, hInst);
        }
    }
    return 0;
}
