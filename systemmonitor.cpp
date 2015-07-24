#include "systemmonitor.h"

#define WINDOWS_XP 0x501        //Version 5.1
#define WINDOWS_7 0x601        //Version 6.1
#define WINDOWS_8_1 0x603        //Version 6.3

#define STATE_SIZE 40

#define TICKS_PER_MUS 10LL        //1 tick = 100NS
#define TICKS_PER_MS (TICKS_PER_MUS*1000LL)
#define TICKS_PER_SEC (TICKS_PER_MS*1000LL)
#define TICKS_PER_MIN (TICKS_PER_SEC*60LL)
#define TICKS_PER_HOUR (TICKS_PER_MIN*60LL)
#define TICKS_PER_DAY (TICKS_PER_HOUR*24LL)

//-------------------------------------------------------------------------------------------------
LabelEx::LabelEx(const QString &strAppName, QWidget *parent) : QLabel(strAppName, parent),
    wgtApp(parent)
{
    this->setAlignment(Qt::AlignHCenter);
    this->setAutoFillBackground(true);
    QFont font = this->font();
    font.setBold(true);
    this->setFont(font);
    QPalette pal = this->palette();
    pal.setColor(QPalette::Window, QColor(255, 127, 0));        //orange
    this->setPalette(pal);
}

//-------------------------------------------------------------------------------------------------
void LabelEx::mousePressEvent(QMouseEvent *event)
{
    point = this->mapTo(wgtApp, event->pos());
}

//-------------------------------------------------------------------------------------------------
void LabelEx::mouseMoveEvent(QMouseEvent *event)
{
    wgtApp->move(event->globalPos() - point);
}

//-------------------------------------------------------------------------------------------------
SystemMonitor::SystemMonitor() : QDialog(0, Qt::ToolTip),
    NtPowerInformation(0),
    ii(0),
    pPreviousPerformanceDistribution(0),
    pCurrentPerformanceDistribution(0),
    pBufDistrib(0),
    iLength(0x100),        //1 byte
    wWinVer(::GetVersion()),        //legal overflow (need manifest to detect Win 8.1)
    pFtIdleTime(static_cast<DWORDLONG*>(static_cast<void*>(&ftIdleTime))),
    pFtKernelTime(static_cast<DWORDLONG*>(static_cast<void*>(&ftKernelTime))),
    pFtUserTime(static_cast<DWORDLONG*>(static_cast<void*>(&ftUserTime))),
    iLastLoad(0),
    iLastTotal(0),
    strApp("taskmgr.exe"),
    hWinEventHook(0),
    bValid(false)
{
    Q_ASSERT_X(sizeof(DWORDLONG) == sizeof(FILETIME), "Error", "Low and High part DateTime of FILETIME structure");
    Q_ASSERT(sizeof(WORD) == 2);
    Q_ASSERT(FIELD_OFFSET(SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION, States) + sizeof(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT)*2 == STATE_SIZE);

    const QString strAppName = qAppName(),
            strAppDir = qApp->applicationDirPath();
    QTranslator *translator = new QTranslator(this);
    if (translator->load(strAppName, strAppDir) || translator->load(strAppName + '_' + QLocale::system().name(), strAppDir))
        qApp->installTranslator(translator);

    wWinVer = wWinVer << 8 | wWinVer >> 8;
    Q_ASSERT_X(wWinVer >= 0x501 && wWinVer <= 0x603, "Warning", "OS not tested");

    //Cpu Speed
    if (const HMODULE hMod = ::GetModuleHandle(L"ntdll.dll"))
        if ((NtQuerySystemInformation = reinterpret_cast<PNtQuerySystemInformation>(::GetProcAddress(hMod, "NtQuerySystemInformation"))))
            NtPowerInformation = reinterpret_cast<PNtPowerInformation>(::GetProcAddress(hMod, "NtPowerInformation"));
    if (!NtPowerInformation)
    {
        QMessageBox::critical(0, 0, tr("Error:") + " NtQuerySystemInformation/NtPowerInformation");
        return;
    }

    SYSTEM_INFO sysInfo;
    ::GetNativeSystemInfo(&sysInfo);
    iNumberOfProcessors = sysInfo.dwNumberOfProcessors;

    NTSTATUS ntStatus;
    do
    {
        if (!(pBuffer = malloc(iLength)))
            return;
        ntStatus = NtQuerySystemInformation(SystemProcessorPerformanceDistribution, pBuffer, iLength, &iLength);
        free(pBuffer);
        ++ii;
    } while (ntStatus == STATUS_INFO_LENGTH_MISMATCH && ii < 8);        //[8 - attempts count]
    if (ntStatus == STATUS_INFO_LENGTH_MISMATCH)
    {
        QMessageBox::critical(0, 0, tr("Error:") + " StatusInfoLengthMismatch");
        return;
    }

    pPowerInformation = new PROCESSOR_POWER_INFORMATION[iNumberOfProcessors];
    if (!NT_SUCCESS(NtPowerInformation(ProcessorInformation, 0, 0, pPowerInformation, sizeof(PROCESSOR_POWER_INFORMATION)*iNumberOfProcessors)))
    {
        QMessageBox::critical(0, 0, tr("Error:") + " CallNtPowerInformation");
        return;
    }

    if (wWinVer >= WINDOWS_7 && !(pBufDistrib = malloc(STATE_SIZE*iNumberOfProcessors)))
        return;
    dBaseCpuSpeed = pPowerInformation[0].MaxMhz/1000.0;
    strBaseGhz = '/' + QString::number(dBaseCpuSpeed, 'f', 2) + ' ' + tr("GHz");

    //Memory
    memStatusEx.dwLength = sizeof(MEMORYSTATUSEX);

    //Process count
    processEntry32.dwSize = sizeof(PROCESSENTRY32);

    //Run app
    memset(&si, 0, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

    //
    lblHeader = new LabelEx(strAppName, this);

    //
    QTransform transform;
    transform.rotate(90);
    QPushButton *pbRunApp = new QPushButton(QPixmap::fromImage(style()->standardPixmap(QStyle::SP_TitleBarShadeButton).toImage().transformed(transform)), 0, this);
    pbRunApp->setMaximumSize(16, 16);
    pbRunApp->setFlat(true);
    pbRunApp->setToolTip(tr("Run app"));
    QPushButton *pbSaveGeometry = new QPushButton(style()->standardIcon(QStyle::SP_TitleBarUnshadeButton), 0, this);
    pbSaveGeometry->setMaximumSize(16, 16);
    pbSaveGeometry->setFlat(true);
    pbSaveGeometry->setToolTip(tr("Remember position"));
    QPushButton *pbSettings = new QPushButton(style()->standardIcon(QStyle::SP_TitleBarMaxButton), 0, this);
    pbSettings->setMaximumSize(16, 16);
    pbSettings->setFlat(true);
    pbSettings->setToolTip(tr("Settings"));
    QPushButton *pbQuit = new QPushButton(style()->standardIcon(QStyle::SP_TitleBarCloseButton), 0, this);
    pbQuit->setMaximumSize(16, 16);
    pbQuit->setFlat(true);
    pbQuit->setToolTip(tr("Quit"));
    QHBoxLayout *hblTop = new QHBoxLayout;
    hblTop->addWidget(pbRunApp);
    hblTop->addWidget(pbSaveGeometry, 0, Qt::AlignRight);
    hblTop->addWidget(pbSettings);
    hblTop->addWidget(pbQuit);

    //
    QLabel *lblMemPhysName = new QLabel(tr("Physical:") + ' ', this);
    lblMemPhys = new QLabel(this);
    QHBoxLayout *hblMemPhys = new QHBoxLayout;
    hblMemPhys->addWidget(lblMemPhysName);
    hblMemPhys->addWidget(lblMemPhys, 0, Qt::AlignRight);

    prbMemPhys = new QProgressBar(this);
    prbMemPhys->setAlignment(Qt::AlignHCenter);
    prbMemPhys->setFixedHeight(15);

    QLabel *lblMemVirtualName = new QLabel(tr("Virtual:") + ' ', this);
    lblMemVirtual = new QLabel(this);
    QHBoxLayout *hblMemVirtual = new QHBoxLayout;
    hblMemVirtual->addWidget(lblMemVirtualName);
    hblMemVirtual->addWidget(lblMemVirtual, 0, Qt::AlignRight);

    prbMemVirtual = new QProgressBar(this);
    prbMemVirtual->setAlignment(Qt::AlignHCenter);
    prbMemVirtual->setFixedHeight(15);

    QGroupBox *gbMemory = new QGroupBox(tr("Memory"), this);
    QVBoxLayout *vblMemory = new QVBoxLayout(gbMemory);
    vblMemory->setContentsMargins(6, 2, 6, 2);
    vblMemory->setSpacing(0);
    vblMemory->addLayout(hblMemPhys);
    vblMemory->addWidget(prbMemPhys);
    vblMemory->addSpacing(6);
    vblMemory->addLayout(hblMemVirtual);
    vblMemory->addWidget(prbMemVirtual);

    //
    prbCpuLoad = new QProgressBar(this);
    prbCpuLoad->setAlignment(Qt::AlignHCenter);
    prbCpuLoad->setFixedHeight(15);
    prbCpuLoad->setToolTip(tr("CPU Load"));

    QLabel *lblCpuSpeedName = new QLabel(tr("Speed:") + ' ', this);
    lblCpuSpeed = new QLabel(this);
    QHBoxLayout *hblCpuSpeed = new QHBoxLayout;
    hblCpuSpeed->addWidget(lblCpuSpeedName);
    hblCpuSpeed->addWidget(lblCpuSpeed, 0, Qt::AlignRight);

    QGroupBox *gbCpu = new QGroupBox(tr("CPU"), this);
    QVBoxLayout *vblCpu = new QVBoxLayout(gbCpu);
    vblCpu->setContentsMargins(6, 2, 6, 2);
    vblCpu->setSpacing(6);
    vblCpu->addWidget(prbCpuLoad);
    vblCpu->addLayout(hblCpuSpeed);

    //
    lblProcCount = new QLabel(this);
    QPushButton *pbInfo = new QPushButton(style()->standardIcon(QStyle::SP_TitleBarContextHelpButton), 0, this);
    pbInfo->setMaximumSize(16, 16);
    pbInfo->setFlat(true);
    QHBoxLayout *hblProcCount = new QHBoxLayout;
    hblProcCount->setContentsMargins(7, 0, 0, 0);
    hblProcCount->addWidget(lblProcCount);
    hblProcCount->addWidget(pbInfo);

    //
    QFrame *frame = new QFrame(this);
    frame->setFrameStyle(QFrame::StyledPanel);
    QVBoxLayout *vblFrame = new QVBoxLayout(frame);
    vblFrame->setContentsMargins(4, 4, 4, 4);
    vblFrame->setSpacing(0);
    vblFrame->addWidget(lblHeader);
    vblFrame->addLayout(hblTop);
    vblFrame->addStretch();
    vblFrame->addWidget(gbMemory);
    vblFrame->addStretch();
    vblFrame->addWidget(gbCpu);
    vblFrame->addStretch();
    vblFrame->addLayout(hblProcCount);
    QVBoxLayout *vblMain = new QVBoxLayout(this);
    vblMain->setContentsMargins(1, 1, 1, 1);
    vblMain->addWidget(frame);

    timer = new QTimer(this);
    timer->setInterval(5000);
    this->setSizeGripEnabled(true);
    this->setAttribute(Qt::WA_QuitOnClose);
    hWndApp = reinterpret_cast<HWND>(this->winId());

    //connects
    connect(pbRunApp, SIGNAL(clicked()), this, SLOT(slotRunApp()));
    connect(pbSaveGeometry, SIGNAL(clicked()), this, SLOT(slotSaveGeometry()));
    connect(pbSettings, SIGNAL(clicked()), this, SLOT(slotShowSettings()));
    connect(pbQuit, SIGNAL(clicked()), this, SLOT(close()));
    connect(pbInfo, SIGNAL(clicked()), this, SLOT(slotShowInfo()));
    connect(timer, SIGNAL(timeout()), this, SLOT(slotTimer()));

    //settings
    QSettings stg(strAppDir + '/' + strAppName + ".ini", QSettings::IniFormat);
    stg.setIniCodec("UTF-8");
    if (stg.childGroups().contains("Settings"))
    {
        stg.beginGroup("Settings");

        int iStg = stg.value("UpdateInterval").toInt();
        if (iStg >= 1000 && iStg <= 90000)
            timer->setInterval(iStg);

        iStg = stg.value("Opacity").toInt();
        if (iStg > 0 && iStg < 255)        //255 - fully opaque
        {
            ::SetWindowLongPtr(hWndApp, GWL_EXSTYLE, ::GetWindowLongPtr(hWndApp, GWL_EXSTYLE) | WS_EX_LAYERED);
            ::SetLayeredWindowAttributes(hWndApp, 0, iStg, LWA_ALPHA);
        }

        QString strStg = stg.value("RunApp").toString();
        if (!strStg.isEmpty())
            strApp = strStg;

        iStg = stg.value("HeightIndicators").toInt();
        if (iStg > 0 && iStg <= 100)
            slotChangeHeightIndicators(iStg);

        QFont font;
        strStg = stg.value("Font").toString();
        if (!strStg.isEmpty() && font.fromString(strStg))
            this->setFont(font);

        strStg = stg.value("HeaderText").toString();
        if (!strStg.isEmpty())
            lblHeader->setText(strStg);

        QPalette pal = lblHeader->palette();
        QColor color = stg.value("HeaderTextColor").toString();
        if (color.isValid())
            pal.setColor(QPalette::WindowText, color);
        color = stg.value("HeaderBackColor").toString();
        if (color.isValid())
            pal.setColor(QPalette::Window, color);
        lblHeader->setPalette(pal);

        strStg = stg.value("HeaderFont").toString();
        if (!strStg.isEmpty() && font.fromString(strStg))
            lblHeader->setFont(font);

        stg.endGroup();
    }

    hWinEventHook = ::SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, 0, WinEventProc, 0, 0, 0);
    this->restoreGeometry(QSettings("UserPrograms", strAppName).value("Geometry").toByteArray());
    timer->start();
    slotTimer();
    bValid = true;
}

//-------------------------------------------------------------------------------------------------
SystemMonitor::~SystemMonitor()
{
    if (hWinEventHook)
        ::UnhookWinEvent(hWinEventHook);
    delete[] pPowerInformation;
    free(pBufDistrib);
    free(pPreviousPerformanceDistribution);
    free(pCurrentPerformanceDistribution);
}

//-------------------------------------------------------------------------------------------------
HWND SystemMonitor::hWndApp;

//-------------------------------------------------------------------------------------------------
void CALLBACK SystemMonitor::WinEventProc(HWINEVENTHOOK, DWORD, HWND hWnd, LONG, LONG, DWORD, DWORD)
{
    wchar_t wClassName[9];
    if (::GetClassName(hWnd, wClassName, 9/*"WorkerW\0?"*/) == 7 && wcscmp(wClassName, L"WorkerW") == 0)        //class name desktop (using show desktop function)
        for (int i = 0; i < 10; ++i)        //repeat several times
        {
            ::SetWindowPos(hWndApp, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            ::SetWindowPos(hWndApp, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
        }
}

//-------------------------------------------------------------------------------------------------
void SystemMonitor::slotShowSettings()
{
    hWndApp = reinterpret_cast<HWND>(this->winId());        //update just in case
    BYTE iAlpha;
    DWORD pdwFlags;
    if (!(GetLayeredWindowAttributes(hWndApp, 0, &iAlpha, &pdwFlags) && (pdwFlags & LWA_ALPHA)))
    {
        ::SetWindowLongPtr(hWndApp, GWL_EXSTYLE, ::GetWindowLongPtr(hWndApp, GWL_EXSTYLE) | WS_EX_LAYERED);
        iAlpha = 255;
    }

    const QString strAppName = qAppName();
    QDialog dialSettings(0, Qt::WindowCloseButtonHint);
    dialSettings.setWindowTitle(strAppName + ": " + tr("Settings"));

    QSpinBox *sbInterval = new QSpinBox(&dialSettings);
    sbInterval->setRange(1000, 90000);
    sbInterval->setSuffix(' ' + tr("msec"));
    sbInterval->setValue(timer->interval());
    QLabel *lblInterval = new QLabel(tr("(5000 - by default)"), &dialSettings);
    QHBoxLayout *hblInterval = new QHBoxLayout;
    hblInterval->addWidget(sbInterval);
    hblInterval->addWidget(lblInterval);
    hblInterval->addStretch();

    QSpinBox *sbOpacity = new QSpinBox(&dialSettings);
    sbOpacity->setRange(1, 255);
    sbOpacity->setValue(iAlpha);
    QLabel *lblOpacity = new QLabel(tr("(255 - fully opaque)"), &dialSettings);
    QHBoxLayout *hblOpacity = new QHBoxLayout;
    hblOpacity->addWidget(sbOpacity);
    hblOpacity->addWidget(lblOpacity);
    hblOpacity->addStretch();

    QLineEdit *leRunApp = new QLineEdit(strApp, &dialSettings);
    leRunApp->setPlaceholderText("taskmgr.exe");

    QSpinBox *sbHeightIndicators = new QSpinBox(&dialSettings);
    sbHeightIndicators->setRange(1, 100);
    sbHeightIndicators->setSuffix(' ' + tr("pix"));
    sbHeightIndicators->setValue(prbCpuLoad->height());
    QLabel *lblHeightIndicators = new QLabel(tr("(15 - by default)"), &dialSettings);
    QHBoxLayout *hblHeightIndicators = new QHBoxLayout;
    hblHeightIndicators->addWidget(sbHeightIndicators);
    hblHeightIndicators->addWidget(lblHeightIndicators);
    hblHeightIndicators->addStretch();

    QPushButton *pbFont = new QPushButton(style()->standardIcon(QStyle::SP_TitleBarMaxButton), 0, &dialSettings);
    QHBoxLayout *hblFont = new QHBoxLayout;
    hblFont->addWidget(pbFont, 0, Qt::AlignLeft);

    QVBoxLayout *vblSpacing = new QVBoxLayout;
    vblSpacing->addSpacing(10);

    QLineEdit *leHeaderText = new QLineEdit(lblHeader->text(), &dialSettings);
    leHeaderText->setPlaceholderText(strAppName);

    QPalette pal = lblHeader->palette();
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(pal.color(QPalette::WindowText));
    painter.drawEllipse(1, 1, 14, 14);
    QPushButton *pbHeaderTextColor = new QPushButton(pixmap, 0, &dialSettings);
    QHBoxLayout *hblHeaderTextColor = new QHBoxLayout;
    hblHeaderTextColor->addWidget(pbHeaderTextColor, 0, Qt::AlignLeft);

    pixmap.fill(Qt::transparent);
    painter.setBrush(pal.color(QPalette::Window));
    painter.drawEllipse(1, 1, 14, 14);
    QPushButton *pbHeaderBackColor = new QPushButton(pixmap, 0, &dialSettings);
    QHBoxLayout *hblHeaderBackColor = new QHBoxLayout;
    hblHeaderBackColor->addWidget(pbHeaderBackColor, 0, Qt::AlignLeft);

    QPushButton *pbHeaderFont = new QPushButton(style()->standardIcon(QStyle::SP_TitleBarMaxButton), 0, &dialSettings);
    QHBoxLayout *hblHeaderFont = new QHBoxLayout;
    hblHeaderFont->addWidget(pbHeaderFont, 0, Qt::AlignLeft);

    QFormLayout *frml = new QFormLayout;
    frml->addRow(tr("Update Interval:"), hblInterval);
    frml->addRow(tr("Opacity:"), hblOpacity);
    frml->addRow(tr("Run Application:"), leRunApp);
    frml->addRow(tr("Height Indicators:"), hblHeightIndicators);
    frml->addRow(tr("Font:"), hblFont);
    frml->addRow(vblSpacing);
    frml->addRow(tr("Header Text:"), leHeaderText);
    frml->addRow(tr("Header Text Color:"), hblHeaderTextColor);
    frml->addRow(tr("Header Back Color:"), hblHeaderBackColor);
    frml->addRow(tr("Header Font:"), hblHeaderFont);

    QDialogButtonBox *dialogBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialSettings);

    QVBoxLayout *vblSettings = new QVBoxLayout(&dialSettings);
    vblSettings->setContentsMargins(5, 5, 5, 5);
    vblSettings->setSpacing(2);
    vblSettings->addLayout(frml);
    vblSettings->addWidget(dialogBox);

    dialSettings.setFixedHeight(dialSettings.minimumSizeHint().height());

    //connects
    connect(sbOpacity, SIGNAL(valueChanged(int)), this, SLOT(slotChangeOpacity(int)));
    connect(sbHeightIndicators, SIGNAL(valueChanged(int)), this, SLOT(slotChangeHeightIndicators(int)));
    connect(pbFont, SIGNAL(clicked()), this, SLOT(slotChangeFont()));
    connect(pbHeaderTextColor, SIGNAL(clicked()), this, SLOT(slotChangeHeaderTextColor()));
    connect(pbHeaderBackColor, SIGNAL(clicked()), this, SLOT(slotChangeHeaderBackColor()));
    connect(pbHeaderFont, SIGNAL(clicked()), this, SLOT(slotChangeHeaderFont()));
    connect(dialogBox, SIGNAL(accepted()), &dialSettings, SLOT(accept()));
    connect(dialogBox, SIGNAL(rejected()), &dialSettings, SLOT(reject()));

    const int iHeight = prbCpuLoad->height();
    const QFont font = this->font(),
            fontHeader = lblHeader->font();

    if (dialSettings.exec() == QDialog::Accepted)
    {
        const int iInterval = sbInterval->value();
        if (timer->interval() != iInterval)
            timer->setInterval(iInterval);

        QString strStg = leRunApp->text();
        strApp = strStg.isEmpty() ? "taskmgr.exe" : strStg;

        strStg = leHeaderText->text();
        lblHeader->setText(strStg.isEmpty() ? strAppName : strStg);

        pal = lblHeader->palette();

        QSettings stg(qApp->applicationDirPath() + '/' + strAppName + ".ini", QSettings::IniFormat);
        stg.setIniCodec("UTF-8");
        stg.beginGroup("Settings");
        stg.setValue("UpdateInterval", iInterval);
        stg.setValue("Opacity", sbOpacity->value());
        stg.setValue("RunApp", strApp);
        stg.setValue("HeightIndicators", sbHeightIndicators->value());
        stg.setValue("Font", this->font().toString());
        stg.setValue("HeaderText", strStg.isEmpty() ? strAppName : strStg);
        stg.setValue("HeaderTextColor", QString("#%1").arg(pal.color(QPalette::WindowText).rgba(), 8, 16, QChar('0')));        //[8 - "aarrggbb"]
        stg.setValue("HeaderBackColor", QString("#%1").arg(pal.color(QPalette::Window).rgba(), 8, 16, QChar('0')));
        stg.setValue("HeaderFont", lblHeader->font().toString());
        stg.endGroup();
    }
    else
    {
        ::SetLayeredWindowAttributes(hWndApp, 0, iAlpha, LWA_ALPHA);
        slotChangeHeightIndicators(iHeight);
        this->setFont(font);
        lblHeader->setPalette(pal);
        lblHeader->setFont(fontHeader);
    }
}

//-------------------------------------------------------------------------------------------------
void SystemMonitor::slotShowInfo()
{
    if (NT_SUCCESS(NtQuerySystemInformation(SystemTimeOfDayInformation, &sysTimeInfo, sizeof(SYSTEM_TIMEOFDAY_INFORMATION), 0)))
    {
        iTotal = sysTimeInfo.CurrentTime.QuadPart - sysTimeInfo.BootTime.QuadPart;
        Q_ASSERT(iTotal > 0);
        QString strInfo = tr("Uptime:") + '\n';
        ii = iTotal/TICKS_PER_DAY;
        if (ii)
            strInfo += QString::number(ii) + tr("d") + ' ';
        iTotal = iTotal-ii*TICKS_PER_DAY;
        ii = iTotal/TICKS_PER_HOUR;
        if (ii)
            strInfo += QString::number(ii) + tr("h") + ' ';
        strInfo += QString::number((iTotal-ii*TICKS_PER_HOUR)/TICKS_PER_MIN) + tr("m");
        QToolTip::showText(lblHeader->mapToGlobal(QPoint(0, 0)), strInfo);
    }
}

//-------------------------------------------------------------------------------------------------
bool SystemMonitor::fGetCpuSpeedFromDistribution()
{
    //taken from Process Hacker (processhacker.sourceforge.net)
    if (pCurrentPerformanceDistribution->ProcessorCount != iNumberOfProcessors || pPreviousPerformanceDistribution->ProcessorCount != iNumberOfProcessors)
        return false;
    for (ii = 0; ii < iNumberOfProcessors; ++ii)
    {
        pStateDistribution = static_cast<PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION>(static_cast<PVOID>(static_cast<PCHAR>(static_cast<PVOID>(pCurrentPerformanceDistribution)) + pCurrentPerformanceDistribution->Offsets[ii]));
        pStateDifference = static_cast<PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION>(static_cast<PVOID>(static_cast<PCHAR>(pBufDistrib) + STATE_SIZE*ii));
        if (pStateDistribution->StateCount != 2)
            return false;
        if (wWinVer >= WINDOWS_8_1)
        {
            pStateDifference->States[0] = pStateDistribution->States[0];
            pStateDifference->States[1] = pStateDistribution->States[1];
        }
        else
        {
            pHitcountOld = static_cast<PSYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8>(static_cast<PVOID>(pStateDistribution->States));
            pStateDifference->States[0].Hits.QuadPart = pHitcountOld->Hits;
            pStateDifference->States[0].PercentFrequency = pHitcountOld->PercentFrequency;
            pHitcountOld = static_cast<PSYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8>(static_cast<PVOID>(static_cast<PCHAR>(static_cast<PVOID>(pStateDistribution->States)) + sizeof(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8)));
            pStateDifference->States[1].Hits.QuadPart = pHitcountOld->Hits;
            pStateDifference->States[1].PercentFrequency = pHitcountOld->PercentFrequency;
        }
    }
    for (ii = 0; ii < iNumberOfProcessors; ++ii)
    {
        pStateDistribution = static_cast<PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION>(static_cast<PVOID>(static_cast<PCHAR>(static_cast<PVOID>(pPreviousPerformanceDistribution)) + pPreviousPerformanceDistribution->Offsets[ii]));
        pStateDifference = static_cast<PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION>(static_cast<PVOID>(static_cast<PCHAR>(pBufDistrib) + STATE_SIZE*ii));
        if (pStateDistribution->StateCount != 2)
            return false;
        if (wWinVer >= WINDOWS_8_1)
        {
            pStateDifference->States[0].Hits.QuadPart -= pStateDistribution->States[0].Hits.QuadPart;
            pStateDifference->States[1].Hits.QuadPart -= pStateDistribution->States[1].Hits.QuadPart;
        }
        else
        {
            pHitcountOld = static_cast<PSYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8>(static_cast<PVOID>(pStateDistribution->States));
            pStateDifference->States[0].Hits.QuadPart -= pHitcountOld->Hits;
            pHitcountOld = static_cast<PSYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8>(static_cast<PVOID>(static_cast<PCHAR>(static_cast<PVOID>(pStateDistribution->States)) + sizeof(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8)));
            pStateDifference->States[1].Hits.QuadPart -= pHitcountOld->Hits;
        }
    }
    iTotal = iCount = 0;
    for (ii = 0; ii < iNumberOfProcessors; ++ii)
    {
        pStateDifference = static_cast<PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION>(static_cast<PVOID>(static_cast<PCHAR>(pBufDistrib) + STATE_SIZE*ii));
        iCount += pStateDifference->States[0].Hits.QuadPart;
        iTotal += pStateDifference->States[0].Hits.QuadPart*pStateDifference->States[0].PercentFrequency;
        iCount += pStateDifference->States[1].Hits.QuadPart;
        iTotal += pStateDifference->States[1].Hits.QuadPart*pStateDifference->States[1].PercentFrequency;
    }
    if (!iCount)
        return false;
    dCpuSpeed = iTotal/(iCount*100.0);
    return true;
}

//-------------------------------------------------------------------------------------------------
void SystemMonitor::slotTimer()
{
    //Cpu Speed
    if (!NT_SUCCESS(NtPowerInformation(ProcessorInformation, 0, 0, pPowerInformation, sizeof(PROCESSOR_POWER_INFORMATION)*iNumberOfProcessors)))
        memset(pPowerInformation, 0, sizeof(PROCESSOR_POWER_INFORMATION)*iNumberOfProcessors);
    if (wWinVer >= WINDOWS_7)
    {
        if (pPreviousPerformanceDistribution)
            free(pPreviousPerformanceDistribution);
        pPreviousPerformanceDistribution = pCurrentPerformanceDistribution;
        pCurrentPerformanceDistribution = 0;
        if (!(pBuffer = malloc(iLength)))
        {
            this->close();
            return;
        }
        if (NT_SUCCESS(NtQuerySystemInformation(SystemProcessorPerformanceDistribution, pBuffer, iLength, &iLength)))
            pCurrentPerformanceDistribution = static_cast<PSYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION>(pBuffer);
        else
            free(pBuffer);
        dCpuSpeed = (pCurrentPerformanceDistribution && pPreviousPerformanceDistribution && fGetCpuSpeedFromDistribution()) ?
                    pPowerInformation[0].MaxMhz*dCpuSpeed/1000.0 : pPowerInformation[0].CurrentMhz/1000.0;
    }
    else
        dCpuSpeed = pPowerInformation[0].CurrentMhz/1000.0;
    lblCpuSpeed->setText(((dCpuSpeed - dBaseCpuSpeed > 0.005) ? ("<font color=red>" + QString::number(dCpuSpeed, 'f', 2) + "</font>") :
                                                                (QString::number(dCpuSpeed, 'f', 2))) + strBaseGhz);

    //Load Cpu
    if (::GetSystemTimes(&ftIdleTime, &ftKernelTime, &ftUserTime))
    {
        iCurTotal = *pFtKernelTime + *pFtUserTime;
        iCurLoad = iCurTotal - *pFtIdleTime;
        prbCpuLoad->setValue((iCurLoad - iLastLoad)*100/(iCurTotal - iLastTotal));
        iLastLoad = iCurLoad;
        iLastTotal = iCurTotal;
    }
    else
        prbCpuLoad->setValue(0);

    //Memory
    if (::GlobalMemoryStatusEx(&memStatusEx))
    {
        lblMemPhys->setText(QString::number((memStatusEx.ullTotalPhys - memStatusEx.ullAvailPhys) >> 20) + '/' +
                            QString::number(memStatusEx.ullTotalPhys >> 20) + " MB");
        prbMemPhys->setValue(memStatusEx.dwMemoryLoad);
        iCurLoad = memStatusEx.ullTotalPageFile - memStatusEx.ullAvailPageFile;
        lblMemVirtual->setText(QString::number(iCurLoad >> 20) + '/' +
                               QString::number(memStatusEx.ullTotalPageFile >> 20) + " MB");
        prbMemVirtual->setValue(iCurLoad*100/memStatusEx.ullTotalPageFile);
    }
    else
    {
        lblMemPhys->setText("?");
        prbMemPhys->setValue(0);
        lblMemVirtual->setText("?");
        prbMemVirtual->setValue(0);
    }

    //Process count
    ii = 0;
    if ((hndProc = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)) != INVALID_HANDLE_VALUE)
    {
        if (::Process32First(hndProc, &processEntry32))
            do {++ii;} while (::Process32Next(hndProc, &processEntry32));
        ::CloseHandle(hndProc);
    }
    lblProcCount->setText(tr("Processes:") + ' ' + QString::number(ii));
}

//-------------------------------------------------------------------------------------------------
void SystemMonitor::slotChangeHeightIndicators(const int iHeight) const
{
    prbMemPhys->setFixedHeight(iHeight);
    prbMemVirtual->setFixedHeight(iHeight);
    prbCpuLoad->setFixedHeight(iHeight);
}

//-------------------------------------------------------------------------------------------------
void SystemMonitor::slotChangeFont()
{
    bool bOk;
    const QFont font = QFontDialog::getFont(&bOk, this->font());
    if (bOk)
    {
        //const QFont fontHeader = lblHeader->font();
        //this->setFont(font);
        //lblHeader->setFont(fontHeader);
        //fix bug (feature?)
        QFont fontHeader;
        fontHeader.fromString(lblHeader->font().toString());
        this->setFont(font);
        lblHeader->setFont(fontHeader);
    }
}

//-------------------------------------------------------------------------------------------------
void SystemMonitor::slotChangeHeaderTextColor() const
{
    Q_ASSERT(qobject_cast<QPushButton*>(this->sender()));
    QPalette pal = lblHeader->palette();
    const QColor color = QColorDialog::getColor(pal.color(QPalette::WindowText), 0, 0, QColorDialog::ShowAlphaChannel);
    if (color.isValid())
    {
        pal.setColor(QPalette::WindowText, color);
        lblHeader->setPalette(pal);
        if (QPushButton *pbHeaderTextColor = static_cast<QPushButton*>(this->sender()))
        {
            QPixmap pixmap(16, 16);
            pixmap.fill(Qt::transparent);
            QPainter painter(&pixmap);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setBrush(color);
            painter.drawEllipse(1, 1, 14, 14);
            pbHeaderTextColor->setIcon(pixmap);
        }
    }
}

//-------------------------------------------------------------------------------------------------
void SystemMonitor::slotChangeHeaderBackColor() const
{
    Q_ASSERT(qobject_cast<QPushButton*>(this->sender()));
    QPalette pal = lblHeader->palette();
    const QColor color = QColorDialog::getColor(pal.color(QPalette::Window), 0, 0, QColorDialog::ShowAlphaChannel);
    if (color.isValid())
    {
        pal.setColor(QPalette::Window, color);
        lblHeader->setPalette(pal);
        if (QPushButton *pbHeaderBackColor = static_cast<QPushButton*>(this->sender()))
        {
            QPixmap pixmap(16, 16);
            pixmap.fill(Qt::transparent);
            QPainter painter(&pixmap);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setBrush(color);
            painter.drawEllipse(1, 1, 14, 14);
            pbHeaderBackColor->setIcon(pixmap);
        }
    }
}

//-------------------------------------------------------------------------------------------------
void SystemMonitor::slotChangeHeaderFont() const
{
    bool bOk;
    const QFont fontHeader = QFontDialog::getFont(&bOk, lblHeader->font());
    if (bOk)
        lblHeader->setFont(fontHeader);
}
