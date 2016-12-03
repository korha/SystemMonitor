//SystemMonitorSettings
#define _WIN32_WINNT _WIN32_IE_WINBLUE
#include <windows.h>
#include <commctrl.h>

static constexpr const wchar_t *const g_wGuidClass = L"App::2e5f0c91-fb12-ac77-869f-dfb342870253";
static constexpr const wchar_t *const g_wGuidClassMainApp = L"App::ec19dd9c-939a-4b05-c2fb-309ff78bcf32";

//-------------------------------------------------------------------------------------------------
#ifdef NDEBUG
#define ___assert___(cond) do{static_cast<void>(sizeof(cond));}while(false)
#else
#define ___assert___(cond) do{if(!(cond)){int i=__LINE__;char h[]="RUNTIME ASSERTION. Line:           "; \
    if(i>=0){char *c=h+35;do{*--c=i%10+'0';i/=10;}while(i>0);}else{h[25]='?';} \
    if(MessageBoxA(nullptr,__FILE__,h,MB_ICONERROR|MB_OKCANCEL)==IDCANCEL)ExitProcess(0);}}while(false)
#endif

static void FZeroMemory(void *const pDst__, DWORD dwSize)
{
    BYTE *pDst = static_cast<BYTE*>(pDst__);
    while (dwSize--)
        *pDst++ = '\0';
}

static inline void FCopyMemory(void *const pDst__, const void *const pSrc__, DWORD dwSize)
{
    char *pDst = static_cast<char*>(pDst__);
    const char *pSrc = static_cast<const char*>(pSrc__);
    while (dwSize--)
        *pDst++ = *pSrc++;
}

static inline void FCopyMemoryW(wchar_t *pDst, const wchar_t *pSrc)
{
    while ((*pDst++ = *pSrc++));
}

static inline wchar_t * FStrChrW(wchar_t *pSrc, const wchar_t wChar)
{
    while (*pSrc && *pSrc != wChar)
        ++pSrc;
    return *pSrc == wChar ? pSrc : nullptr;
}

//-------------------------------------------------------------------------------------------------
struct TagSaveSettings
{
    DWORD dwMagic;
    LONG iDx,
    iDy;
    unsigned __int16 iUpdateInterval;
    BYTE iOpacity;
    unsigned __int16 iThemeMargin,
    iThemeGap,
    iProgressbarWidth,
    iProgressbarHeight;
    COLORREF colorRef;
    unsigned __int16 iLabelHeight;
    LOGFONT logFont;
    wchar_t wAppPath[1+MAX_PATH+1];
};

struct TagCreateParams
{
    wchar_t *pBufPath,
    *pDelimPath;
    COLORREF *pColorRefCust;
    TagSaveSettings *pTgSave;
};

//-------------------------------------------------------------------------------------------------
static void FRunMainApp(wchar_t *const pBufPath, wchar_t *const pDelimPath)
{
    if (FindWindowW(g_wGuidClassMainApp, nullptr))
    {
        PROCESS_INFORMATION pi;
        STARTUPINFO si;
        FZeroMemory(&si, sizeof(STARTUPINFO));
        si.cb = sizeof(STARTUPINFO);
        pDelimPath[0] = L'"';
        pDelimPath[1] = L'\0';
        if (CreateProcessW(nullptr, pBufPath-1/*"*/, nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT, nullptr, nullptr, &si, &pi))
        {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
        pDelimPath[0] = L'.';
        pDelimPath[1] = L'c';
    }
}

//-------------------------------------------------------------------------------------------------
static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    enum
    {
        eBtnRunApp = 1,
        eBtnColor,
        eBtnFont,
        eBtnReset,
        eBtnApply,
    };
    static constexpr const DWORD dwMagic = 0x29D6036C;

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
    static wchar_t *pBufPath,
            *pDelimPath;
    static COLORREF *pColorRefCust;
    static HBITMAP hBitmapColor;
    static TagSaveSettings *tgSave;

    switch (uMsg)
    {
    case WM_CREATE:
    {
        const CREATESTRUCT *const crStruct = reinterpret_cast<const CREATESTRUCT*>(lParam);
        const HINSTANCE hInst = crStruct->hInstance;
        const TagCreateParams *const tgCreateParams = static_cast<const TagCreateParams*>(crStruct->lpCreateParams);
        pBufPath = tgCreateParams->pBufPath;
        pDelimPath = tgCreateParams->pDelimPath;
        pColorRefCust = tgCreateParams->pColorRefCust;
        *pColorRefCust = RGB(6, 176, 37);
        for (int i = 1; i < 17; ++i)
            pColorRefCust[i] = RGB(255, 255, 255);
        tgSave = tgCreateParams->pTgSave;
        if (const HWND hWndGbBase = CreateWindowExW(0, WC_BUTTON, L"Base", WS_CHILD | WS_VISIBLE | BS_GROUPBOX | BS_CENTER, 5, 5, 348, 87, hWnd, nullptr, hInst, nullptr))
            if (const HWND hWndUpdateInterval = CreateWindowExW(0, WC_STATIC, L"Update Interval (msecs):", WS_CHILD | WS_VISIBLE, 5, 20, 135, 18, hWndGbBase, nullptr, hInst, nullptr))
                if (const HWND hWndEditUpdateInterval = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | ES_RIGHT | ES_NUMBER, 145, 18, 61, 20, hWndGbBase, nullptr, hInst, nullptr))
                    if ((hWndUpDownUpdateInterval = CreateWindowExW(0, UPDOWN_CLASS, nullptr, WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_AUTOBUDDY | UDS_ARROWKEYS | UDS_NOTHOUSANDS, 0, 0, 0, 0, hWndGbBase, nullptr, hInst, nullptr)))
                        if (const HWND hWndOpacity = CreateWindowExW(0, WC_STATIC, L"Opacity:", WS_CHILD | WS_VISIBLE, 5, 42, 135, 18, hWndGbBase, nullptr, hInst, nullptr))
                            if (const HWND hWndEditOpacity = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | ES_RIGHT | ES_NUMBER, 145, 40, 61, 20, hWndGbBase, nullptr, hInst, nullptr))
                                if ((hWndUpDownOpacity = CreateWindowExW(0, UPDOWN_CLASS, nullptr, WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_AUTOBUDDY | UDS_ARROWKEYS | UDS_NOTHOUSANDS, 0, 0, 0, 0, hWndGbBase, nullptr, hInst, nullptr)))
                                    if (const HWND hWndRunApp = CreateWindowExW(0, WC_STATIC, L"Run Application:", WS_CHILD | WS_VISIBLE, 5, 64, 135, 18, hWndGbBase, nullptr, hInst, nullptr))
                                        if ((hWndEditRunApp = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 145, 62, 170, 20, hWndGbBase, nullptr, hInst, nullptr)))
                                            if (const HWND hWndBtnRunApp = CreateWindowExW(0, WC_BUTTON, L"*", WS_CHILD | WS_VISIBLE, 322, 66, 26, 22, hWnd, reinterpret_cast<HMENU>(eBtnRunApp), hInst, nullptr))
                                                if (const HWND hWndGbTheme = CreateWindowExW(0, WC_BUTTON, L"Theme", WS_CHILD | WS_VISIBLE | BS_GROUPBOX | BS_CENTER, 5, 97, 348, 175, hWnd, nullptr, hInst, nullptr))
                                                    if (const HWND hWndThemeMargin = CreateWindowExW(0, WC_STATIC, L"Theme Margin:", WS_CHILD | WS_VISIBLE, 5, 20, 135, 18, hWndGbTheme, nullptr, hInst, nullptr))
                                                        if (const HWND hWndEditThemeMargin = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | ES_RIGHT | ES_NUMBER, 145, 18, 61, 20, hWndGbTheme, nullptr, hInst, nullptr))
                                                            if ((hWndUpDownThemeMargin = CreateWindowExW(0, UPDOWN_CLASS, nullptr, WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_AUTOBUDDY | UDS_ARROWKEYS | UDS_NOTHOUSANDS, 0, 0, 0, 0, hWndGbTheme, nullptr, hInst, nullptr)))
                                                                if (const HWND hWndThemeGap = CreateWindowExW(0, WC_STATIC, L"Theme Gap:", WS_CHILD | WS_VISIBLE, 5, 42, 135, 18, hWndGbTheme, nullptr, hInst, nullptr))
                                                                    if (const HWND hWndEditThemeGap = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | ES_RIGHT | ES_NUMBER, 145, 40, 61, 20, hWndGbTheme, nullptr, hInst, nullptr))
                                                                        if ((hWndUpDownThemeGap = CreateWindowExW(0, UPDOWN_CLASS, nullptr, WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_AUTOBUDDY | UDS_ARROWKEYS | UDS_NOTHOUSANDS, 0, 0, 0, 0, hWndGbTheme, nullptr, hInst, nullptr)))
                                                                            if (const HWND hWndProgressbarWidth = CreateWindowExW(0, WC_STATIC, L"Progressbar Width:", WS_CHILD | WS_VISIBLE, 5, 64, 135, 18, hWndGbTheme, nullptr, hInst, nullptr))
                                                                                if (const HWND hWndEditProgressbarWidth = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | ES_RIGHT | ES_NUMBER, 145, 62, 61, 20, hWndGbTheme, nullptr, hInst, nullptr))
                                                                                    if ((hWndUpDownProgressbarWidth = CreateWindowExW(0, UPDOWN_CLASS, nullptr, WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_AUTOBUDDY | UDS_ARROWKEYS | UDS_NOTHOUSANDS, 0, 0, 0, 0, hWndGbTheme, nullptr, hInst, nullptr)))
                                                                                        if (const HWND hWndProgressbarHeight = CreateWindowExW(0, WC_STATIC, L"Progressbar Height:", WS_CHILD | WS_VISIBLE, 5, 86, 135, 18, hWndGbTheme, nullptr, hInst, nullptr))
                                                                                            if (const HWND hWndEditProgressbarHeight = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | ES_RIGHT | ES_NUMBER, 145, 84, 61, 20, hWndGbTheme, nullptr, hInst, nullptr))
                                                                                                if ((hWndUpDownProgressbarHeight = CreateWindowExW(0, UPDOWN_CLASS, nullptr, WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_AUTOBUDDY | UDS_ARROWKEYS | UDS_NOTHOUSANDS, 0, 0, 0, 0, hWndGbTheme, nullptr, hInst, nullptr)))
                                                                                                    if (const HWND hWndProgressbarColor = CreateWindowExW(0, WC_STATIC, L"Progressbar Color:", WS_CHILD | WS_VISIBLE, 5, 108, 135, 18, hWndGbTheme, nullptr, hInst, nullptr))
                                                                                                        if ((hWndBtnColor = CreateWindowExW(0, WC_BUTTON, nullptr, WS_CHILD | WS_VISIBLE | BS_BITMAP, 150, 202, 26, 22, hWnd, reinterpret_cast<HMENU>(eBtnColor), hInst, nullptr)))
                                                                                                            if (const HWND hWndLabelHeight = CreateWindowExW(0, WC_STATIC, L"Label Height:", WS_CHILD | WS_VISIBLE, 5, 130, 135, 18, hWndGbTheme, nullptr, hInst, nullptr))
                                                                                                                if (const HWND hWndEditLabelHeight = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDIT, nullptr, WS_CHILD | WS_VISIBLE | ES_RIGHT | ES_NUMBER, 145, 128, 61, 20, hWndGbTheme, nullptr, hInst, nullptr))
                                                                                                                    if ((hWndUpDownLabelHeight = CreateWindowExW(0, UPDOWN_CLASS, nullptr, WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_AUTOBUDDY | UDS_ARROWKEYS | UDS_NOTHOUSANDS, 0, 0, 0, 0, hWndGbTheme, nullptr, hInst, nullptr)))
                                                                                                                        if (const HWND hWndLabelFont = CreateWindowExW(0, WC_STATIC, L"Label Font:", WS_CHILD | WS_VISIBLE, 5, 152, 135, 18, hWndGbTheme, nullptr, hInst, nullptr))
                                                                                                                            if (const HWND hWndBtnFont = CreateWindowExW(0, WC_BUTTON, L"*", WS_CHILD | WS_VISIBLE, 150, 246, 26, 22, hWnd, reinterpret_cast<HMENU>(eBtnFont), hInst, nullptr))
                                                                                                                                if (const HWND hWndBtnReset = CreateWindowExW(0, WC_BUTTON, L"Reset", WS_CHILD | WS_VISIBLE, 5, 282, 75, 22, hWnd, reinterpret_cast<HMENU>(eBtnReset), hInst, nullptr))
                                                                                                                                    if (const HWND hWndBtnApply = CreateWindowExW(0, WC_BUTTON, L"Apply", WS_CHILD | WS_VISIBLE, 150, 282, 75, 22, hWnd, reinterpret_cast<HMENU>(eBtnApply), hInst, nullptr))
                                                                                                                                    {
                                                                                                                                        NONCLIENTMETRICS nonClientMetrics;
                                                                                                                                        nonClientMetrics.cbSize = sizeof(NONCLIENTMETRICS);
                                                                                                                                        if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &nonClientMetrics, 0) && (hFont = CreateFontIndirect(&nonClientMetrics.lfMessageFont)))
                                                                                                                                        {
                                                                                                                                            SendMessageW(hWndUpDownUpdateInterval, UDM_SETRANGE, 0, MAKELPARAM(30000, 1000));
                                                                                                                                            SendMessageW(hWndUpDownUpdateInterval, UDM_SETPOS, 0, 5000);
                                                                                                                                            SendMessageW(hWndUpDownOpacity, UDM_SETRANGE, 0, MAKELPARAM(255, 1));
                                                                                                                                            SendMessageW(hWndUpDownOpacity, UDM_SETPOS, 0, 255);
                                                                                                                                            SendMessageW(hWndEditRunApp, EM_SETLIMITTEXT, MAX_PATH+2-1, 0);
                                                                                                                                            SetWindowTextW(hWndEditRunApp, L"taskmgr.exe");
                                                                                                                                            SendMessageW(hWndUpDownThemeMargin, UDM_SETRANGE, 0, MAKELPARAM(1000, 0));
                                                                                                                                            SendMessageW(hWndUpDownThemeMargin, UDM_SETPOS, 0, 5);
                                                                                                                                            SendMessageW(hWndUpDownThemeGap, UDM_SETRANGE, 0, MAKELPARAM(1000, 0));
                                                                                                                                            SendMessageW(hWndUpDownThemeGap, UDM_SETPOS, 0, 15);
                                                                                                                                            SendMessageW(hWndUpDownProgressbarWidth, UDM_SETRANGE, 0, MAKELPARAM(10000, 80));
                                                                                                                                            SendMessageW(hWndUpDownProgressbarWidth, UDM_SETPOS, 0, 150);
                                                                                                                                            SendMessageW(hWndUpDownProgressbarHeight, UDM_SETRANGE, 0, MAKELPARAM(1000, 0));
                                                                                                                                            SendMessageW(hWndUpDownProgressbarHeight, UDM_SETPOS, 0, 15);
                                                                                                                                            SendMessageW(hWndUpDownLabelHeight, UDM_SETRANGE, 0, MAKELPARAM(1000, 0));
                                                                                                                                            SendMessageW(hWndUpDownLabelHeight, UDM_SETPOS, 0, 13);

                                                                                                                                            SendMessageW(hWndGbBase, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndUpdateInterval, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndEditUpdateInterval, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndUpDownUpdateInterval, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndOpacity, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndEditOpacity, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndUpDownOpacity, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndRunApp, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndEditRunApp, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndBtnRunApp, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndGbTheme, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndThemeMargin, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndEditThemeMargin, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndUpDownThemeMargin, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndThemeGap, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndEditThemeGap, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndUpDownThemeGap, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndProgressbarWidth, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndEditProgressbarWidth, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndUpDownProgressbarWidth, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndProgressbarHeight, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndEditProgressbarHeight, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndUpDownProgressbarHeight, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndProgressbarColor, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndBtnColor, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndLabelHeight, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndEditLabelHeight, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndUpDownLabelHeight, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndLabelFont, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndBtnFont, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndBtnReset, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                            SendMessageW(hWndBtnApply, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);

                                                                                                                                            bool bOk = false;
                                                                                                                                            const HANDLE hFile = CreateFileW(pBufPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
                                                                                                                                            if (hFile != INVALID_HANDLE_VALUE)
                                                                                                                                            {
                                                                                                                                                DWORD dwBytes;
                                                                                                                                                LARGE_INTEGER iFileSize;
                                                                                                                                                if (GetFileSizeEx(hFile, &iFileSize) && iFileSize.QuadPart == sizeof(TagSaveSettings) &&
                                                                                                                                                        ReadFile(hFile, tgSave, sizeof(TagSaveSettings), &dwBytes, nullptr) && dwBytes == sizeof(TagSaveSettings))
                                                                                                                                                    bOk = true;
                                                                                                                                                CloseHandle(hFile);
                                                                                                                                            }

                                                                                                                                            HFONT hFontNew;
                                                                                                                                            if (bOk &&
                                                                                                                                                    tgSave->dwMagic == dwMagic &&
                                                                                                                                                    tgSave->iUpdateInterval >= 1000 && tgSave->iUpdateInterval <= 30000 &&
                                                                                                                                                    tgSave->iOpacity &&
                                                                                                                                                    tgSave->iThemeMargin <= 1000 &&
                                                                                                                                                    tgSave->iThemeGap <= 1000 &&
                                                                                                                                                    tgSave->iProgressbarWidth >= 80 && tgSave->iProgressbarWidth <= 10000 &&
                                                                                                                                                    tgSave->iProgressbarHeight <= 1000 &&
                                                                                                                                                    tgSave->iLabelHeight <= 1000 &&
                                                                                                                                                    tgSave->wAppPath[1+MAX_PATH] == L'\0' &&
                                                                                                                                                    (hFontNew = CreateFontIndirectW(&tgSave->logFont)))
                                                                                                                                            {
                                                                                                                                                DeleteObject(hFontNew);
                                                                                                                                                *pColorRefCust = tgSave->colorRef;
                                                                                                                                                SendMessageW(hWndUpDownUpdateInterval, UDM_SETPOS, 0, tgSave->iUpdateInterval);
                                                                                                                                                SendMessageW(hWndUpDownOpacity, UDM_SETPOS, 0, tgSave->iOpacity);
                                                                                                                                                SendMessageW(hWndUpDownThemeMargin, UDM_SETPOS, 0, tgSave->iThemeMargin);
                                                                                                                                                SendMessageW(hWndUpDownThemeGap, UDM_SETPOS, 0, tgSave->iThemeGap);
                                                                                                                                                SendMessageW(hWndUpDownProgressbarWidth, UDM_SETPOS, 0, tgSave->iProgressbarWidth);
                                                                                                                                                SendMessageW(hWndUpDownProgressbarHeight, UDM_SETPOS, 0, tgSave->iProgressbarHeight);
                                                                                                                                                SendMessageW(hWndUpDownLabelHeight, UDM_SETPOS, 0, tgSave->iLabelHeight);
                                                                                                                                                SetWindowTextW(hWndEditRunApp, tgSave->wAppPath);
                                                                                                                                            }
                                                                                                                                            else
                                                                                                                                            {
                                                                                                                                                FCopyMemory(&tgSave->logFont, &nonClientMetrics.lfMessageFont, sizeof(LOGFONT));
                                                                                                                                                *pColorRefCust = RGB(6, 176, 37);
                                                                                                                                                SendMessageW(hWndUpDownUpdateInterval, UDM_SETPOS, 0, 5000);
                                                                                                                                                SendMessageW(hWndUpDownOpacity, UDM_SETPOS, 0, 255);
                                                                                                                                                SendMessageW(hWndUpDownThemeMargin, UDM_SETPOS, 0, 5);
                                                                                                                                                SendMessageW(hWndUpDownThemeGap, UDM_SETPOS, 0, 15);
                                                                                                                                                SendMessageW(hWndUpDownProgressbarWidth, UDM_SETPOS, 0, 150);
                                                                                                                                                SendMessageW(hWndUpDownProgressbarHeight, UDM_SETPOS, 0, 15);
                                                                                                                                                SendMessageW(hWndUpDownLabelHeight, UDM_SETPOS, 0, 13);
                                                                                                                                            }

                                                                                                                                            const COLORREF colorRefNew = ((*pColorRefCust & 0xFF) << 16) | (*pColorRefCust & 0xFF00) | ((*pColorRefCust >> 16) & 0xFF);        //BGR->RGB
                                                                                                                                            for (int i = 1+16; i < 1+16+100; ++i)
                                                                                                                                                pColorRefCust[i] = colorRefNew;

                                                                                                                                            if ((hBitmapColor = CreateBitmap(10, 10, 1, 32, pColorRefCust+1+16)))
                                                                                                                                                SendMessageW(hWndBtnColor, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(hBitmapColor));
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
            tgSave->wAppPath[0] = L'"';
            tgSave->wAppPath[1] = L'\0';
            OPENFILENAME openFileName;
            FZeroMemory(&openFileName, sizeof(OPENFILENAME));
            openFileName.lStructSize = sizeof(OPENFILENAME);
            openFileName.hwndOwner = hWnd;
            openFileName.lpstrFilter = L"Executable (*.exe)\0*.exe\0All Files (*.*)\0*.*\0";
            openFileName.nFilterIndex = 1;
            openFileName.lpstrFile = tgSave->wAppPath+1;
            openFileName.nMaxFile = MAX_PATH;
            openFileName.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
            if (GetOpenFileNameW(&openFileName))
            {
                if (wchar_t *pDelim = FStrChrW(tgSave->wAppPath, L' '))
                {
                    *(pDelim = FStrChrW(pDelim, L'\0')) = L'"';
                    pDelim[1] = L'\0';
                    SetWindowTextW(hWndEditRunApp, tgSave->wAppPath);
                }
                else
                    SetWindowTextW(hWndEditRunApp, tgSave->wAppPath+1);
            }
            break;
        }
        case eBtnColor:
        {
            CHOOSECOLOR chooseColor;
            FZeroMemory(&chooseColor, sizeof(CHOOSECOLOR));
            chooseColor.lStructSize = sizeof(CHOOSECOLOR);
            chooseColor.hwndOwner = hWnd;
            chooseColor.rgbResult = *pColorRefCust;
            chooseColor.lpCustColors = pColorRefCust+1;
            chooseColor.Flags = CC_FULLOPEN | CC_RGBINIT;
            if (ChooseColorW(&chooseColor))
            {
                *pColorRefCust = chooseColor.rgbResult;
                const COLORREF colorRefNew = ((*pColorRefCust & 0xFF) << 16) | (*pColorRefCust & 0xFF00) | ((*pColorRefCust >> 16) & 0xFF);        //BGR->RGB
                for (int i = 1+16; i < 1+16+100; ++i)
                    pColorRefCust[i] = colorRefNew;
                DeleteObject(hBitmapColor);
                if ((hBitmapColor = CreateBitmap(10, 10, 1, 32, pColorRefCust+1+16)))
                    SendMessageW(hWndBtnColor, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(hBitmapColor));
            }
            break;
        }
        case eBtnFont:
        {
            CHOOSEFONT chooseFont;
            FZeroMemory(&chooseFont, sizeof(CHOOSEFONT));
            chooseFont.lStructSize = sizeof(CHOOSEFONT);
            chooseFont.hwndOwner = hWnd;
            chooseFont.lpLogFont = &tgSave->logFont;
            chooseFont.Flags = CF_SCREENFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT;
            ChooseFontW(&chooseFont);
            break;
        }
        case eBtnReset:
        {
            if (GetFileAttributesW(pBufPath) == INVALID_FILE_ATTRIBUTES || DeleteFile(pBufPath))
            {
                FRunMainApp(pBufPath, pDelimPath);
                PostMessageW(hWnd, WM_CLOSE, 0, 0);
            }
            break;
        }
        case eBtnApply:
        {
            bool bOk = false;
            LRESULT iRes = SendMessageW(hWndUpDownUpdateInterval, UDM_GETPOS, 0, 0);
            if (iRes >= 1000 && iRes <= 30000)
            {
                tgSave->iUpdateInterval = iRes;
                if ((iRes = SendMessageW(hWndUpDownOpacity, UDM_GETPOS, 0, 0)) > 0 && iRes <= 255)
                {
                    tgSave->iOpacity = iRes;
                    if ((iRes = SendMessageW(hWndUpDownThemeMargin, UDM_GETPOS, 0, 0)) >= 0 && iRes <= 1000)
                    {
                        tgSave->iThemeMargin = iRes;
                        if ((iRes = SendMessageW(hWndUpDownThemeGap, UDM_GETPOS, 0, 0)) >= 0 && iRes <= 1000)
                        {
                            tgSave->iThemeGap = iRes;
                            if ((iRes = SendMessageW(hWndUpDownProgressbarWidth, UDM_GETPOS, 0, 0)) >= 80 && iRes <= 10000)
                            {
                                tgSave->iProgressbarWidth = iRes;
                                if ((iRes = SendMessageW(hWndUpDownProgressbarHeight, UDM_GETPOS, 0, 0)) >= 0 && iRes <= 1000)
                                {
                                    tgSave->iProgressbarHeight = iRes;
                                    if ((iRes = SendMessageW(hWndUpDownLabelHeight, UDM_GETPOS, 0, 0)) >= 0 && iRes <= 1000)
                                    {
                                        tgSave->iLabelHeight = iRes;
                                        iRes = SendMessageW(hWndEditRunApp, EM_LINELENGTH, 0, 0);
                                        if (iRes >= 0 && iRes < MAX_PATH+2)
                                        {
                                            *static_cast<WORD*>(static_cast<void*>(tgSave->wAppPath)) = iRes+1;        //first word contains buffer size (+1 to check trim)
                                            if (SendMessageW(hWndEditRunApp, EM_GETLINE, 0, reinterpret_cast<LPARAM>(tgSave->wAppPath)) == iRes)
                                            {
                                                tgSave->wAppPath[iRes] = L'\0';
                                                tgSave->wAppPath[1+MAX_PATH] = L'\0';
                                                if (tgSave->dwMagic != dwMagic)
                                                {
                                                    tgSave->dwMagic = dwMagic;
                                                    tgSave->iDx = 0;
                                                    tgSave->iDy = 0;
                                                }
                                                tgSave->colorRef = *pColorRefCust;
                                                const HANDLE hFile = CreateFileW(pBufPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
                                                if (hFile != INVALID_HANDLE_VALUE)
                                                {
                                                    DWORD dwBytes;
                                                    if (WriteFile(hFile, tgSave, sizeof(TagSaveSettings), &dwBytes, nullptr) && dwBytes == sizeof(TagSaveSettings))
                                                        bOk = true;
                                                    CloseHandle(hFile);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (bOk)
                FRunMainApp(pBufPath, pDelimPath);
            else
                MessageBoxW(hWnd, L"Save settings failed", L"SystemMonitorSettings", MB_ICONWARNING);
            break;
        }
        }
        return 0;
    }
    case WM_ENDSESSION:
    {
        SendMessageW(hWnd, WM_CLOSE, 0, 0);
        return 0;
    }
    case WM_DESTROY:
    {
        if (hFont)
            DeleteObject(hFont);
        if (hBitmapColor)
            DeleteObject(hBitmapColor);
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

//-------------------------------------------------------------------------------------------------
static void FMain()
{
    if (const HWND hWnd = FindWindowW(g_wGuidClass, nullptr))
        PostMessageW(hWnd, WM_CLOSE, 0, 0);

    wchar_t wBufPath[1+MAX_PATH+1];
    *wBufPath = L'"';
    wchar_t *wBuf = wBufPath+1;
    const DWORD dwLen = GetModuleFileNameW(nullptr, wBuf, MAX_PATH+1);
    if (dwLen >= 4 && dwLen < MAX_PATH)
    {
        wchar_t *pDelim = wBuf+dwLen;
        do
        {
            if (*--pDelim == L'\\')
                break;
        } while (pDelim > wBuf);
        if (pDelim >= wBuf+2 && pDelim <= wBuf+MAX_PATH-23)        //"\SystemMonitor.exe.cfg`"
        {
            RECT rect; rect.left = 0; rect.top = 0; rect.right = 358; rect.bottom = 314;
            if (AdjustWindowRectEx(&rect, (WS_OVERLAPPEDWINDOW | WS_VISIBLE) & ~(WS_MAXIMIZEBOX | WS_SIZEBOX), FALSE, 0))
            {
                const HINSTANCE hInst = GetModuleHandleW(nullptr);
                WNDCLASSEX wndCl;
                wndCl.cbSize = sizeof(WNDCLASSEX);
                wndCl.style = 0;
                wndCl.lpfnWndProc = WindowProc;
                wndCl.cbClsExtra = 0;
                wndCl.cbWndExtra = 0;
                wndCl.hInstance = hInst;
                wndCl.hIcon = static_cast<HICON>(LoadImageW(hInst, L"IDI_ICON1", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
                wndCl.hCursor = LoadCursorW(nullptr, IDC_ARROW);
                wndCl.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1);
                wndCl.lpszMenuName = nullptr;
                wndCl.lpszClassName = g_wGuidClass;
                wndCl.hIconSm = static_cast<HICON>(LoadImageW(hInst, L"IDI_ICON1", IMAGE_ICON, 16, 16, 0));

                if (RegisterClassExW(&wndCl))
                {
                    RECT rectArea;
                    if (SystemParametersInfoW(SPI_GETWORKAREA, 0, &rectArea, 0))
                    {
                        TagCreateParams tgCreateParams;
                        tgCreateParams.pBufPath = wBuf;
                        FCopyMemoryW(pDelim+1, L"SystemMonitor.exe.cfg");
                        tgCreateParams.pDelimPath = pDelim+18;        //"\SystemMonitor.exe"
                        COLORREF colorRefCust[1+16+100];        //current+custom+image*
                        tgCreateParams.pColorRefCust = colorRefCust;        //***
                        TagSaveSettings tgSave;
                        tgCreateParams.pTgSave = &tgSave;        //***
                        rect.right -= rect.left;
                        rect.bottom -= rect.top;
                        if (CreateWindowExW(0, g_wGuidClass, L"SystemMonitorSettings", (WS_OVERLAPPEDWINDOW | WS_VISIBLE) & ~(WS_MAXIMIZEBOX | WS_SIZEBOX),
                                            (rectArea.right-rectArea.left)/2-rect.right/2, (rectArea.bottom-rectArea.top)/2-rect.bottom/2, rect.right, rect.bottom, nullptr, nullptr, hInst, &tgCreateParams))
                        {
                            MSG msg;
                            while (GetMessageW(&msg, nullptr, 0, 0) > 0)
                            {
                                TranslateMessage(&msg);
                                DispatchMessageW(&msg);
                            }
                        }
                    }
                    UnregisterClassW(g_wGuidClass, hInst);
                }
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------
extern "C" int start()
{
    FMain();
    ExitProcess(0);
    return 0;
}
