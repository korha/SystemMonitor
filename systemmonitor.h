#ifndef SYSTEMMONITOR_H
#define SYSTEMMONITOR_H

#include <QtWidgets/QApplication>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFontDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStyle>
#include <QtWidgets/QToolTip>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QPainter>
#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtCore/qt_windows.h>
#include <ntdef.h>
#include <ntstatus.h>
#include <powrprof.h>
#include <tlhelp32.h>

class LabelEx : public QLabel
{
    //Q_OBJECT not required
public:
    explicit LabelEx(QWidget *parent);

private:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    QWidget *const wgtApp;
    QPoint point;
};

class SystemMonitor : public QDialog
{
    Q_OBJECT
public:
    explicit SystemMonitor(bool &bValid);
    ~SystemMonitor();

private:
    typedef struct _PROCESSOR_POWER_INFORMATION
    {
        ULONG Number,
        MaxMhz,
        CurrentMhz,
        MhzLimit,
        MaxIdleState,
        CurrentIdleState;
    } PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

    typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION
    {
        ULONG ProcessorCount,
        Offsets[1];
    } SYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION, *PSYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION;

    typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT
    {
        LARGE_INTEGER Hits;
        UCHAR PercentFrequency;
    } SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT, *PSYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT;

    typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION
    {
        ULONG ProcessorNumber,
        StateCount;
        SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT States[1];
    } SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION, *PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION;

    typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8
    {
        ULONG Hits;
        UCHAR PercentFrequency;
    } SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8, *PSYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8;

    typedef struct _SYSTEM_TIMEOFDAY_INFORMATION
    {
        LARGE_INTEGER BootTime,
        CurrentTime,
        TimeZoneBias;
        ULONG CurrentTimeZoneId;
        BYTE Reserved1[20];
    } SYSTEM_TIMEOFDAY_INFORMATION,*PSYSTEM_TIMEOFDAY_INFORMATION;

    typedef enum _SYSTEM_INFORMATION_CLASS
    {
        SystemTimeOfDayInformation = 3,
        SystemProcessorPerformanceDistribution = 100
    } SYSTEM_INFORMATION_CLASS;

    typedef NTSTATUS (WINAPI *PNtQuerySystemInformation)(SYSTEM_INFORMATION_CLASS SystemInformationClass,
                                                         PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);
    static HWND hWndApp;
    static void CALLBACK WinEventProc(HWINEVENTHOOK, DWORD, HWND hWnd, LONG, LONG, DWORD, DWORD);
    PNtQuerySystemInformation NtQuerySystemInformation;
    DWORD ii;
    DWORDLONG iCurLoad;

    //Cpu Speed
    PPROCESSOR_POWER_INFORMATION pPowerInformation;
    PSYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION pPreviousPerformanceDistribution,
    pCurrentPerformanceDistribution;
    PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION pStateDistribution,
    pStateDifference;
    PSYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8 pHitcountOld;

    PVOID pBuffer,
    pBufDistrib;
    DWORD iNumberOfProcessors,
    iLength;
    long long iCount,
    iTotal;
    double dBaseCpuSpeed,
    dCpuSpeed;
    QString strBaseGhz;
    WORD wWinVer;

    //Load Cpu
    FILETIME ftIdleTime,
    ftKernelTime,
    ftUserTime;
    const DWORDLONG *const pFtIdleTime,
    *const pFtKernelTime,
    *const pFtUserTime;
    DWORDLONG iCurTotal,
    iLastLoad,
    iLastTotal;

    //Memory
    MEMORYSTATUSEX memStatusEx;

    //Process count
    HANDLE hndProc;
    PROCESSENTRY32 processEntry32;

    LabelEx *lblHeader;
    QLabel *lblMemPhys;
    QProgressBar *prbMemPhys;
    QLabel *lblMemVirtual;
    QProgressBar *prbMemVirtual,
    *prbCpuLoad;
    QLabel *lblCpuSpeed,
    *lblProcCount;
    QTimer *timer;
    QString strApp;
    const HWINEVENTHOOK hWinEventHook;
    bool fGetCpuSpeedFromDistribution();

private slots:
    inline void slotRunApp() const
    {
        QProcess::startDetached(strApp);
    }
    inline void slotSaveGeometry() const
    {
        QSettings("UserPrograms", qAppName()).setValue("Geometry", this->saveGeometry());
    }
    void slotShowSettings();
    void slotShowInfo();
    void slotTimer();
    inline void slotChangeOpacity(const int iAlpha) const
    {
        ::SetLayeredWindowAttributes(hWndApp, 0, iAlpha, LWA_ALPHA);
    }
    void slotChangeHeightIndicators(const int iHeight) const;
    void slotChangeFont();
    void slotChangeHeaderTextColor() const;
    void slotChangeHeaderBackColor() const;
    void slotChangeHeaderFont() const;
};

#endif // SYSTEMMONITOR_H
