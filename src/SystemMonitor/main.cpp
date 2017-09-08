//SystemMonitor
#define _WIN32_WINNT _WIN32_IE_WINBLUE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ntdef.h>
#include <ntstatus.h>
#include <commctrl.h>

#define IMPORT __declspec(dllimport)

static constexpr const wchar_t *const g_wGuidClass = L"App::ec19dd9c-939a-4b05-c2fb-309ff78bcf32";
static constexpr const wchar_t *const g_wGuidClassProgress = L"Progress::ec19dd9c-939a-4b05-c2fb-309ff78bcf32";
static constexpr const wchar_t *const g_wSettingsApp = L"SystemMonitorSettings.exe\"";
static constexpr const ULONGLONG g_iTicksPerMus = 10;        //1 tick = 100NS
static constexpr const ULONGLONG g_iTicksPerMs = g_iTicksPerMus*1000;
static constexpr const ULONGLONG g_iTicksPerSec = g_iTicksPerMs*1000;
static constexpr const ULONGLONG g_iTicksPerMin = g_iTicksPerSec*60;
static constexpr const ULONGLONG g_iTicksPerHour = g_iTicksPerMin*60;
static constexpr const ULONGLONG g_iTicksPerDay = g_iTicksPerHour*24;
static constexpr const DWORD g_dwBufferSizeProcInfo = 0x100000;
static HWND g_hWndApp, g_hWndPrbPhysical, g_hWndPrbVirtual, g_hWndPrbCpu, g_hWndCpuSpeed;
static ULONG g_iPhysicalPercentOld, g_iVirtualPercentOld, g_iCpuLoadOld, g_iCpuSpeedOld, g_iCpuSpeedAll;
static wchar_t *g_pPercents, *g_pClassName;
static RECT g_rectBackground, g_rectLoad;
static HBRUSH g_hbrBackground, g_hbrLoad, g_hbrSysBackground;
static PAINTSTRUCT *g_pPs;
static HFONT g_hFont;

//-------------------------------------------------------------------------------------------------
#ifdef NDEBUG
#define ___assert___(cond) do{static_cast<void>(sizeof(cond));}while(false)
#else
#define ___assert___(cond) do{if(!(cond)){int i=__LINE__;char h[]="RUNTIME ASSERTION. Line:           "; \
    if(i>=0){char *c=h+35;do{*--c=i%10+'0';i/=10;}while(i>0);} \
    if(MessageBoxA(nullptr,__FILE__,h,MB_ICONERROR|MB_OKCANCEL)==IDCANCEL)ExitProcess(0);}}while(false)
#endif

template<typename T1, typename T2>
inline T1 pointer_cast(T2 *const pSrc)
{return static_cast<T1>(static_cast<void*>(pSrc));}

template<typename T1, typename T2>
inline T1 pointer_cast(const T2 *const pSrc)
{return static_cast<T1>(static_cast<const void*>(pSrc));}

static inline void FZeroMemory(void *const pDst__, DWORD dwSize)
{
    BYTE *pDst = static_cast<BYTE*>(pDst__);
    while (dwSize--)
        *pDst++ = '\0';
}

static inline void FCopyMemoryW(wchar_t *pDst, const wchar_t *pSrc)
{
    while ((*pDst++ = *pSrc++));
}

static inline wchar_t * FStrChrWNull(wchar_t *pSrc)
{
    while (*pSrc)
        ++pSrc;
    return pSrc;
}

static inline void FCopyMemoryWEnd(wchar_t *pDst, const wchar_t *pSrc)
{
    while (*pDst)
        ++pDst;
    while ((*pDst++ = *pSrc++));
}

static void FNumToStrW(DWORD dwValue, wchar_t *pBuf)
{
    wchar_t *pIt = pBuf;
    do
    {
        *pIt++ = dwValue%10 + L'0';
        dwValue /= 10;
    } while (dwValue > 0);
    *pIt-- = L'\0';
    do
    {
        const wchar_t wTemp = *pIt;
        *pIt = *pBuf;
        *pBuf = wTemp;
    } while (++pBuf < --pIt);
}

//-------------------------------------------------------------------------------------------------
typedef enum _SYSTEM_INFORMATION_CLASS
{
    SystemTimeOfDayInformation = 3,
    SystemProcessInformation = 5,
    SystemProcessorPerformanceDistribution = 100
} SYSTEM_INFORMATION_CLASS;

extern "C" IMPORT NTSTATUS NTAPI NtPowerInformation
(POWER_INFORMATION_LEVEL InformationLevel, PVOID lpInputBuffer, ULONG nInputiBufferSize, PVOID lpOutputBuffer, ULONG nOutputBufferSize);

extern "C" IMPORT NTSTATUS NTAPI NtQuerySystemInformation
(SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);

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

typedef struct _SYSTEM_PROCESS_INFORMATION
{
    ULONG NextEntryOffset;
    //...
} SYSTEM_PROCESS_INFORMATION;

struct SSaveSettings
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
    wchar_t wAppPath[MAX_PATH+2];
};

struct SCreateParams
{
    wchar_t *pBufPath;
    MEMORYSTATUSEX *pMemStatusEx;
    ULONGLONG *pFts;
    PROCESS_INFORMATION *pPi;
    STARTUPINFO *pSi;
    SYSTEM_TIMEOFDAY_INFORMATION *pSysTimeInfo;
    WNDCLASSEX *pWndCl;
    RECT *pRectGeometry;
    SSaveSettings *sSave;
};

//-------------------------------------------------------------------------------------------------
static void CALLBACK WinEventProc(HWINEVENTHOOK, DWORD, HWND hWnd, LONG, LONG, DWORD, DWORD)
{
    if (GetClassNameW(hWnd, g_pClassName, 9/*"WorkerW`?"*/) == 7 &&
            g_pClassName[0] == L'W' &&
            g_pClassName[1] == L'o' &&
            g_pClassName[2] == L'r' &&
            g_pClassName[3] == L'k' &&
            g_pClassName[4] == L'e' &&
            g_pClassName[5] == L'r' &&
            g_pClassName[6] == L'W' &&
            g_pClassName[7] == L'\0')
    {
        int i = 5;        //repeat several times
        do
        {
            SetWindowPos(g_hWndApp, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            SetWindowPos(g_hWndApp, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
        } while (--i);
    }
}

//-------------------------------------------------------------------------------------------------
static LRESULT CALLBACK WindowProcProgress(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_PAINT)
    {
        ___assert___(hWnd == g_hWndPrbPhysical || hWnd == g_hWndPrbVirtual || hWnd == g_hWndPrbCpu);
        const ULONG iValue = (hWnd == g_hWndPrbCpu ? g_iCpuLoadOld : (hWnd == g_hWndPrbPhysical ? g_iPhysicalPercentOld : g_iVirtualPercentOld));
        ___assert___(iValue <= 100);
        g_rectLoad.right = (g_rectBackground.right - 1/*fix right border*/)*iValue/100 + 1/*left border*/;
        int iLength;
        if (iValue < 10)
        {
            g_pPercents[0] = L'0' + iValue;
            g_pPercents[1] = L'%';
            iLength = 2;
        }
        else if (iValue < 100)
        {
            g_pPercents[0] = L'0' + iValue/10;
            g_pPercents[1] = L'0' + iValue%10;
            g_pPercents[2] = L'%';
            iLength = 3;
        }
        else
        {
            FCopyMemoryW(g_pPercents, L"100%");
            iLength = 4;
        }
        if (const HDC hDc = BeginPaint(hWnd, g_pPs))
        {
            FillRect(hDc, &g_rectBackground, g_hbrBackground);
            FillRect(hDc, &g_rectLoad, g_hbrLoad);
            SelectObject(hDc, g_hFont);
            SetBkMode(hDc, TRANSPARENT);
            DrawTextW(hDc, g_pPercents, iLength, &g_rectBackground, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
            EndPaint(hWnd, g_pPs);
        }
        return 0;
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

//-------------------------------------------------------------------------------------------------
static LRESULT CALLBACK WindowProcStaticCpuSpeed(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR)
{
    if (uMsg == WM_CTLCOLORSTATIC)
    {
        if (reinterpret_cast<HWND>(lParam) == g_hWndCpuSpeed)
        {
            SetBkMode(reinterpret_cast<HDC>(wParam), TRANSPARENT);
            if (g_iCpuSpeedOld > g_iCpuSpeedAll)        //Turbo Boost (Turbo Core)
                SetTextColor(reinterpret_cast<HDC>(wParam), RGB(255, 0, 0));
            return reinterpret_cast<LRESULT>(g_hbrSysBackground);
        }
    }
    else if (uMsg == WM_NCDESTROY)
        RemoveWindowSubclass(hWnd, WindowProcStaticCpuSpeed, uIdSubclass);
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

//-------------------------------------------------------------------------------------------------
static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    enum
    {
        eRunApp = 1,
        eUptime,
        eSavePos,
        eStg,
        eClose,
    };

    static constexpr const DWORD dwMagic = 0x29D6036C;
    static constexpr const ULONG iStateSize = FIELD_OFFSET(SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION, States) + 2*sizeof(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT);
    ___assert___(iStateSize == 40);
    ___assert___(_WIN32_WINNT_WINBLUE == 0x0603);

    static HWINEVENTHOOK hWinEventHook;
    static DWORD dwNumberOfProcessors, dwProcessorsSize;
    static PROCESSOR_POWER_INFORMATION *pPowerInformation;
    static BYTE *pDifferences;
    static ULONG iBufferSize;
    static SYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION *pBufferA, *pBufferB;
    static SYSTEM_PROCESS_INFORMATION *pBufferProcInfo;
    static HICON hIconRunApp, hIconUptime, hIconSavePos, hIconStg, hIconClose;
    static HWND hWndPhysical, hWndVirtual, hWndProcUptime;
    static wchar_t *pPath, *pPhysical, *pVirtual, *pCpuSpeed, *pProcesses, *pUptime,
            *pPhysicalAll, *pVirtualAll, *pCpuSpeedAll;
    static SYSTEM_TIMEOFDAY_INFORMATION *pSysTimeInfo;
    static PROCESS_INFORMATION *pPi;
    static STARTUPINFO *pSi;
    static MEMORYSTATUSEX *pMemStatusEx;
    static ULONGLONG *pDwIdleTime, *pDwKernelTime, *pDwUserTime;
    static WORD wWinVer;
    static RECT rectWindow;
    static ULONG iProcessesOld;
    static SSaveSettings *pTgSave;

    switch (uMsg)
    {
    case WM_CREATE:
    {
        const CREATESTRUCT *const crStruct = reinterpret_cast<const CREATESTRUCT*>(lParam);
        const SCreateParams *tgCreateParams = static_cast<const SCreateParams*>(crStruct->lpCreateParams);
        pPath = tgCreateParams->pBufPath;
        pPi = tgCreateParams->pPi;
        pSi = tgCreateParams->pSi;
        const HANDLE hFile = CreateFileW(pPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            pTgSave = tgCreateParams->sSave;
            bool bOk = false;
            DWORD dwBytes;
            LARGE_INTEGER iFileSize;
            if (GetFileSizeEx(hFile, &iFileSize) && iFileSize.QuadPart == sizeof(SSaveSettings) &&
                    ReadFile(hFile, pTgSave, sizeof(SSaveSettings), &dwBytes, nullptr) && dwBytes == sizeof(SSaveSettings))
                bOk = true;
            CloseHandle(hFile);

            if (bOk && pTgSave->dwMagic == dwMagic &&
                    pTgSave->iUpdateInterval >= 1000 && pTgSave->iUpdateInterval <= 30000 &&
                    pTgSave->iOpacity &&
                    pTgSave->iThemeMargin <= 1000 &&
                    pTgSave->iThemeGap <= 1000 &&
                    pTgSave->iProgressbarWidth <= 10000 &&
                    pTgSave->iProgressbarHeight <= 1000 &&
                    pTgSave->iLabelHeight <= 1000 &&
                    pTgSave->wAppPath[MAX_PATH+2-1] == L'\0' &&
                    (g_hFont = CreateFontIndirectW(&pTgSave->logFont)))
                if (const HANDLE hProcHeap = GetProcessHeap())
                {
                    SYSTEM_INFO sysInfo;
                    GetNativeSystemInfo(&sysInfo);
                    dwNumberOfProcessors = sysInfo.dwNumberOfProcessors;
                    dwProcessorsSize = dwNumberOfProcessors*sizeof(PROCESSOR_POWER_INFORMATION);
                    if ((pPowerInformation = static_cast<PROCESSOR_POWER_INFORMATION*>(HeapAlloc(hProcHeap, HEAP_NO_SERIALIZE, dwProcessorsSize))))
                    {
                        wWinVer = GetVersion();
                        wWinVer = wWinVer << 8 | wWinVer >> 8;
                        if ((wWinVer < _WIN32_WINNT_VISTA ||
                             ((pDifferences = static_cast<BYTE*>(HeapAlloc(hProcHeap, HEAP_NO_SERIALIZE, iStateSize*dwNumberOfProcessors))) &&
                              NtQuerySystemInformation(SystemProcessorPerformanceDistribution, nullptr, 0, &iBufferSize) == STATUS_INFO_LENGTH_MISMATCH &&
                              ((pBufferA = static_cast<SYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION*>(HeapAlloc(hProcHeap, HEAP_NO_SERIALIZE, iBufferSize*2)))) &&
                              NT_SUCCESS(NtQuerySystemInformation(SystemProcessorPerformanceDistribution, pBufferA, iBufferSize, nullptr)))) &&
                                (pBufferProcInfo = static_cast<SYSTEM_PROCESS_INFORMATION*>(HeapAlloc(hProcHeap, HEAP_NO_SERIALIZE, g_dwBufferSizeProcInfo))))
                        {
                            pBufferB = pointer_cast<SYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION*>(pointer_cast<BYTE*>(pBufferA) + iBufferSize);

                            //[MAX_PATH+MAX_PATH+136] = [999000/999000 MB`999000/999000 MB`99.00/99.00 Ghz`Processes: 99000`Uptime: 9999d 23h 59m`/999000 MB`/999000 MB`/99.00 Ghz`100%`WorkerW`?]
                            pPhysical = pPath+MAX_PATH*2+2;
                            pVirtual = pPath+MAX_PATH*2+2+17;
                            pCpuSpeed = pPath+MAX_PATH*2+2+34;
                            pProcesses = pPath+MAX_PATH*2+2+50;
                            pUptime = pPath+MAX_PATH*2+2+67;
                            pPhysicalAll = pPath+MAX_PATH*2+2+89;
                            pVirtualAll = pPath+MAX_PATH*2+2+100;
                            pCpuSpeedAll = pPath+MAX_PATH*2+2+111;
                            g_pPercents = pPath+MAX_PATH*2+2+122;
                            g_pClassName = pPath+MAX_PATH*2+2+127;

                            FCopyMemoryW(pProcesses, L"Processes: ");
                            FCopyMemoryW(pUptime, L"Uptime: ");
                            *pPhysicalAll = L'/';
                            *pVirtualAll = L'/';
                            *pCpuSpeedAll = L'/';

                            pMemStatusEx = tgCreateParams->pMemStatusEx;
                            pDwIdleTime = tgCreateParams->pFts;
                            pDwKernelTime = pDwIdleTime+1;
                            pDwUserTime = pDwIdleTime+2;
                            pSysTimeInfo = tgCreateParams->pSysTimeInfo;

                            if (GlobalMemoryStatusEx(pMemStatusEx) && NT_SUCCESS(NtPowerInformation(ProcessorInformation, nullptr, 0, pPowerInformation, dwProcessorsSize)))
                            {
                                const HINSTANCE hInst = crStruct->hInstance;
                                WNDCLASSEX *const pWndCl = tgCreateParams->pWndCl;
                                pWndCl->lpfnWndProc = WindowProcProgress;
                                pWndCl->hbrBackground = reinterpret_cast<HBRUSH>(COLOR_ACTIVEBORDER+1);
                                pWndCl->lpszClassName = g_wGuidClassProgress;
                                if (RegisterClassExW(pWndCl) && (pWndCl->lpszClassName = nullptr/*check for unregister at end*/, (g_hbrBackground = CreateSolidBrush(RGB(230, 230, 230)))) && (g_hbrLoad = CreateSolidBrush(pTgSave->colorRef)) &&
                                        (hIconRunApp = static_cast<HICON>(LoadImageW(hInst, L"IDI_ICON2", IMAGE_ICON, 16, 16, 0))) &&
                                        (hIconUptime = static_cast<HICON>(LoadImageW(hInst, L"IDI_ICON3", IMAGE_ICON, 16, 16, 0))) &&
                                        (hIconSavePos = static_cast<HICON>(LoadImageW(hInst, L"IDI_ICON4", IMAGE_ICON, 16, 16, 0))) &&
                                        (hIconStg = static_cast<HICON>(LoadImageW(hInst, L"IDI_ICON5", IMAGE_ICON, 16, 16, 0))) &&
                                        (hIconClose = static_cast<HICON>(LoadImageW(hInst, L"IDI_ICON6", IMAGE_ICON, 16, 16, 0))))
                                    if (const HWND hWndBtnRunApp = CreateWindowExW(WS_EX_NOACTIVATE, WC_BUTTON, nullptr, WS_CHILD | WS_VISIBLE | BS_ICON, pTgSave->iThemeMargin, pTgSave->iThemeMargin, 16, 16, hWnd, reinterpret_cast<HMENU>(eRunApp), hInst, nullptr))
                                        if (const HWND hWndBtnUptime = CreateWindowExW(WS_EX_NOACTIVATE, WC_BUTTON, nullptr, WS_CHILD | WS_VISIBLE | BS_ICON, pTgSave->iThemeMargin+16, pTgSave->iThemeMargin, 16, 16, hWnd, reinterpret_cast<HMENU>(eUptime), hInst, nullptr))
                                        {
                                            const int iWidth = pTgSave->iThemeMargin*3+pTgSave->iProgressbarWidth;
                                            if (const HWND hWndBtnSavePos = CreateWindowExW(WS_EX_NOACTIVATE, WC_BUTTON, nullptr, WS_CHILD | WS_VISIBLE | BS_ICON, iWidth-48, pTgSave->iThemeMargin, 16, 16, hWnd, reinterpret_cast<HMENU>(eSavePos), hInst, nullptr))
                                                if (const HWND hWndBtnStg = CreateWindowExW(WS_EX_NOACTIVATE, WC_BUTTON, nullptr, WS_CHILD | WS_VISIBLE | BS_ICON, iWidth-32, pTgSave->iThemeMargin, 16, 16, hWnd, reinterpret_cast<HMENU>(eStg), hInst, nullptr))
                                                    if (const HWND hWndBtnClose = CreateWindowExW(WS_EX_NOACTIVATE, WC_BUTTON, nullptr, WS_CHILD | WS_VISIBLE | BS_ICON, iWidth-16, pTgSave->iThemeMargin, 16, 16, hWnd, reinterpret_cast<HMENU>(eClose), hInst, nullptr))
                                                    {
                                                        g_rectBackground.left = 1;
                                                        g_rectBackground.top = 1;
                                                        g_rectBackground.right = pTgSave->iProgressbarWidth-1;
                                                        g_rectBackground.bottom = pTgSave->iProgressbarHeight-1;
                                                        g_rectLoad = g_rectBackground;
                                                        const int iGbMemoryHeight = pTgSave->iThemeGap+pTgSave->iLabelHeight*2+pTgSave->iProgressbarHeight*2+pTgSave->iThemeMargin*3;
                                                        if (const HWND hWndGbMemory = CreateWindowExW(0, WC_BUTTON, L"Memory", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, pTgSave->iThemeMargin, pTgSave->iThemeMargin*2+16, pTgSave->iThemeMargin*2+pTgSave->iProgressbarWidth, iGbMemoryHeight, hWnd, nullptr, hInst, nullptr))
                                                            if (const HWND hWndStPhysical = CreateWindowExW(0, WC_STATIC, L"Physic:\0\0", WS_CHILD | WS_VISIBLE, pTgSave->iThemeMargin, pTgSave->iThemeGap, pTgSave->iProgressbarWidth/3, pTgSave->iLabelHeight, hWndGbMemory, nullptr, hInst, nullptr))
                                                                if ((hWndPhysical = CreateWindowExW(0, WC_STATIC, nullptr, WS_CHILD | WS_VISIBLE | SS_RIGHT, pTgSave->iThemeMargin+pTgSave->iProgressbarWidth/3, pTgSave->iThemeGap, pTgSave->iProgressbarWidth*2/3, pTgSave->iLabelHeight, hWndGbMemory, nullptr, hInst, nullptr)) &&
                                                                        (g_hWndPrbPhysical = CreateWindowExW(0, g_wGuidClassProgress, nullptr, WS_CHILD | WS_VISIBLE, pTgSave->iThemeMargin, pTgSave->iThemeGap+pTgSave->iLabelHeight, pTgSave->iProgressbarWidth, pTgSave->iProgressbarHeight, hWndGbMemory, nullptr, hInst, nullptr)))
                                                                    if (const HWND hWndStVirtual = CreateWindowExW(0, WC_STATIC, L"Virtual:", WS_CHILD | WS_VISIBLE, pTgSave->iThemeMargin, pTgSave->iThemeGap+pTgSave->iLabelHeight+pTgSave->iProgressbarHeight+pTgSave->iThemeMargin*2, pTgSave->iProgressbarWidth/3, pTgSave->iLabelHeight, hWndGbMemory, nullptr, hInst, nullptr))
                                                                        if ((hWndVirtual = CreateWindowExW(0, WC_STATIC, nullptr, WS_CHILD | WS_VISIBLE | SS_RIGHT, pTgSave->iThemeMargin+pTgSave->iProgressbarWidth/3, pTgSave->iThemeGap+pTgSave->iLabelHeight+pTgSave->iProgressbarHeight+pTgSave->iThemeMargin*2, pTgSave->iProgressbarWidth*2/3, pTgSave->iLabelHeight, hWndGbMemory, nullptr, hInst, nullptr)) &&
                                                                                (g_hWndPrbVirtual = CreateWindowExW(0, g_wGuidClassProgress, nullptr, WS_CHILD | WS_VISIBLE, pTgSave->iThemeMargin, pTgSave->iThemeGap+pTgSave->iProgressbarHeight+pTgSave->iLabelHeight*2+pTgSave->iThemeMargin*2, pTgSave->iProgressbarWidth, pTgSave->iProgressbarHeight, hWndGbMemory, nullptr, hInst, nullptr)))
                                                                        {
                                                                            const int iGbCpuHeight = pTgSave->iThemeGap+pTgSave->iLabelHeight*2+pTgSave->iProgressbarHeight+pTgSave->iThemeMargin*2;
                                                                            if (const HWND hWndGbCpu = CreateWindowExW(0, WC_BUTTON, L"CPU", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, pTgSave->iThemeMargin, pTgSave->iThemeMargin*3+16+iGbMemoryHeight, pTgSave->iThemeMargin*2+pTgSave->iProgressbarWidth, iGbCpuHeight, hWnd, nullptr, hInst, nullptr))
                                                                                if (const HWND hWndStLoad = CreateWindowExW(0, WC_STATIC, L"Load:", WS_CHILD | WS_VISIBLE, pTgSave->iThemeMargin, pTgSave->iThemeGap, pTgSave->iProgressbarWidth, pTgSave->iLabelHeight, hWndGbCpu, nullptr, hInst, nullptr))
                                                                                    if ((g_hWndPrbCpu = CreateWindowExW(0, g_wGuidClassProgress, nullptr, WS_CHILD | WS_VISIBLE, pTgSave->iThemeMargin, pTgSave->iThemeGap+pTgSave->iLabelHeight, pTgSave->iProgressbarWidth, pTgSave->iProgressbarHeight, hWndGbCpu, nullptr, hInst, nullptr)))
                                                                                        if (const HWND hWndStCpuSpeed = CreateWindowExW(0, WC_STATIC, L"Speed:", WS_CHILD | WS_VISIBLE, pTgSave->iThemeMargin, pTgSave->iThemeGap+pTgSave->iLabelHeight+pTgSave->iProgressbarHeight+pTgSave->iThemeMargin, pTgSave->iProgressbarWidth/3, pTgSave->iLabelHeight, hWndGbCpu, nullptr, hInst, nullptr))
                                                                                            if ((g_hWndCpuSpeed = CreateWindowExW(0, WC_STATIC, nullptr, WS_CHILD | WS_VISIBLE | SS_RIGHT, pTgSave->iThemeMargin+pTgSave->iProgressbarWidth/3, pTgSave->iThemeGap+pTgSave->iLabelHeight+pTgSave->iProgressbarHeight+pTgSave->iThemeMargin, pTgSave->iProgressbarWidth*2/3, pTgSave->iLabelHeight, hWndGbCpu, nullptr, hInst, nullptr)) &&
                                                                                                    (wWinVer < _WIN32_WINNT_VISTA || ((g_hbrSysBackground = GetSysColorBrush(COLOR_BTNFACE)) && SetWindowSubclass(hWndGbCpu, WindowProcStaticCpuSpeed, 1, 0) == TRUE)))
                                                                                            {
                                                                                                const int iHeight = pTgSave->iThemeMargin*4+16+iGbMemoryHeight+iGbCpuHeight;
                                                                                                if ((hWndProcUptime = CreateWindowExW(0, WC_STATIC, nullptr, WS_CHILD | WS_VISIBLE, pTgSave->iThemeMargin*2, iHeight, pTgSave->iProgressbarWidth, pTgSave->iLabelHeight, hWnd, nullptr, hInst, nullptr)))
                                                                                                    if (HWND hWndTooltip = CreateWindowExW(0, TOOLTIPS_CLASS, nullptr, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, nullptr, hInst, nullptr))
                                                                                                        if (SetWindowPos(hWndTooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE))
                                                                                                        {
                                                                                                            wchar_t wBufTooltips[] = L"Run app\0Uptime\0Save position\0Settings\0Close";
                                                                                                            TOOLINFOW toolInfo;
                                                                                                            toolInfo.cbSize = sizeof(TOOLINFOW);
                                                                                                            toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
                                                                                                            toolInfo.hwnd = hWnd;
                                                                                                            toolInfo.uId = reinterpret_cast<UINT_PTR>(hWndBtnRunApp);
                                                                                                            toolInfo.rect = rectWindow;
                                                                                                            toolInfo.hinst = hInst;
                                                                                                            toolInfo.lpszText = wBufTooltips;
                                                                                                            toolInfo.lParam = 0;
                                                                                                            toolInfo.lpReserved = nullptr;
                                                                                                            if (SendMessageW(hWndTooltip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo)) == TRUE)
                                                                                                                if ((hWndTooltip = CreateWindowExW(0, TOOLTIPS_CLASS, nullptr, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, nullptr, hInst, nullptr)))
                                                                                                                    if (SetWindowPos(hWndTooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE))
                                                                                                                    {
                                                                                                                        toolInfo.uId = reinterpret_cast<UINT_PTR>(hWndBtnUptime);
                                                                                                                        toolInfo.lpszText = wBufTooltips+8;
                                                                                                                        if (SendMessageW(hWndTooltip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo)) == TRUE)
                                                                                                                            if ((hWndTooltip = CreateWindowExW(0, TOOLTIPS_CLASS, nullptr, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, nullptr, hInst, nullptr)))
                                                                                                                                if (SetWindowPos(hWndTooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE))
                                                                                                                                {
                                                                                                                                    toolInfo.uId = reinterpret_cast<UINT_PTR>(hWndBtnSavePos);
                                                                                                                                    toolInfo.lpszText = wBufTooltips+15;
                                                                                                                                    if (SendMessageW(hWndTooltip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo)) == TRUE)
                                                                                                                                        if ((hWndTooltip = CreateWindowExW(0, TOOLTIPS_CLASS, nullptr, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, nullptr, hInst, nullptr)))
                                                                                                                                            if (SetWindowPos(hWndTooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE))
                                                                                                                                            {
                                                                                                                                                toolInfo.uId = reinterpret_cast<UINT_PTR>(hWndBtnStg);
                                                                                                                                                toolInfo.lpszText = wBufTooltips+29;
                                                                                                                                                if (SendMessageW(hWndTooltip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo)) == TRUE)
                                                                                                                                                    if ((hWndTooltip = CreateWindowExW(0, TOOLTIPS_CLASS, nullptr, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, nullptr, hInst, nullptr)))
                                                                                                                                                        if (SetWindowPos(hWndTooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE))
                                                                                                                                                        {
                                                                                                                                                            toolInfo.uId = reinterpret_cast<UINT_PTR>(hWndBtnClose);
                                                                                                                                                            toolInfo.lpszText = wBufTooltips+38;
                                                                                                                                                            if (SendMessageW(hWndTooltip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo)) == TRUE)
                                                                                                                                                            {
                                                                                                                                                                RECT *const pRectGeometry = tgCreateParams->pRectGeometry;
                                                                                                                                                                if (GetWindowRect(hWnd, &rectWindow) && GetClientRect(hWnd, pRectGeometry))
                                                                                                                                                                {
                                                                                                                                                                    SendMessageW(hWndGbMemory, WM_SETFONT, reinterpret_cast<WPARAM>(g_hFont), FALSE);
                                                                                                                                                                    SendMessageW(hWndStPhysical, WM_SETFONT, reinterpret_cast<WPARAM>(g_hFont), FALSE);
                                                                                                                                                                    SendMessageW(hWndPhysical, WM_SETFONT, reinterpret_cast<WPARAM>(g_hFont), FALSE);
                                                                                                                                                                    SendMessageW(hWndStVirtual, WM_SETFONT, reinterpret_cast<WPARAM>(g_hFont), FALSE);
                                                                                                                                                                    SendMessageW(hWndVirtual, WM_SETFONT, reinterpret_cast<WPARAM>(g_hFont), FALSE);
                                                                                                                                                                    SendMessageW(hWndGbCpu, WM_SETFONT, reinterpret_cast<WPARAM>(g_hFont), FALSE);
                                                                                                                                                                    SendMessageW(hWndStLoad, WM_SETFONT, reinterpret_cast<WPARAM>(g_hFont), FALSE);
                                                                                                                                                                    SendMessageW(hWndStCpuSpeed, WM_SETFONT, reinterpret_cast<WPARAM>(g_hFont), FALSE);
                                                                                                                                                                    SendMessageW(g_hWndCpuSpeed, WM_SETFONT, reinterpret_cast<WPARAM>(g_hFont), FALSE);
                                                                                                                                                                    SendMessageW(hWndProcUptime, WM_SETFONT, reinterpret_cast<WPARAM>(g_hFont), FALSE);

                                                                                                                                                                    SendMessageW(hWndBtnRunApp, BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(hIconRunApp));
                                                                                                                                                                    SendMessageW(hWndBtnUptime, BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(hIconUptime));
                                                                                                                                                                    SendMessageW(hWndBtnSavePos, BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(hIconSavePos));
                                                                                                                                                                    SendMessageW(hWndBtnStg, BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(hIconStg));
                                                                                                                                                                    SendMessageW(hWndBtnClose, BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(hIconClose));

                                                                                                                                                                    if (SetTimer(hWnd, 1, pTgSave->iUpdateInterval, nullptr))
                                                                                                                                                                    {
                                                                                                                                                                        pRectGeometry->left = pTgSave->iDx;
                                                                                                                                                                        pRectGeometry->top = pTgSave->iDy;
                                                                                                                                                                        pRectGeometry->right = pTgSave->iThemeMargin*4+pTgSave->iProgressbarWidth+(rectWindow.right - pRectGeometry->right);
                                                                                                                                                                        pRectGeometry->bottom = iHeight+pTgSave->iLabelHeight+pTgSave->iThemeMargin+(rectWindow.bottom - pRectGeometry->bottom);
                                                                                                                                                                        ___assert___(rectWindow.left == 0 && rectWindow.top == 0);
                                                                                                                                                                        rectWindow.right = pTgSave->iProgressbarWidth;
                                                                                                                                                                        rectWindow.bottom = pTgSave->iProgressbarHeight;

                                                                                                                                                                        FNumToStrW(pMemStatusEx->ullTotalPhys >> 20, pPhysicalAll+1);
                                                                                                                                                                        FCopyMemoryWEnd(pPhysicalAll+1, L" MB");

                                                                                                                                                                        ULONG iCpuAll = pPowerInformation[0].MaxMhz/10;
                                                                                                                                                                        g_iCpuSpeedAll = iCpuAll;
                                                                                                                                                                        const ULONG iCpuShort = iCpuAll/100;
                                                                                                                                                                        FNumToStrW(iCpuShort, pCpuSpeedAll+1);
                                                                                                                                                                        wchar_t *const pDelim = pCpuSpeedAll + (iCpuShort >= 10 ? 3 : 2);
                                                                                                                                                                        *pDelim = L'.';
                                                                                                                                                                        iCpuAll -= iCpuShort*100;
                                                                                                                                                                        pDelim[1] = L'0' + iCpuAll/10;
                                                                                                                                                                        pDelim[2] = L'0' + iCpuAll%10;
                                                                                                                                                                        FCopyMemoryW(pDelim+3, L" GHz");
                                                                                                                                                                        SetWindowTextW(g_hWndCpuSpeed, pCpuSpeedAll+1);

                                                                                                                                                                        if (pTgSave->iOpacity < 255)
                                                                                                                                                                        {
                                                                                                                                                                            SetWindowLongPtrW(hWnd, GWL_EXSTYLE, WS_EX_WINDOWEDGE | WS_EX_LAYERED);
                                                                                                                                                                            SetLayeredWindowAttributes(hWnd, 0, pTgSave->iOpacity, LWA_ALPHA);
                                                                                                                                                                        }

                                                                                                                                                                        if (wWinVer >= _WIN32_WINNT_VISTA)        //bad work in XP
                                                                                                                                                                            hWinEventHook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, nullptr, WinEventProc, 0, 0, 0);
                                                                                                                                                                        g_hWndApp = hWnd;
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
        }
        else if (CreateProcessW(nullptr, pPath+MAX_PATH, nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT, nullptr, nullptr, pSi, pPi))
        {
            CloseHandle(pPi->hThread);
            CloseHandle(pPi->hProcess);
        }
        return -1;
    }
    case WM_COMMAND:
    {
        ___assert___(HIWORD(wParam) == 0);
        switch (wParam)
        {
        case eUptime:
            if (NT_SUCCESS(NtQuerySystemInformation(SystemTimeOfDayInformation, pSysTimeInfo, sizeof(SYSTEM_TIMEOFDAY_INFORMATION), nullptr)))
            {
                pSysTimeInfo->CurrentTime.QuadPart -= pSysTimeInfo->BootTime.QuadPart;
                ___assert___(pSysTimeInfo->CurrentTime.QuadPart > 0);
                wchar_t *pDelim = pUptime+8;        //"Uptime: ";
                ULONG iTempTotal = pSysTimeInfo->CurrentTime.QuadPart/g_iTicksPerDay;
                if (iTempTotal)        //days
                {
                    FNumToStrW(iTempTotal, pDelim);
                    pDelim = FStrChrWNull(pDelim);
                    FCopyMemoryW(pDelim, L"d ");
                    pDelim += 2;
                    pSysTimeInfo->CurrentTime.QuadPart -= iTempTotal*g_iTicksPerDay;
                }
                iTempTotal = pSysTimeInfo->CurrentTime.QuadPart/g_iTicksPerHour;
                if (iTempTotal)        //hours
                {
                    FNumToStrW(iTempTotal, pDelim);
                    pDelim = FStrChrWNull(pDelim);
                    FCopyMemoryW(pDelim, L"h ");
                    pDelim += 2;
                    pSysTimeInfo->CurrentTime.QuadPart -= iTempTotal*g_iTicksPerHour;
                }
                pSysTimeInfo->CurrentTime.QuadPart /= g_iTicksPerMin;
                FNumToStrW(pSysTimeInfo->CurrentTime.QuadPart, pDelim);
                FCopyMemoryWEnd(pDelim, L"m");        //minutes
                SetWindowTextW(hWndProcUptime, pUptime);
                iProcessesOld = 0;        //force update label
            }
            break;
        case eSavePos:
        {
            RECT rect;
            if (GetWindowRect(hWnd, &rect))
            {
                pTgSave->iDx = rect.left;
                pTgSave->iDy = rect.top;
                const HANDLE hFile = CreateFileW(pPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    DWORD dwBytes;
                    WriteFile(hFile, pTgSave, sizeof(SSaveSettings), &dwBytes, nullptr);
                    CloseHandle(hFile);
                }
            }
            break;
        }
        case eRunApp:
            if (CreateProcessW(nullptr, pTgSave->wAppPath, nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT, nullptr, nullptr, pSi, pPi))
            {
                CloseHandle(pPi->hThread);
                CloseHandle(pPi->hProcess);
            }
            break;
        case eStg:
            if (CreateProcessW(nullptr, pPath+MAX_PATH, nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT, nullptr, nullptr, pSi, pPi))
            {
                CloseHandle(pPi->hThread);
                CloseHandle(pPi->hProcess);
            }
            break;
        case eClose:
            PostMessageW(hWnd, WM_CLOSE, 0, 0);
            break;
        }
        return 0;
    }
    case WM_TIMER:
    {
        ULONG iTemp;
        ULONGLONG iTempTotal;
        //Memory
        if (GlobalMemoryStatusEx(pMemStatusEx))
        {
            static ULONG iPhysicalOld, iVirtualOld;
            //Physical
            iTemp = (pMemStatusEx->ullTotalPhys - pMemStatusEx->ullAvailPhys) >> 20;
            if (iTemp != iPhysicalOld)
            {
                iPhysicalOld = iTemp;
                FNumToStrW(iPhysicalOld, pPhysical);
                FCopyMemoryWEnd(pPhysical, pPhysicalAll);
                SetWindowTextW(hWndPhysical, pPhysical);
            }
            //Virtual
            iTempTotal = pMemStatusEx->ullTotalPageFile - pMemStatusEx->ullAvailPageFile;
            iTemp = iTempTotal >> 20;
            if (iTemp != iVirtualOld)
            {
                iVirtualOld = iTemp;
                FNumToStrW(iVirtualOld, pVirtual);
                static ULONGLONG iVirtualAllOld;
                if (pMemStatusEx->ullTotalPageFile != iVirtualAllOld)
                {
                    iVirtualAllOld = pMemStatusEx->ullTotalPageFile;
                    FNumToStrW(iVirtualAllOld >> 20, pVirtualAll+1);
                    FCopyMemoryWEnd(pVirtualAll+1, L" MB");
                }
                FCopyMemoryWEnd(pVirtual, pVirtualAll);
                SetWindowTextW(hWndVirtual, pVirtual);
            }
            //Physical Load
            if (pMemStatusEx->dwMemoryLoad != g_iPhysicalPercentOld)
            {
                g_iPhysicalPercentOld = pMemStatusEx->dwMemoryLoad;
                InvalidateRect(g_hWndPrbPhysical, nullptr, FALSE);
            }
            //Virtual Load
            iTemp = iTempTotal*100/pMemStatusEx->ullTotalPageFile;
            if (iTemp != g_iVirtualPercentOld)
            {
                g_iVirtualPercentOld = iTemp;
                InvalidateRect(g_hWndPrbVirtual, nullptr, FALSE);
            }
        }

        //Number of processes
        iTemp = 0;
        if (NT_SUCCESS(NtQuerySystemInformation(SystemProcessInformation, pBufferProcInfo, g_dwBufferSizeProcInfo, nullptr)))
        {
            const SYSTEM_PROCESS_INFORMATION *pIt = pBufferProcInfo;
            do {++iTemp;}
            while (pIt->NextEntryOffset ?
                   (pIt = pointer_cast<const SYSTEM_PROCESS_INFORMATION*>(pointer_cast<const BYTE*>(pIt) + pIt->NextEntryOffset), true) :
                   false);
        }

        if (iTemp != iProcessesOld)
        {
            iProcessesOld = iTemp;
            FNumToStrW(iProcessesOld, pProcesses+11);
            SetWindowTextW(hWndProcUptime, pProcesses);
        }

        ULONGLONG iTempCount;

        //CPU Load
        static ULONGLONG iLoadOld, iTotalOld;
        if (GetSystemTimes(pointer_cast<FILETIME*>(pDwIdleTime), pointer_cast<FILETIME*>(pDwKernelTime), pointer_cast<FILETIME*>(pDwUserTime)))
        {
            iTempTotal = *pDwKernelTime + *pDwUserTime;
            iTempCount = iTempTotal - *pDwIdleTime;
            iTemp = (iTempCount - iLoadOld)*100/(iTempTotal <= iTotalOld ? 1 : (iTempTotal - iTotalOld));
            iLoadOld = iTempCount;
            iTotalOld = iTempTotal;
            if (iTemp != g_iCpuLoadOld)
            {
                g_iCpuLoadOld = iTemp;
                InvalidateRect(g_hWndPrbCpu, nullptr, FALSE);
            }
        }

        //CPU Speed
        if (wWinVer >= _WIN32_WINNT_VISTA && NT_SUCCESS(NtPowerInformation(ProcessorInformation, nullptr, 0, pPowerInformation, dwProcessorsSize)))
        {
            iTempCount = 0;
            //this method taken from Process Hacker (processhacker.sourceforge.net: sysinfo.c)
            if (wWinVer >= _WIN32_WINNT_WIN7)
            {
                static bool bToggle = false;
                SYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION *pCurrentPerformanceDistribution, *pPreviousPerformanceDistribution;
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

                if (NT_SUCCESS(NtQuerySystemInformation(SystemProcessorPerformanceDistribution, pCurrentPerformanceDistribution, iBufferSize, nullptr)) &&
                        pCurrentPerformanceDistribution->ProcessorCount == dwNumberOfProcessors &&
                        pPreviousPerformanceDistribution->ProcessorCount == dwNumberOfProcessors)
                {
                    SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION *stateDistribution, *stateDifference;
                    DWORD ii = 0;
                    for (; ii < dwNumberOfProcessors; ++ii)
                    {
                        stateDistribution = pointer_cast<SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION*>(pointer_cast<BYTE*>(pCurrentPerformanceDistribution) + pCurrentPerformanceDistribution->Offsets[ii]);
                        stateDifference = pointer_cast<SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION*>(pDifferences + iStateSize*ii);
                        if (stateDistribution->StateCount == 2)
                        {
                            if (wWinVer >= _WIN32_WINNT_WINBLUE)
                            {
                                stateDifference->States[0] = stateDistribution->States[0];
                                stateDifference->States[1] = stateDistribution->States[1];
                            }
                            else
                            {
                                const SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8 *hitCountOld = pointer_cast<const SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8*>(stateDistribution->States);
                                stateDifference->States[0].Hits.QuadPart = hitCountOld->Hits;
                                stateDifference->States[0].PercentFrequency = hitCountOld->PercentFrequency;
                                hitCountOld = pointer_cast<const SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8*>(pointer_cast<BYTE*>(stateDistribution->States) + sizeof(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8));
                                stateDifference->States[1].Hits.QuadPart = hitCountOld->Hits;
                                stateDifference->States[1].PercentFrequency = hitCountOld->PercentFrequency;
                            }
                        }
                        else
                        {
                            ii = 0;
                            break;
                        }
                    }

                    if (ii)
                    {
                        for (ii = 0; ii < dwNumberOfProcessors; ++ii)
                        {
                            stateDistribution = pointer_cast<SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION*>(pointer_cast<BYTE*>(pPreviousPerformanceDistribution) + pPreviousPerformanceDistribution->Offsets[ii]);
                            stateDifference = pointer_cast<SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION*>(pDifferences + iStateSize*ii);
                            if (stateDistribution->StateCount == 2)
                            {
                                if (wWinVer >= _WIN32_WINNT_WINBLUE)
                                {
                                    stateDifference->States[0].Hits.QuadPart -= stateDistribution->States[0].Hits.QuadPart;
                                    stateDifference->States[1].Hits.QuadPart -= stateDistribution->States[1].Hits.QuadPart;
                                }
                                else
                                {
                                    stateDifference->States[0].Hits.QuadPart -= pointer_cast<const SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8*>(stateDistribution->States)->Hits;
                                    stateDifference->States[1].Hits.QuadPart -= pointer_cast<const SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8*>(pointer_cast<BYTE*>(stateDistribution->States) + sizeof(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8))->Hits;
                                }
                            }
                            else
                            {
                                ___assert___(false);
                                ii = 0;
                                break;
                            }
                        }

                        if (ii)
                        {
                            iTempTotal = 0;
                            for (ii = 0; ii < dwNumberOfProcessors; ++ii)
                            {
                                stateDifference = pointer_cast<SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION*>(pDifferences + iStateSize*ii);
                                iTempCount += stateDifference->States[0].Hits.QuadPart;
                                iTempTotal += stateDifference->States[0].Hits.QuadPart*stateDifference->States[0].PercentFrequency;
                                iTempCount += stateDifference->States[1].Hits.QuadPart;
                                iTempTotal += stateDifference->States[1].Hits.QuadPart*stateDifference->States[1].PercentFrequency;
                            }
                        }
                    }
                }
            }

            ULONG iCpuAll = iTempCount ? pPowerInformation[0].MaxMhz*iTempTotal/(iTempCount*1000) : pPowerInformation[0].CurrentMhz/10;
            ___assert___(iCpuAll >= 10 && iCpuAll <= 9900);
            if (iCpuAll != g_iCpuSpeedOld)
            {
                g_iCpuSpeedOld = iCpuAll;
                ULONG iCpuShort = g_iCpuSpeedOld/100;

                wchar_t *const pDelim = pCpuSpeed+1;
                if (iCpuShort < 10)
                    *pCpuSpeed = L'0' + iCpuShort;
                else
                {
                    pCpuSpeed[0] = L'0' + iCpuShort/10;
                    pCpuSpeed[1] = L'0' + iCpuShort%10;
                    ++pCpuSpeed;
                }
                *pDelim = L'.';
                iCpuAll -= iCpuShort*100;
                pDelim[1] = L'0' + iCpuAll/10;
                pDelim[2] = L'0' + iCpuAll%10;
                FCopyMemoryW(pDelim+3, pCpuSpeedAll);
                SetWindowTextW(g_hWndCpuSpeed, pCpuSpeed);
            }
        }
        return 0;
    }
    case WM_WINDOWPOSCHANGING:
    {
        RECT rectArea;
        if (SystemParametersInfoW(SPI_GETWORKAREA, 0, &rectArea, 0))
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
        SendMessageW(hWnd, WM_CLOSE, 0, 0);
        return 0;
    }
    case WM_DESTROY:
    {
        KillTimer(hWnd, 1);
        if (hWinEventHook)
            UnhookWinEvent(hWinEventHook);
        if (g_hbrBackground)
            DeleteObject(g_hbrBackground);
        if (g_hbrLoad)
            DeleteObject(g_hbrLoad);
        if (g_hFont)
            DeleteObject(g_hFont);
        if (hIconRunApp)
            DestroyIcon(hIconRunApp);
        if (hIconUptime)
            DestroyIcon(hIconUptime);
        if (hIconSavePos)
            DestroyIcon(hIconSavePos);
        if (hIconStg)
            DestroyIcon(hIconStg);
        if (hIconClose)
            DestroyIcon(hIconClose);
        if (pPowerInformation)
            if (const HANDLE hProcHeap = GetProcessHeap())
            {
                if (pDifferences)
                {
                    if (pBufferA)
                    {
                        if (pBufferProcInfo)
                            HeapFree(hProcHeap, HEAP_NO_SERIALIZE, pBufferProcInfo);
                        HeapFree(hProcHeap, HEAP_NO_SERIALIZE, pBufferA);
                    }
                    HeapFree(hProcHeap, HEAP_NO_SERIALIZE, pDifferences);
                }
                HeapFree(hProcHeap, HEAP_NO_SERIALIZE, pPowerInformation);
            }
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

    INITCOMMONCONTROLSEX initComCtrlEx;
    initComCtrlEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
    initComCtrlEx.dwICC = ICC_STANDARD_CLASSES;
    if (InitCommonControlsEx(&initComCtrlEx) == TRUE)
    {
        wchar_t wBuf[MAX_PATH+MAX_PATH+2+136];//[999000/999000 MB`999000/999000 MB`99.00/99.00 Ghz`Processes: 99000`Uptime: 9999d 23h 59m`/999000 MB`/999000 MB`/99.00 Ghz`100%`WorkerW`?]
        const DWORD dwLen = GetModuleFileNameW(nullptr, wBuf, MAX_PATH+1-4);        //".cfg"
        if (dwLen >= 4 && dwLen < MAX_PATH-4)        //".cfg"
        {
            wchar_t *pDelim = wBuf+dwLen;
            do
            {
                if (*--pDelim == L'\\')
                    break;
            } while (pDelim > wBuf);
            if (pDelim >= wBuf+2 && pDelim <= wBuf+MAX_PATH-27)        //"\SystemMonitorSettings.exe`"
            {
                WNDCLASSEX wndCl;
                wndCl.cbSize = sizeof(WNDCLASSEX);
                wndCl.style = 0;
                wndCl.lpfnWndProc = WindowProc;
                wndCl.cbClsExtra = 0;
                wndCl.cbWndExtra = 0;
                wndCl.hInstance = GetModuleHandleW(nullptr);
                wndCl.hIcon = nullptr;
                wndCl.hCursor = LoadCursorW(nullptr, IDC_ARROW);
                wndCl.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1);
                wndCl.lpszMenuName = nullptr;
                wndCl.lpszClassName = g_wGuidClass;
                wndCl.hIconSm = nullptr;
                if (RegisterClassExW(&wndCl))
                {
                    PAINTSTRUCT ps;
                    g_pPs = &ps;        //***
                    if (const HWND hWndParent = CreateWindowExW(0, L"#32770", nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr, nullptr))        //hide from taskbar
                    {
                        FCopyMemoryW(wBuf+dwLen, L".cfg");
                        wBuf[MAX_PATH] = L'"';
                        FCopyMemoryW(wBuf+MAX_PATH+1, wBuf);
                        FCopyMemoryW(wBuf+MAX_PATH+2+(pDelim-wBuf), g_wSettingsApp);

                        MEMORYSTATUSEX memStatusEx;
                        memStatusEx.dwLength = sizeof(MEMORYSTATUSEX);
                        ULONGLONG iFileTimes[3];
                        PROCESS_INFORMATION pi;
                        STARTUPINFO si;
                        FZeroMemory(&si, sizeof(STARTUPINFO));
                        si.cb = sizeof(STARTUPINFO);
                        SYSTEM_TIMEOFDAY_INFORMATION sysTimeInfo;
                        RECT rectGeometry;
                        SSaveSettings tgSave;
                        SCreateParams sCreateParams;
                        sCreateParams.pBufPath = wBuf;
                        sCreateParams.pMemStatusEx = &memStatusEx;
                        sCreateParams.pFts = iFileTimes;
                        sCreateParams.pPi = &pi;
                        sCreateParams.pSi = &si;
                        sCreateParams.pSysTimeInfo = &sysTimeInfo;
                        sCreateParams.pWndCl = &wndCl;
                        sCreateParams.pRectGeometry = &rectGeometry;
                        sCreateParams.sSave = &tgSave;        //***it's ok
                        if (CreateWindowExW(0, g_wGuidClass, L"SystemMonitor", 0, 0, 0, 0, 0, hWndParent, nullptr, wndCl.hInstance, &sCreateParams) &&
                                SetWindowPos(g_hWndApp, HWND_BOTTOM, rectGeometry.left, rectGeometry.top, rectGeometry.right, rectGeometry.bottom, SWP_SHOWWINDOW | SWP_NOACTIVATE))
                        {
                            PostMessageW(g_hWndApp, WM_TIMER, 0, 0);
                            MSG msg;
                            while (GetMessageW(&msg, nullptr, 0, 0) > 0)
                                DispatchMessageW(&msg);
                        }
                        SendMessageW(hWndParent, WM_CLOSE, 0, 0);
                    }
                    if (!wndCl.lpszClassName)
                        UnregisterClassW(g_wGuidClassProgress, wndCl.hInstance);
                    UnregisterClassW(g_wGuidClass, wndCl.hInstance);
                }
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------
extern "C" int start()
{
    ___assert___(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ && sizeof(FILETIME) == sizeof(ULONGLONG));
    FMain();
    ExitProcess(0);
    return 0;
}
