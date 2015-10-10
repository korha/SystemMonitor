#include <cstdio>
#include <windows.h>
#include <ntdef.h>
#include <ntstatus.h>
#include <tlhelp32.h>
#include <commctrl.h>
#include <cassert>

#define TICKS_PER_MUS 10LL        //1 tick = 100NS
#define TICKS_PER_MS (TICKS_PER_MUS*1000LL)
#define TICKS_PER_SEC (TICKS_PER_MS*1000LL)
#define TICKS_PER_MIN (TICKS_PER_SEC*60LL)
#define TICKS_PER_HOUR (TICKS_PER_MIN*60LL)
#define TICKS_PER_DAY (TICKS_PER_HOUR*24LL)

static const wchar_t *const g_wGuidClass = L"App::ec19dd9c-939a-4b05-c2fb-309ff78bcf32",
*const g_wGuidClassProgress = L"Progress::ec19dd9c-939a-4b05-c2fb-309ff78bcf32",
#ifdef _WIN64
*const g_wSettingsApp = L"SystemMonitorSettings64.exe";
#else
*const g_wSettingsApp = L"SystemMonitorSettings32.exe";
#endif

typedef struct _PROCESSOR_POWER_INFORMATION
{
    ULONG Number,
    MaxMhz,
    CurrentMhz,
    MhzLimit,
    MaxIdleState,
    CurrentIdleState;
} PROCESSOR_POWER_INFORMATION;

typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION
{
    ULONG ProcessorCount,
    Offsets[1];
} SYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION;

typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT
{
    LARGE_INTEGER Hits;
    UCHAR PercentFrequency;
} SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT;

typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8
{
    ULONG Hits;
    UCHAR PercentFrequency;
} SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8;

typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION
{
    ULONG ProcessorNumber,
    StateCount;
    SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT States[1];
} SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION;

typedef struct _SYSTEM_TIMEOFDAY_INFORMATION
{
    LARGE_INTEGER BootTime,
    CurrentTime,
    TimeZoneBias;
    ULONG CurrentTimeZoneId;
    BYTE Reserved1[20];
} SYSTEM_TIMEOFDAY_INFORMATION;

struct TagCreateParams
{
    wchar_t *pBufPath;
    MEMORYSTATUSEX *pMemStatusEx;
    FILETIME *pFts;
    PROCESS_INFORMATION *pPi;
    STARTUPINFO *pSi;
    SYSTEM_TIMEOFDAY_INFORMATION *pSysTimeInfo;
    PROCESSENTRY32 *pProcessEntry32;
    WNDCLASS *pWndCl;
    RECT *pRectGeometry;
};

//-------------------------------------------------------------------------------------------------
inline SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION* fByteToSppsd(BYTE *pByte)
{
    return static_cast<SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION*>(static_cast<void*>(pByte));
}
inline SYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION* fByteToSppd(BYTE *pByte)
{
    return static_cast<SYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION*>(static_cast<void*>(pByte));
}
inline BYTE* fSppdToByte(SYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION *pSppd)
{
    return static_cast<BYTE*>(static_cast<void*>(pSppd));
}
inline BYTE* fSpphToByte(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT *pSpph)
{
    return static_cast<BYTE*>(static_cast<void*>(pSpph));
}
inline const SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8* fByteToSpphw(const BYTE *pByte)
{
    return static_cast<const SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8*>(static_cast<const void*>(pByte));
}
inline const SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8* fSpphToSpphw(const SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT *pSpph)
{
    return static_cast<const SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8*>(static_cast<const void*>(pSpph));
}
inline bool fOk(const wchar_t *const wOk)
{
    return !(*wOk || errno);
}

//-------------------------------------------------------------------------------------------------
static HWND hWndApp, hWndPrbPhysical, hWndPrbVirtual, hWndPrbCpu, hWndCpuSpeed;
static ULONG iPhysicalPercentOld, iVirtualPercentOld, iCpuLoadOld, iCpuSpeedOld, iCpuSpeedAll;
static wchar_t *pPercents, *pClassName;
static RECT rectBackground, rectLoad;
static HBRUSH hbrBackground, hbrLoad, hbrSysBackground;
static PAINTSTRUCT *pPs;
static HFONT hFont;

//-------------------------------------------------------------------------------------------------
void CALLBACK WinEventProc(HWINEVENTHOOK, DWORD, HWND hWnd, LONG, LONG, DWORD, DWORD)
{
    if (GetClassName(hWnd, pClassName, 9/*"WorkerW`?"*/) == 7 && wcscmp(pClassName, L"WorkerW") == 0)
        for (int i = 0; i < 5; ++i)        //repeat several times
        {
            SetWindowPos(hWndApp, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            SetWindowPos(hWndApp, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
        }
}

//-------------------------------------------------------------------------------------------------
LRESULT CALLBACK WindowProcProgress(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_PAINT)
    {
        assert(hWnd == hWndPrbPhysical || hWnd == hWndPrbVirtual || hWnd == hWndPrbCpu);
        const ULONG iValue = (hWnd == hWndPrbCpu ? iCpuLoadOld : (hWnd == hWndPrbPhysical ? iPhysicalPercentOld : iVirtualPercentOld));
        assert(iValue <= 100);
        rectLoad.right = (rectBackground.right - 1/*fix right border*/)*iValue/100 + 1/*left border*/;
        int iLength;
        if (iValue < 10)
        {
            pPercents[0] = L'0' + iValue;
            pPercents[1] = L'%';
            iLength = 2;
        }
        else if (iValue < 100)
        {
            pPercents[0] = L'0' + iValue/10;
            pPercents[1] = L'0' + iValue%10;
            pPercents[2] = L'%';
            iLength = 3;
        }
        else
        {
            wcscpy(pPercents, L"100%");
            iLength = 4;
        }
        if (const HDC hDc = BeginPaint(hWnd, pPs))
        {
            FillRect(hDc, &rectBackground, hbrBackground);
            FillRect(hDc, &rectLoad, hbrLoad);
            SelectObject(hDc, hFont);
            SetBkMode(hDc, TRANSPARENT);
            DrawText(hDc, pPercents, iLength, &rectBackground, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
            EndPaint(hWnd, pPs);
        }
        return 0;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//-------------------------------------------------------------------------------------------------
LRESULT CALLBACK WindowProcStaticCpuSpeed(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
{
    if (uMsg == WM_CTLCOLORSTATIC)
    {
        if (reinterpret_cast<HWND>(lParam) == hWndCpuSpeed)
        {
            SetBkMode(reinterpret_cast<HDC>(wParam), TRANSPARENT);
            if (iCpuSpeedOld > iCpuSpeedAll)        //Turbo Boost / Turbo Core
                SetTextColor(reinterpret_cast<HDC>(wParam), RGB(255, 0, 0));
            return reinterpret_cast<LRESULT>(hbrSysBackground);
        }
    }
    else if (uMsg == WM_NCDESTROY)
        RemoveWindowSubclass(hWnd, WindowProcStaticCpuSpeed, 1);
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

//-------------------------------------------------------------------------------------------------
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    typedef enum _SYSTEM_INFORMATION_CLASS
    {
        SystemTimeOfDayInformation = 3,
        SystemProcessorPerformanceDistribution = 100
    } SYSTEM_INFORMATION_CLASS;

    typedef NTSTATUS WINAPI (*PNtQuerySystemInformation)
            (SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation,
             ULONG SystemInformationLength, PULONG ReturnLength);

    typedef NTSTATUS WINAPI (*PNtPowerInformation)
            (POWER_INFORMATION_LEVEL InformationLevel, PVOID lpInputBuffer,
             ULONG nInputiBufferSize, PVOID lpOutputBuffer, ULONG nOutputiBufferSize);

    enum {eRunApp = 1, eUptime, eSavePos, eStg, eClose};
    enum {eWinVista = 0x600, eWin7 = 0x601, eWin8_1 = 0x603};

    static const ULONG iStateSize = FIELD_OFFSET(SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION, States) + 2*sizeof(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT);
    assert(iStateSize == 40);

    static HWINEVENTHOOK hWinEventHook;
    static PNtQuerySystemInformation NtQuerySystemInformation;
    static PNtPowerInformation NtPowerInformation;
    static DWORD dwNumberOfProcessors, dwProcessorsSize;
    static PROCESSOR_POWER_INFORMATION *pPowerInformation;
    static BYTE *pDifferences;
    static ULONG iBufferSize;
    static SYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION *pBufferA, *pBufferB;
    static HWND hWndPhysical, hWndVirtual, hWndProcUptime;
    static wchar_t *pRunApp, *pPath, *pPhysical, *pVirtual, *pCpuSpeed, *pProcesses, *pUptime,
            *pPhysicalAll, *pVirtualAll, *pCpuSpeedAll;
    static SYSTEM_TIMEOFDAY_INFORMATION *pSysTimeInfo;
    static PROCESS_INFORMATION *pPi;
    static STARTUPINFO *pSi;
    static MEMORYSTATUSEX *pMemStatusEx;
    static PROCESSENTRY32 *pProcessEntry32;
    static FILETIME *pFtIdleTime, *pFtKernelTime, *pFtUserTime;
    static const DWORDLONG *pDwIdleTime, *pDwKernelTime, *pDwUserTime;
    static WORD wWinVer;
    static RECT rectWindow;
    static ULONG iProcessesOld, iTemp, iTempInt;
    static ULONGLONG iTempTotal, iTempCount;

    switch (uMsg)
    {
    case WM_CREATE:
        if (const HMODULE hMod = GetModuleHandle(L"ntdll.dll"))
            if ((NtQuerySystemInformation = reinterpret_cast<PNtQuerySystemInformation>(GetProcAddress(hMod, "NtQuerySystemInformation"))) &&
                    ((NtPowerInformation = reinterpret_cast<PNtPowerInformation>(GetProcAddress(hMod, "NtPowerInformation")))))
            {
                SYSTEM_INFO sysInfo;
                GetNativeSystemInfo(&sysInfo);
                dwNumberOfProcessors = sysInfo.dwNumberOfProcessors;
                dwProcessorsSize = dwNumberOfProcessors*sizeof(PROCESSOR_POWER_INFORMATION);
                if ((pPowerInformation = static_cast<PROCESSOR_POWER_INFORMATION*>(malloc(dwProcessorsSize))))
                {
                    wWinVer = GetVersion();
                    wWinVer = wWinVer << 8 | wWinVer >> 8;

                    if (wWinVer >= eWinVista &&
                            !((pDifferences = static_cast<BYTE*>(malloc(iStateSize*dwNumberOfProcessors))) &&
                              NtQuerySystemInformation(SystemProcessorPerformanceDistribution, 0, 0, &iBufferSize) == STATUS_INFO_LENGTH_MISMATCH &&
                              ((pBufferA = static_cast<SYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION*>(malloc(iBufferSize*2)))) &&
                              NT_SUCCESS(NtQuerySystemInformation(SystemProcessorPerformanceDistribution, pBufferA, iBufferSize, 0))))
                        return -1;

                    pBufferB = fByteToSppd((fSppdToByte(pBufferA) + iBufferSize));

                    const CREATESTRUCT *const crStruct = reinterpret_cast<const CREATESTRUCT*>(lParam);
                    const TagCreateParams *tgCreateParams = static_cast<const TagCreateParams*>(crStruct->lpCreateParams);
                    //[MAX_PATH+MAX_PATH+136] = [999000/999000 MB`999000/999000 MB`99.00/99.00 Ghz`Processes: 99000`Uptime: 9999d 23h 59m`/999000 MB`/999000 MB`/99.00 Ghz`100%`WorkerW`?]
                    pPath = tgCreateParams->pBufPath;
                    pPhysical = pPath+MAX_PATH*2;
                    pVirtual = pPath+MAX_PATH*2+17;
                    pCpuSpeed = pPath+MAX_PATH*2+34;
                    pProcesses = pPath+MAX_PATH*2+50;
                    pUptime = pPath+MAX_PATH*2+67;
                    pPhysicalAll = pPath+MAX_PATH*2+89;
                    pVirtualAll = pPath+MAX_PATH*2+100;
                    pCpuSpeedAll = pPath+MAX_PATH*2+111;
                    pPercents = pPath+MAX_PATH*2+122;
                    pClassName = pPath+MAX_PATH*2+127;

                    wcscpy(pProcesses, L"Processes: ");
                    wcscpy(pUptime, L"Uptime: ");
                    *pPhysicalAll = L'/';
                    *pVirtualAll = L'/';
                    *pCpuSpeedAll = L'/';

                    pMemStatusEx = tgCreateParams->pMemStatusEx;
                    pFtIdleTime = tgCreateParams->pFts;
                    pFtKernelTime = tgCreateParams->pFts+1;
                    pFtUserTime = tgCreateParams->pFts+2;
                    pDwIdleTime = static_cast<const DWORDLONG*>(static_cast<const void*>(pFtIdleTime));
                    pDwKernelTime = static_cast<const DWORDLONG*>(static_cast<const void*>(pFtKernelTime));
                    pDwUserTime = static_cast<const DWORDLONG*>(static_cast<const void*>(pFtUserTime));
                    pPi = tgCreateParams->pPi;
                    pSi = tgCreateParams->pSi;
                    pSysTimeInfo = tgCreateParams->pSysTimeInfo;
                    pProcessEntry32 = tgCreateParams->pProcessEntry32;

                    if (GlobalMemoryStatusEx(pMemStatusEx) && NT_SUCCESS(NtPowerInformation(ProcessorInformation, 0, 0, pPowerInformation, dwProcessorsSize)))
                    {
                        int iStgDx = 0,
                                iStgDy = 0,
                                iUpdateInterval = 5000,
                                iOpacity = 255,
                                iThemeMargin = 5,
                                iThemeGap = 15,
                                iPrbWidth = 150,
                                iPrbHeight = 15,
                                iLabelHeight = 13;
                        COLORREF colorRefLoad = RGB(6, 176, 37);

                        if (FILE *file = _wfopen(pPath, L"rb"))
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
                                        if (pForLine)
                                        {
                                            pForLine += 4;
                                            if ((pDelim = wcschr(pForLine, L'\n')))
                                            {
                                                *pDelim = L'\0';
                                                iTemp = wcstoul(pForLine, &wOk, 10);
                                                if (fOk(wOk) && iTemp <= 0x7FFF)
                                                    iStgDx = iTemp;
                                                *pDelim = L'\n';
                                            }
                                        }
                                        if ((pForLine = wcsstr(wFileFind, L"\nDy=")))
                                        {
                                            pForLine += 4;
                                            if ((pDelim = wcschr(pForLine, L'\n')))
                                            {
                                                *pDelim = L'\0';
                                                iTemp = wcstoul(pForLine, &wOk, 10);
                                                if (fOk(wOk) && iTemp <= 0x7FFF)
                                                    iStgDy = iTemp;
                                                *pDelim = L'\n';
                                            }
                                        }
                                        if ((pForLine = wcsstr(wFileFind, L"\nUpdateInterval=")))
                                        {
                                            pForLine += 16;
                                            if ((pDelim = wcschr(pForLine, L'\n')))
                                            {
                                                *pDelim = L'\0';
                                                iTemp = wcstoul(pForLine, &wOk, 10);
                                                if (fOk(wOk) && iTemp >= 1000 && iTemp <= 30000)
                                                    iUpdateInterval = iTemp;
                                                *pDelim = L'\n';
                                            }
                                        }
                                        if ((pForLine = wcsstr(wFileFind, L"\nOpacity=")))
                                        {
                                            pForLine += 9;
                                            if ((pDelim = wcschr(pForLine, L'\n')))
                                            {
                                                *pDelim = L'\0';
                                                iTemp = wcstoul(pForLine, &wOk, 10);
                                                if (fOk(wOk) && iTemp > 1 && iTemp < 255)
                                                    iOpacity = iTemp;
                                                *pDelim = L'\n';
                                            }
                                        }
                                        if ((pForLine = wcsstr(wFileFind, L"\nRunApp=")))
                                        {
                                            pForLine += 8;
                                            if ((pDelim = wcschr(pForLine, L'\n')))
                                            {
                                                *pDelim = L'\0';
                                                if ((pRunApp = static_cast<wchar_t*>(malloc((pDelim-pForLine+1)*sizeof(wchar_t)))))
                                                    wcscpy(pRunApp, pForLine);
                                                *pDelim = L'\n';
                                            }
                                        }
                                        if ((pForLine = wcsstr(wFileFind, L"\nThemeMargin=")))
                                        {
                                            pForLine += 13;
                                            if ((pDelim = wcschr(pForLine, L'\n')))
                                            {
                                                *pDelim = L'\0';
                                                iTemp = wcstoul(pForLine, &wOk, 10);
                                                if (fOk(wOk) && iTemp <= 1000)
                                                    iThemeMargin = iTemp;
                                                *pDelim = L'\n';
                                            }
                                        }
                                        if ((pForLine = wcsstr(wFileFind, L"\nThemeGap=")))
                                        {
                                            pForLine += 10;
                                            if ((pDelim = wcschr(pForLine, L'\n')))
                                            {
                                                *pDelim = L'\0';
                                                iTemp = wcstol(pForLine, &wOk, 10);
                                                if (fOk(wOk) && iTemp <= 1000)
                                                    iThemeGap = iTemp;
                                                *pDelim = L'\n';
                                            }
                                        }
                                        if ((pForLine = wcsstr(wFileFind, L"\nProgressbarWidth=")))
                                        {
                                            pForLine += 18;
                                            if ((pDelim = wcschr(pForLine, L'\n')))
                                            {
                                                *pDelim = L'\0';
                                                iTemp = wcstoul(pForLine, &wOk, 10);
                                                if (fOk(wOk) && iTemp >= 80 && iTemp <= 10000)
                                                    iPrbWidth = iTemp;
                                                *pDelim = L'\n';
                                            }
                                        }
                                        if ((pForLine = wcsstr(wFileFind, L"\nProgressbarHeight=")))
                                        {
                                            pForLine += 19;
                                            if ((pDelim = wcschr(pForLine, L'\n')))
                                            {
                                                *pDelim = L'\0';
                                                iTemp = wcstoul(pForLine, &wOk, 10);
                                                if (fOk(wOk) && iTemp <= 1000)
                                                    iPrbHeight = iTemp;
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
                                                iTemp = wcstoul(pForLine, &wOk, 16);
                                                if (fOk(wOk))
                                                {
                                                    assert(iTemp <= 0xFFFFFF);
                                                    colorRefLoad = iTemp;
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
                                                iTemp = wcstoul(pForLine, &wOk, 10);
                                                if (fOk(wOk) && iTemp <= 1000)
                                                    iLabelHeight = iTemp;
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
                                                    LOGFONT logFont;
                                                    logFont.lfHeight = iFontArray[eHeight];
                                                    logFont.lfWidth = iFontArray[eWidth];
                                                    logFont.lfEscapement = iFontArray[eEscapement];
                                                    logFont.lfOrientation = iFontArray[eOrientation];
                                                    logFont.lfWeight = iFontArray[eWeight];
                                                    logFont.lfItalic = iFontArray[eItalic];
                                                    logFont.lfUnderline = iFontArray[eUnderline];
                                                    logFont.lfStrikeOut = iFontArray[eStrikeOut];
                                                    logFont.lfCharSet = iFontArray[eCharSet];
                                                    logFont.lfOutPrecision = iFontArray[eOutPrecision];
                                                    logFont.lfClipPrecision = iFontArray[eClipPrecision];
                                                    logFont.lfQuality = iFontArray[eQuality];
                                                    logFont.lfPitchAndFamily = iFontArray[ePitchAndFamily];
                                                    *pDelim = L'\0';
                                                    wcscpy(logFont.lfFaceName, wFontArray[eFaceName]);
                                                    hFont = CreateFontIndirect(&logFont);
                                                }
                                            }
                                        }
                                    }
                                    free(wFile);
                                }
                            if (file)
                                fclose(file);
                        }

                        const HINSTANCE hInst = crStruct->hInstance;
                        WNDCLASS *const pWndCl = tgCreateParams->pWndCl;
                        pWndCl->lpfnWndProc = WindowProcProgress;
                        pWndCl->hbrBackground = reinterpret_cast<HBRUSH>(COLOR_ACTIVEBORDER+1);
                        pWndCl->lpszClassName = g_wGuidClassProgress;
                        if (RegisterClass(pWndCl) && (pWndCl->lpszClassName = 0/*check unreg*/, (hbrBackground = CreateSolidBrush(RGB(230, 230, 230)))) && (hbrLoad = CreateSolidBrush(colorRefLoad)))
                            if (const HWND hWndBtnRunApp = CreateWindowEx(WS_EX_NOACTIVATE, WC_BUTTON, 0, WS_CHILD | WS_VISIBLE | BS_ICON, iThemeMargin, iThemeMargin, 16, 16, hWnd, reinterpret_cast<HMENU>(eRunApp), hInst, 0))
                                if (const HWND hWndBtnUptime = CreateWindowEx(WS_EX_NOACTIVATE, WC_BUTTON, 0, WS_CHILD | WS_VISIBLE | BS_ICON, iThemeMargin+16, iThemeMargin, 16, 16, hWnd, reinterpret_cast<HMENU>(eUptime), hInst, 0))
                                {
                                    iTemp = iThemeMargin*3+iPrbWidth;
                                    if (const HWND hWndBtnSavePos = CreateWindowEx(WS_EX_NOACTIVATE, WC_BUTTON, 0, WS_CHILD | WS_VISIBLE | BS_ICON, iTemp-48, iThemeMargin, 16, 16, hWnd, reinterpret_cast<HMENU>(eSavePos), hInst, 0))
                                        if (const HWND hWndBtnStg = CreateWindowEx(WS_EX_NOACTIVATE, WC_BUTTON, 0, WS_CHILD | WS_VISIBLE | BS_ICON, iTemp-32, iThemeMargin, 16, 16, hWnd, reinterpret_cast<HMENU>(eStg), hInst, 0))
                                            if (const HWND hWndBtnClose = CreateWindowEx(WS_EX_NOACTIVATE, WC_BUTTON, 0, WS_CHILD | WS_VISIBLE | BS_ICON, iTemp-16, iThemeMargin, 16, 16, hWnd, reinterpret_cast<HMENU>(eClose), hInst, 0))
                                            {
                                                rectBackground.left = 1;
                                                rectBackground.top = 1;
                                                rectBackground.right = iPrbWidth-1;
                                                rectBackground.bottom = iPrbHeight-1;
                                                rectLoad = rectBackground;
                                                const int iGbMemoryHeight = iThemeGap+iLabelHeight*2+iPrbHeight*2+iThemeMargin*3;
                                                if (const HWND hWndGbMemory = CreateWindowEx(0, WC_BUTTON, L"Memory", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, iThemeMargin, iThemeMargin*2+16, iThemeMargin*2+iPrbWidth, iGbMemoryHeight, hWnd, 0, hInst, 0))
                                                    if (const HWND hWndStPhysical = CreateWindowEx(0, WC_STATIC, L"Physic:\0\0", WS_CHILD | WS_VISIBLE, iThemeMargin, iThemeGap, iPrbWidth/3, iLabelHeight, hWndGbMemory, 0, hInst, 0))
                                                        if ((hWndPhysical = CreateWindowEx(0, WC_STATIC, 0, WS_CHILD | WS_VISIBLE | SS_RIGHT, iThemeMargin+iPrbWidth/3, iThemeGap, iPrbWidth*2/3, iLabelHeight, hWndGbMemory, 0, hInst, 0)) &&
                                                                (hWndPrbPhysical = CreateWindowEx(0, g_wGuidClassProgress, 0, WS_CHILD | WS_VISIBLE, iThemeMargin, iThemeGap+iLabelHeight, iPrbWidth, iPrbHeight, hWndGbMemory, 0, hInst, 0)))
                                                            if (const HWND hWndStVirtual = CreateWindowEx(0, WC_STATIC, L"Virtual:", WS_CHILD | WS_VISIBLE, iThemeMargin, iThemeGap+iLabelHeight+iPrbHeight+iThemeMargin*2, iPrbWidth/3, iLabelHeight, hWndGbMemory, 0, hInst, 0))
                                                                if ((hWndVirtual = CreateWindowEx(0, WC_STATIC, 0, WS_CHILD | WS_VISIBLE | SS_RIGHT, iThemeMargin+iPrbWidth/3, iThemeGap+iLabelHeight+iPrbHeight+iThemeMargin*2, iPrbWidth*2/3, iLabelHeight, hWndGbMemory, 0, hInst, 0)) &&
                                                                        (hWndPrbVirtual = CreateWindowEx(0, g_wGuidClassProgress, 0, WS_CHILD | WS_VISIBLE, iThemeMargin, iThemeGap+iPrbHeight+iLabelHeight*2+iThemeMargin*2, iPrbWidth, iPrbHeight, hWndGbMemory, 0, hInst, 0)))
                                                                {
                                                                    const int iGbCpuHeight = iThemeGap+iLabelHeight*2+iPrbHeight+iThemeMargin*2;
                                                                    if (const HWND hWndGbCpu = CreateWindowEx(0, WC_BUTTON, L"CPU", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, iThemeMargin, iThemeMargin*3+16+iGbMemoryHeight, iThemeMargin*2+iPrbWidth, iGbCpuHeight, hWnd, 0, hInst, 0))
                                                                        if (const HWND hWndStLoad = CreateWindowEx(0, WC_STATIC, L"Load:", WS_CHILD | WS_VISIBLE, iThemeMargin, iThemeGap, iPrbWidth, iLabelHeight, hWndGbCpu, 0, hInst, 0))
                                                                            if ((hWndPrbCpu = CreateWindowEx(0, g_wGuidClassProgress, 0, WS_CHILD | WS_VISIBLE, iThemeMargin, iThemeGap+iLabelHeight, iPrbWidth, iPrbHeight, hWndGbCpu, 0, hInst, 0)))
                                                                                if (const HWND hWndStCpuSpeed = CreateWindowEx(0, WC_STATIC, L"Speed:", WS_CHILD | WS_VISIBLE, iThemeMargin, iThemeGap+iLabelHeight+iPrbHeight+iThemeMargin, iPrbWidth/3, iLabelHeight, hWndGbCpu, 0, hInst, 0))
                                                                                    if ((hWndCpuSpeed = CreateWindowEx(0, WC_STATIC, 0, WS_CHILD | WS_VISIBLE | SS_RIGHT, iThemeMargin+iPrbWidth/3, iThemeGap+iLabelHeight+iPrbHeight+iThemeMargin, iPrbWidth*2/3, iLabelHeight, hWndGbCpu, 0, hInst, 0)))
                                                                                    {
                                                                                        if (wWinVer >= eWinVista && !((hbrSysBackground = GetSysColorBrush(COLOR_BTNFACE)) &&
                                                                                                                      SetWindowSubclass(hWndGbCpu, WindowProcStaticCpuSpeed, 1, 0) == TRUE))
                                                                                            return -1;
                                                                                        iTemp = iThemeMargin*4+16+iGbMemoryHeight+iGbCpuHeight;
                                                                                        if ((hWndProcUptime = CreateWindowEx(0, WC_STATIC, 0, WS_CHILD | WS_VISIBLE, iThemeMargin*2, iTemp, iPrbWidth, iLabelHeight, hWnd, 0, hInst, 0)))
                                                                                            if (HWND hWndTooltip = CreateWindowEx(0, TOOLTIPS_CLASS, 0, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, 0, hInst, 0))
                                                                                                if (SetWindowPos(hWndTooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE))
                                                                                                {
                                                                                                    wchar_t wBufTooltips[] = L"Run app\0Uptime\0Save position\0Settings\0Close";
                                                                                                    TOOLINFO toolInfo;
                                                                                                    toolInfo.cbSize = sizeof(TOOLINFO);
                                                                                                    toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
                                                                                                    toolInfo.hwnd = hWnd;
                                                                                                    toolInfo.uId = reinterpret_cast<UINT_PTR>(hWndBtnRunApp);
                                                                                                    toolInfo.rect = rectWindow;
                                                                                                    toolInfo.hinst = hInst;
                                                                                                    toolInfo.lpszText = wBufTooltips;
                                                                                                    toolInfo.lParam = 0;
                                                                                                    toolInfo.lpReserved = 0;
                                                                                                    if (SendMessage(hWndTooltip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo)) == TRUE)
                                                                                                        if ((hWndTooltip = CreateWindowEx(0, TOOLTIPS_CLASS, 0, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, 0, hInst, 0)))
                                                                                                            if (SetWindowPos(hWndTooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE))
                                                                                                            {
                                                                                                                toolInfo.uId = reinterpret_cast<UINT_PTR>(hWndBtnUptime);
                                                                                                                toolInfo.lpszText = wBufTooltips+8;
                                                                                                                if (SendMessage(hWndTooltip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo)) == TRUE)
                                                                                                                    if ((hWndTooltip = CreateWindowEx(0, TOOLTIPS_CLASS, 0, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, 0, hInst, 0)))
                                                                                                                        if (SetWindowPos(hWndTooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE))
                                                                                                                        {
                                                                                                                            toolInfo.uId = reinterpret_cast<UINT_PTR>(hWndBtnSavePos);
                                                                                                                            toolInfo.lpszText = wBufTooltips+15;
                                                                                                                            if (SendMessage(hWndTooltip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo)) == TRUE)
                                                                                                                                if ((hWndTooltip = CreateWindowEx(0, TOOLTIPS_CLASS, 0, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, 0, hInst, 0)))
                                                                                                                                    if (SetWindowPos(hWndTooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE))
                                                                                                                                    {
                                                                                                                                        toolInfo.uId = reinterpret_cast<UINT_PTR>(hWndBtnStg);
                                                                                                                                        toolInfo.lpszText = wBufTooltips+29;
                                                                                                                                        if (SendMessage(hWndTooltip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo)) == TRUE)
                                                                                                                                            if ((hWndTooltip = CreateWindowEx(0, TOOLTIPS_CLASS, 0, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, 0, hInst, 0)))
                                                                                                                                                if (SetWindowPos(hWndTooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE))
                                                                                                                                                {
                                                                                                                                                    toolInfo.uId = reinterpret_cast<UINT_PTR>(hWndBtnClose);
                                                                                                                                                    toolInfo.lpszText = wBufTooltips+38;
                                                                                                                                                    if (SendMessage(hWndTooltip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo)) == TRUE)
                                                                                                                                                    {
                                                                                                                                                        RECT *const pRectGeometry = tgCreateParams->pRectGeometry;
                                                                                                                                                        if (GetWindowRect(hWnd, &rectWindow) && GetClientRect(hWnd, pRectGeometry))
                                                                                                                                                        {
                                                                                                                                                            if (!hFont)
                                                                                                                                                            {
                                                                                                                                                                NONCLIENTMETRICS nonClientMetrics;
                                                                                                                                                                nonClientMetrics.cbSize = sizeof(NONCLIENTMETRICS);
                                                                                                                                                                if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &nonClientMetrics, 0))
                                                                                                                                                                    hFont = CreateFontIndirect(&nonClientMetrics.lfMessageFont);
                                                                                                                                                            }
                                                                                                                                                            if (hFont)
                                                                                                                                                            {
                                                                                                                                                                SendMessage(hWndGbMemory, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                                                SendMessage(hWndStPhysical, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                                                SendMessage(hWndPhysical, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                                                SendMessage(hWndStVirtual, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                                                SendMessage(hWndVirtual, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                                                SendMessage(hWndGbCpu, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                                                SendMessage(hWndStLoad, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                                                SendMessage(hWndStCpuSpeed, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                                                SendMessage(hWndCpuSpeed, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                                                                                                                                                                SendMessage(hWndProcUptime, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);

                                                                                                                                                                HICON hIcon = static_cast<HICON>(LoadImage(hInst, L"IDI_ICON2", IMAGE_ICON, 16, 16, 0));
                                                                                                                                                                if (hIcon)
                                                                                                                                                                {
                                                                                                                                                                    SendMessage(hWndBtnRunApp, BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(hIcon));
                                                                                                                                                                    DestroyIcon(hIcon);
                                                                                                                                                                }
                                                                                                                                                                if ((hIcon = static_cast<HICON>(LoadImage(hInst, L"IDI_ICON3", IMAGE_ICON, 16, 16, 0))))
                                                                                                                                                                {
                                                                                                                                                                    SendMessage(hWndBtnUptime, BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(hIcon));
                                                                                                                                                                    DestroyIcon(hIcon);
                                                                                                                                                                }
                                                                                                                                                                if ((hIcon = static_cast<HICON>(LoadImage(hInst, L"IDI_ICON4", IMAGE_ICON, 16, 16, 0))))
                                                                                                                                                                {
                                                                                                                                                                    SendMessage(hWndBtnSavePos, BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(hIcon));
                                                                                                                                                                    DestroyIcon(hIcon);
                                                                                                                                                                }
                                                                                                                                                                if ((hIcon = static_cast<HICON>(LoadImage(hInst, L"IDI_ICON5", IMAGE_ICON, 16, 16, 0))))
                                                                                                                                                                {
                                                                                                                                                                    SendMessage(hWndBtnStg, BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(hIcon));
                                                                                                                                                                    DestroyIcon(hIcon);
                                                                                                                                                                }
                                                                                                                                                                if ((hIcon = static_cast<HICON>(LoadImage(hInst, L"IDI_ICON6", IMAGE_ICON, 16, 16, 0))))
                                                                                                                                                                {
                                                                                                                                                                    SendMessage(hWndBtnClose, BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(hIcon));
                                                                                                                                                                    DestroyIcon(hIcon);
                                                                                                                                                                }

                                                                                                                                                                if (!pRunApp)
                                                                                                                                                                    if ((pRunApp = static_cast<wchar_t*>(malloc(12/*taskmgr.exe`*/ *sizeof(wchar_t)))))
                                                                                                                                                                        wcscpy(pRunApp, L"taskmgr.exe");
                                                                                                                                                                if (pRunApp && SetTimer(hWnd, 1, iUpdateInterval, 0))
                                                                                                                                                                {
                                                                                                                                                                    pRectGeometry->left = iStgDx;
                                                                                                                                                                    pRectGeometry->top = iStgDy;
                                                                                                                                                                    pRectGeometry->right = iThemeMargin*4+iPrbWidth+(rectWindow.right - pRectGeometry->right);
                                                                                                                                                                    pRectGeometry->bottom = iTemp+iLabelHeight+iThemeMargin+(rectWindow.bottom - pRectGeometry->bottom);
                                                                                                                                                                    assert(rectWindow.left == 0 && rectWindow.top == 0);
                                                                                                                                                                    rectWindow.right = iPrbWidth;
                                                                                                                                                                    rectWindow.bottom = iPrbHeight;

                                                                                                                                                                    wcscat(ulltow(pMemStatusEx->ullTotalPhys >> 20, pPhysicalAll+1, 10), L" MB");

                                                                                                                                                                    iTemp = pPowerInformation[0].MaxMhz/10;
                                                                                                                                                                    iCpuSpeedAll = iTemp;
                                                                                                                                                                    iTempInt = iTemp/100;
                                                                                                                                                                    _ultow(iTempInt, pCpuSpeedAll+1, 10);
                                                                                                                                                                    wchar_t *const pDelim = pCpuSpeedAll + (iTempInt >= 10 ? 3 : 2);
                                                                                                                                                                    *pDelim = L'.';
                                                                                                                                                                    iTemp -= iTempInt*100;
                                                                                                                                                                    pDelim[1] = L'0' + iTemp/10;
                                                                                                                                                                    pDelim[2] = L'0' + iTemp%10;
                                                                                                                                                                    wcscpy(pDelim+3, L" GHz");
                                                                                                                                                                    SetWindowText(hWndCpuSpeed, pCpuSpeedAll+1);

                                                                                                                                                                    if (iOpacity < 255)
                                                                                                                                                                    {
                                                                                                                                                                        SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_WINDOWEDGE | WS_EX_LAYERED);
                                                                                                                                                                        if (!SetLayeredWindowAttributes(hWnd, 0, iOpacity, LWA_ALPHA))
                                                                                                                                                                            return -1;
                                                                                                                                                                    }

                                                                                                                                                                    hWndApp = hWnd;
                                                                                                                                                                    if (wWinVer <= eWinVista /*bad work in XP*/ ||
                                                                                                                                                                            (hWinEventHook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, 0, WinEventProc, 0, 0, 0)))
                                                                                                                                                                        return 0;
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
                                                                }
                                            }
                                }
                    }
                }
            }
        return -1;
    case WM_COMMAND:
    {
        assert(HIWORD(wParam) == 0);
        bool bRunApp = false;
        switch (wParam)
        {
        case eUptime:
            if (NT_SUCCESS(NtQuerySystemInformation(SystemTimeOfDayInformation, pSysTimeInfo, sizeof(SYSTEM_TIMEOFDAY_INFORMATION), 0)))
            {
                pSysTimeInfo->CurrentTime.QuadPart -= pSysTimeInfo->BootTime.QuadPart;
                assert(pSysTimeInfo->CurrentTime.QuadPart > 0);
                wchar_t *pDelim = pUptime+8/*Uptime: */;
                if ((iTempTotal = pSysTimeInfo->CurrentTime.QuadPart/TICKS_PER_DAY))        //days
                {
                    lltow(iTempTotal, pDelim, 10);
                    pDelim = wcschr(pDelim, L'\0');
                    wcscpy(pDelim, L"d ");
                    pDelim += 2;
                    pSysTimeInfo->CurrentTime.QuadPart -= iTempTotal*TICKS_PER_DAY;
                }
                if ((iTempTotal = pSysTimeInfo->CurrentTime.QuadPart/TICKS_PER_HOUR))        //hours
                {
                    lltow(iTempTotal, pDelim, 10);
                    pDelim = wcschr(pDelim, L'\0');
                    wcscpy(pDelim, L"h ");
                    pDelim += 2;
                    pSysTimeInfo->CurrentTime.QuadPart -= iTempTotal*TICKS_PER_HOUR;
                }
                pSysTimeInfo->CurrentTime.QuadPart /= TICKS_PER_MIN;
                wcscat(lltow(pSysTimeInfo->CurrentTime.QuadPart, pDelim, 10), L"m");        //minutes
                SetWindowText(hWndProcUptime, pUptime);
                iProcessesOld = 0;        //force update label
            }
            break;
        case eSavePos:
        {
            RECT rect;
            if (GetWindowRect(hWnd, &rect))
            {
                size_t szSub = 11,        //[11 = "[Settings]\n"]
                        szCountAll;
                wchar_t *wFile = 0,
                        *pForLine;
                bool bLoad = false;
                FILE *file = _wfopen(pPath, L"rb");
                if (file)
                {
                    fseek(file, 0, SEEK_END);
                    const size_t szCount = ftell(file);
                    if (szCount >= 16/*[Settings]nDx=1n*/ *sizeof(wchar_t) && szCount <= 1600*sizeof(wchar_t) && szCount%sizeof(wchar_t) == 0)
                        if ((wFile = static_cast<wchar_t*>(malloc(szCount + sizeof(wchar_t)))))
                        {
                            rewind(file);
                            szCountAll = fread(wFile, 1, szCount, file);
                            if (szCount == szCountAll && wcsncmp(wFile, L"[Settings]\n", 11) == 0 && (szCountAll /= sizeof(wchar_t), wFile[szCountAll-1] == L'\n'))
                            {
                                wFile[szCountAll] = L'\0';
                                if ((pForLine = wcsstr(wFile, L"\nDx=")))
                                {
                                    if (const wchar_t *const pDelim = wcschr(pForLine+4, L'\n'))
                                    {
                                        szSub += pDelim-pForLine;
                                        wcscpy(pForLine, pDelim);
                                    }
                                }
                                if ((pForLine = wcsstr(wFile, L"\nDy=")))
                                    if (const wchar_t *const pDelim = wcschr(pForLine+4, L'\n'))
                                    {
                                        szSub += pDelim-pForLine;
                                        wcscpy(pForLine, pDelim);
                                    }
                                bLoad = true;
                            }
                        }
                    fclose(file);
                }
                file = _wfopen(pPath, L"wb");
                if (file)
                {
                    if ((rect.left | 0x7FFF) != 0x7FFF)
                        rect.left = 0;
                    if ((rect.top | 0x7FFF) != 0x7FFF)
                        rect.top = 0;

                    wchar_t wBuf[30] = L"[Settings]\nDx=";        //[30 = "[Settings]nDx=32767nDy=32767n`"]
                    _ltow(rect.left, wBuf+14, 10);
                    pForLine = wcschr(wBuf+15, L'\0');
                    wcscpy(pForLine, L"\nDy=");
                    pForLine += 4;
                    _ltow(rect.top, pForLine, 10);
                    pForLine = wcschr(pForLine, L'\0');
                    *pForLine = L'\n';
                    ++pForLine;
                    fwrite(wBuf, (pForLine-wBuf)*sizeof(wchar_t), 1, file);
                    if (bLoad)
                        fwrite(wFile+11, (szCountAll-szSub)*sizeof(wchar_t), 1, file);
                    fclose(file);
                }
                free(wFile);
            }
            break;
        }
        case eRunApp:
            bRunApp = true;
        case eStg:
            if (CreateProcess(0, bRunApp ? pRunApp : pPath+MAX_PATH, 0, 0, FALSE, CREATE_UNICODE_ENVIRONMENT, 0, 0, pSi, pPi))
            {
                CloseHandle(pPi->hThread);
                CloseHandle(pPi->hProcess);
            }
            break;
        case eClose:
            PostMessage(hWnd, WM_CLOSE, 0, 0);
            break;
        }
        return 0;
    }
    case WM_TIMER:
    {
        //Memory
        if (GlobalMemoryStatusEx(pMemStatusEx))
        {
            static ULONG iPhysicalOld, iVirtualOld;

            //Physical
            iTemp = (pMemStatusEx->ullTotalPhys - pMemStatusEx->ullAvailPhys) >> 20;
            if (iTemp != iPhysicalOld)
            {
                iPhysicalOld = iTemp;
                _ultow(iPhysicalOld, pPhysical, 10);
                wcscat(pPhysical, pPhysicalAll);
                SetWindowText(hWndPhysical, pPhysical);
            }
            //Virtual
            iTempTotal = pMemStatusEx->ullTotalPageFile - pMemStatusEx->ullAvailPageFile;
            iTemp = iTempTotal >> 20;
            if (iTemp != iVirtualOld)
            {
                iVirtualOld = iTemp;
                _ultow(iVirtualOld, pVirtual, 10);
                static ULONGLONG iVirtualAllOld;
                if (pMemStatusEx->ullTotalPageFile != iVirtualAllOld)
                {
                    iVirtualAllOld = pMemStatusEx->ullTotalPageFile;
                    wcscat(_ultow(iVirtualAllOld >> 20, pVirtualAll+1, 10), L" MB");
                }
                wcscat(pVirtual, pVirtualAll);
                SetWindowText(hWndVirtual, pVirtual);
            }
            //Physical Load
            if (pMemStatusEx->dwMemoryLoad != iPhysicalPercentOld)
            {
                iPhysicalPercentOld = pMemStatusEx->dwMemoryLoad;
                InvalidateRect(hWndPrbPhysical, 0, FALSE);
            }
            //Virtual Load
            iTemp = iTempTotal*100/pMemStatusEx->ullTotalPageFile;
            if (iTemp != iVirtualPercentOld)
            {
                iVirtualPercentOld = iTemp;
                InvalidateRect(hWndPrbVirtual, 0, FALSE);
            }
        }

        //Process Count
        iTemp = 0;
        const HANDLE hProcess = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcess != INVALID_HANDLE_VALUE)
        {
            if (Process32First(hProcess, pProcessEntry32))
                do {++iTemp;} while (Process32Next(hProcess, pProcessEntry32));
            CloseHandle(hProcess);
        }
        if (iTemp != iProcessesOld)
        {
            iProcessesOld = iTemp;
            _ultow(iProcessesOld, pProcesses+11, 10);
            SetWindowText(hWndProcUptime, pProcesses);
        }

        //CPU Load
        static ULONGLONG iLoadOld, iTotalOld;
        if (GetSystemTimes(pFtIdleTime, pFtKernelTime, pFtUserTime))
        {
            iTempTotal = *pDwKernelTime + *pDwUserTime;
            iTempCount = iTempTotal - *pDwIdleTime;
            iTemp = (iTempCount - iLoadOld)*100/(iTempTotal - iTotalOld);
            iLoadOld = iTempCount;
            iTotalOld = iTempTotal;
            if (iTemp != iCpuLoadOld)
            {
                iCpuLoadOld = iTemp;
                InvalidateRect(hWndPrbCpu, 0, FALSE);
            }
        }

        //CPU Speed
        if (wWinVer >= eWinVista && NT_SUCCESS(NtPowerInformation(ProcessorInformation, 0, 0, pPowerInformation, dwProcessorsSize)))
        {
            iTempCount = 0;
            //the method taken from Process Hacker (processhacker.sourceforge.net, sysinfo.c)
            if (wWinVer >= eWin7)
            {
                static SYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION *pCurrentPerformanceDistribution, *pPreviousPerformanceDistribution;
                static bool bToggle = false;
                if (bToggle)
                {
                    pPreviousPerformanceDistribution = pBufferB;
                    pCurrentPerformanceDistribution = pBufferA;
                    bToggle = false;
                }
                else
                {
                    pPreviousPerformanceDistribution = pBufferA;
                    pCurrentPerformanceDistribution = pBufferB;
                    bToggle = true;
                }

                if (NT_SUCCESS(NtQuerySystemInformation(SystemProcessorPerformanceDistribution, pCurrentPerformanceDistribution, iBufferSize, 0)) &&
                        pCurrentPerformanceDistribution->ProcessorCount == dwNumberOfProcessors &&
                        pPreviousPerformanceDistribution->ProcessorCount == dwNumberOfProcessors)
                {
                    static SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION *stateDistribution;
                    static SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION *stateDifference;

                    for (iTempInt = 0; iTempInt < dwNumberOfProcessors; ++iTempInt)
                    {
                        stateDistribution = fByteToSppsd(fSppdToByte(pCurrentPerformanceDistribution) + pCurrentPerformanceDistribution->Offsets[iTempInt]);
                        stateDifference = fByteToSppsd(pDifferences + iStateSize*iTempInt);
                        if (stateDistribution->StateCount == 2)
                        {
                            if (wWinVer >= eWin8_1)
                            {
                                stateDifference->States[0] = stateDistribution->States[0];
                                stateDifference->States[1] = stateDistribution->States[1];
                            }
                            else
                            {
                                static const SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8 *hitCountOld;
                                hitCountOld = fSpphToSpphw(stateDistribution->States);
                                stateDifference->States[0].Hits.QuadPart = hitCountOld->Hits;
                                stateDifference->States[0].PercentFrequency = hitCountOld->PercentFrequency;
                                hitCountOld = fByteToSpphw(fSpphToByte(stateDistribution->States) + sizeof(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8));
                                stateDifference->States[1].Hits.QuadPart = hitCountOld->Hits;
                                stateDifference->States[1].PercentFrequency = hitCountOld->PercentFrequency;
                            }
                        }
                        else
                        {
                            iTempInt = 0;
                            break;
                        }
                    }

                    if (iTempInt)
                    {
                        for (iTempInt = 0; iTempInt < dwNumberOfProcessors; ++iTempInt)
                        {
                            stateDistribution = fByteToSppsd(fSppdToByte(pPreviousPerformanceDistribution) + pPreviousPerformanceDistribution->Offsets[iTempInt]);
                            stateDifference = fByteToSppsd(pDifferences + iStateSize*iTempInt);
                            if (stateDistribution->StateCount == 2)
                            {
                                if (wWinVer >= eWin8_1)
                                {
                                    stateDifference->States[0].Hits.QuadPart -= stateDistribution->States[0].Hits.QuadPart;
                                    stateDifference->States[1].Hits.QuadPart -= stateDistribution->States[1].Hits.QuadPart;
                                }
                                else
                                {
                                    stateDifference->States[0].Hits.QuadPart -= fSpphToSpphw(stateDistribution->States)->Hits;
                                    stateDifference->States[1].Hits.QuadPart -= fByteToSpphw(fSpphToByte(stateDistribution->States) + sizeof(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8))->Hits;
                                }
                            }
                            else
                            {
                                assert(false);
                                iTempInt = 0;
                                break;
                            }
                        }

                        if (iTempInt)
                        {
                            iTempTotal = 0;
                            for (iTempInt = 0; iTempInt < dwNumberOfProcessors; ++iTempInt)
                            {
                                stateDifference = fByteToSppsd(pDifferences + iStateSize*iTempInt);
                                iTempCount += stateDifference->States[0].Hits.QuadPart + stateDifference->States[1].Hits.QuadPart;
                                iTempTotal += stateDifference->States[0].Hits.QuadPart*stateDifference->States[0].PercentFrequency +
                                        stateDifference->States[1].Hits.QuadPart*stateDifference->States[1].PercentFrequency;
                            }
                        }
                    }
                }
            }

            iTemp = iTempCount ? pPowerInformation[0].MaxMhz*iTempTotal/(iTempCount*1000) : pPowerInformation[0].CurrentMhz/10;
            assert(iTemp >= 10 && iTemp <= 9900);
            if (iTemp != iCpuSpeedOld)
            {
                iCpuSpeedOld = iTemp;
                iTempInt = iCpuSpeedOld/100;

                wchar_t *pDelim = pCpuSpeed + 1;
                if (iTempInt < 10)
                    *pCpuSpeed = L'0' + iTempInt;
                else
                {
                    pCpuSpeed[0] = L'0' + iTempInt/10;
                    pCpuSpeed[1] = L'0' + iTempInt%10;
                    ++pCpuSpeed;
                }
                *pDelim = L'.';
                iTemp -= iTempInt*100;
                pDelim[1] = L'0' + iTemp/10;
                pDelim[2] = L'0' + iTemp%10;
                wcscpy(pDelim+3, pCpuSpeedAll);
                SetWindowText(hWndCpuSpeed, pCpuSpeed);
            }
        }
        return 0;
    }
    case WM_WINDOWPOSCHANGING:
    {
        static RECT rectArea;
        if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rectArea, 0))
        {
            WINDOWPOS *const wndPos = reinterpret_cast<WINDOWPOS*>(lParam);
            if (wndPos->x < rectArea.left)
                wndPos->x = rectArea.left;
            else if (wndPos->x + wndPos->cx > rectArea.right)
                wndPos->x = rectArea.right - wndPos->cx;

            if (wndPos->y < rectArea.top)
                wndPos->y = rectArea.top;
            else if (wndPos->y + wndPos->cy > rectArea.bottom)
                wndPos->y = rectArea.bottom - wndPos->cy;
        }
        return 0;
    }
    case WM_ENDSESSION:
    {
        SendMessage(hWnd, WM_CLOSE, 0, 0);
        return 0;
    }
    case WM_DESTROY:
    {
        if (SetTimer(hWnd, 1, 5000, 0))
            KillTimer(hWnd, 1);
        if (hWinEventHook)
            UnhookWinEvent(hWinEventHook);
        if (hbrBackground)
            DeleteObject(hbrBackground);
        if (hbrLoad)
            DeleteObject(hbrLoad);
        if (hFont)
            DeleteObject(hFont);
        free(pPowerInformation);
        free(pDifferences);
        free(pBufferA);
        free(pRunApp);
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//-------------------------------------------------------------------------------------------------
int main()
{
    if (const HWND hWnd = FindWindow(g_wGuidClass, 0))
        PostMessage(hWnd, WM_CLOSE, 0, 0);

    INITCOMMONCONTROLSEX initComCtrlEx;
    initComCtrlEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
    initComCtrlEx.dwICC = ICC_STANDARD_CLASSES;
    if (InitCommonControlsEx(&initComCtrlEx) == TRUE)
    {
        wchar_t wBuf[MAX_PATH+MAX_PATH+136];//[999000/999000 MB`999000/999000 MB`99.00/99.00 Ghz`Processes: 99000`Uptime: 9999d 23h 59m`/999000 MB`/999000 MB`/99.00 Ghz`100%`WorkerW`?]
        const DWORD dwLen = GetModuleFileName(0, wBuf, MAX_PATH+1);
        if (dwLen >= 8 && dwLen < MAX_PATH)
            if (const wchar_t *const pDelimPath = wcsrchr(wBuf, L'\\'))
                if (pDelimPath <= wBuf + MAX_PATH-1-28/*\SystemMonitorSettings96.exe*/)
                {
                    WNDCLASS wndCl;
                    wndCl.style = 0;
                    wndCl.lpfnWndProc = WindowProc;
                    wndCl.cbClsExtra = 0;
                    wndCl.cbWndExtra = 0;
                    wndCl.hInstance = GetModuleHandle(0);
                    wndCl.hIcon = 0;
                    wndCl.hCursor = LoadCursor(0, IDC_ARROW);
                    wndCl.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1);
                    wndCl.lpszMenuName = 0;
                    wndCl.lpszClassName = g_wGuidClass;

                    if (RegisterClass(&wndCl))
                    {
                        PAINTSTRUCT ps;
                        pPs = &ps;        //***it's ok
                        if (const HWND hWndParent = CreateWindowEx(0, WC_BUTTON, 0, 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, 0))
                        {
                            wcscpy(wBuf+dwLen-3, L"ini");
                            wcscpy(wBuf+MAX_PATH, wBuf);
                            wcscpy(wBuf+MAX_PATH+1+(pDelimPath-wBuf), g_wSettingsApp);

                            MEMORYSTATUSEX memStatusEx;
                            memStatusEx.dwLength = sizeof(MEMORYSTATUSEX);
                            FILETIME fts[3];
                            PROCESS_INFORMATION pi;
                            STARTUPINFO si;
                            memset(&si, 0, sizeof(STARTUPINFO));
                            si.cb = sizeof(STARTUPINFO);
                            SYSTEM_TIMEOFDAY_INFORMATION sysTimeInfo;
                            PROCESSENTRY32 processEntry32;
                            processEntry32.dwSize = sizeof(PROCESSENTRY32);
                            RECT rectGeometry;
                            TagCreateParams tgCreateParams;
                            tgCreateParams.pBufPath = wBuf;
                            tgCreateParams.pMemStatusEx = &memStatusEx;
                            tgCreateParams.pFts = fts;
                            tgCreateParams.pPi = &pi;
                            tgCreateParams.pSi = &si;
                            tgCreateParams.pSysTimeInfo = &sysTimeInfo;
                            tgCreateParams.pProcessEntry32 = &processEntry32;
                            tgCreateParams.pWndCl = &wndCl;
                            tgCreateParams.pRectGeometry = &rectGeometry;
                            if (CreateWindowEx(0, g_wGuidClass, L"SystemMonitor", 0, 0, 0, 0, 0, hWndParent, 0, wndCl.hInstance, &tgCreateParams) &&
                                    SetWindowPos(hWndApp, HWND_BOTTOM, rectGeometry.left, rectGeometry.top, rectGeometry.right, rectGeometry.bottom, SWP_SHOWWINDOW | SWP_NOACTIVATE))
                            {
                                PostMessage(hWndApp, WM_TIMER, 0, 0);
                                MSG msg;
                                while (GetMessage(&msg, 0, 0, 0) > 0)
                                    DispatchMessage(&msg);
                            }
                            SendMessage(hWndParent, WM_CLOSE, 0, 0);
                        }
                        if (!wndCl.lpszClassName)
                            UnregisterClass(g_wGuidClassProgress, wndCl.hInstance);
                        UnregisterClass(g_wGuidClass, wndCl.hInstance);
                    }
                }
    }
    return 0;
}
