// copyright marazmista @ 29.03.2014

#include "dxorg.h"
#include "globalStuff.h"

#include <QFile>
#include <QTextStream>
#include <QTime>
#include <QCoreApplication>

extern "C" {
#include <X11/extensions/Xrandr.h>
}

#define EDID_OFFSET_PNP_ID    0x08
#define EDID_OFFSET_MODEL_NUMBER     0x0a
#define EDID_OFFSET_SERIAL_NUMBER    0x0c
#define EDID_OFFSET_DATA_BLOCKS  0x36
#define EDID_OFFSET_LAST_BLOCK   0x6c

#define EDID_DESCRIPTOR_PRODUCT_NAME 0xfc

#define ATOM_VALUE (Atom)4
#define INTEGER_VALUE (Atom)19
#define CARDINAL_VALUE (Atom)6

// define static members //
dXorg::tempSensor dXorg::currentTempSensor = dXorg::TS_UNKNOWN;
globalStuff::powerMethod dXorg::currentPowerMethod;
int dXorg::sensorsGPUtempIndex;
QChar dXorg::gpuSysIndex;
QSharedMemory dXorg::sharedMem;
dXorg::driverFilePaths dXorg::filePaths;
// end //

daemonComm *dcomm = new daemonComm();

void dXorg::configure(QString gpuName) {
    figureOutGpuDataFilePaths(gpuName);
    currentTempSensor = testSensor();
    currentPowerMethod = getPowerMethod();

    if (!globalStuff::globalConfig.rootMode) {
        qDebug() << "Running in non-root mode, connecting and configuring the daemon";
        // create the shared mem block. The if comes from that configure method
        // is called on every change gpu, so later, shared mem already exists
        if (!sharedMem.isAttached()) {
            qDebug() << "Shared memory is not attached, creating it";
            sharedMem.setKey("radeon-profile");
           if (!sharedMem.create(SHARED_MEM_SIZE)){
                if(sharedMem.error() == QSharedMemory::AlreadyExists){
                    qDebug() << "Shared memory already exists, attaching to it";
                    if (!sharedMem.attach())
                        qCritical() << "Unable to attach to the shared memory: " << sharedMem.errorString();
                } else
                    qCritical() << "Unable to create the shared memory: " << sharedMem.errorString();
           }
           // If QSharedMemory::create() returns true, it has already automatically attached
        }

        dcomm->connectToDaemon();
        if (daemonConnected()) {
            qDebug() << "Daemon is connected, configuring it";
            //  Configure the daemon to read the data
            QString command; // SIGNAL_CONFIG + SEPARATOR + CLOCKS_PATH + SEPARATOR
            command.append(DAEMON_SIGNAL_CONFIG).append(SEPARATOR); // Configuration flag
            command.append(filePaths.clocksPath).append(SEPARATOR); // Path where the daemon will read clocks

            qDebug() << "Sending daemon config command: " << command;
            dcomm->sendCommand(command);

            reconfigureDaemon(); // Set up timer
        } else
            qCritical() << "Daemon is not connected, therefore it can't be configured";
    }
}

void dXorg::reconfigureDaemon() { // Set up the timer
    if (daemonConnected()) {
        qDebug() << "Configuring daemon timer";
        QString command;

        if (globalStuff::globalConfig.daemonAutoRefresh){ // SIGNAL_TIMER_ON + SEPARATOR + INTERVAL + SEPARATOR
            command.append(DAEMON_SIGNAL_TIMER_ON).append(SEPARATOR); // Enable timer
            command.append(QString::number(globalStuff::globalConfig.interval)).append(SEPARATOR); // Set timer interval
        }
        else // SIGNAL_TIMER_OFF + SEPARATOR
            command.append(DAEMON_SIGNAL_TIMER_OFF).append(SEPARATOR); // Disable timer

        qDebug() << "Sending daemon reconfig signal: " << command;
        dcomm->sendCommand(command);
    } else
        qWarning() << "Daemon is not connected, therefore it's timer can't be reconfigured";
}

bool dXorg::daemonConnected() {
    return dcomm->connected();
}

void dXorg::figureOutGpuDataFilePaths(QString gpuName) {
    gpuSysIndex = gpuName.at(gpuName.length()-1);
    QString devicePath = "/sys/class/drm/"+gpuName+"/device/";

    filePaths.powerMethodFilePath = devicePath + file_PowerMethod,
            filePaths.profilePath = devicePath + file_powerProfile,
            filePaths.dpmStateFilePath = devicePath + file_powerDpmState,
            filePaths.forcePowerLevelFilePath = devicePath + file_powerDpmForcePerformanceLevel,
            filePaths.moduleParamsPath = devicePath + "driver/module/holders/radeon/parameters/",
            filePaths.clocksPath = "/sys/kernel/debug/dri/"+QString(gpuSysIndex)+"/radeon_pm_info"; // this path contains only index
          //  filePaths.clocksPath = "/tmp/radeon_pm_info"; // testing


    QString hwmonDevicePath = globalStuff::grabSystemInfo("ls "+ devicePath+ "hwmon/")[0]; // look for hwmon devices in card dir
    hwmonDevicePath =  devicePath + "hwmon/" + ((hwmonDevicePath.isEmpty() ? "hwmon0" : hwmonDevicePath));

    filePaths.sysfsHwmonTempPath = hwmonDevicePath  + "/temp1_input";

    if (QFile::exists(hwmonDevicePath + "/pwm1_enable")) {
        filePaths.pwmEnablePath = hwmonDevicePath + "/pwm1_enable";

        if (QFile::exists(hwmonDevicePath + "/pwm1"))
            filePaths.pwmSpeedPath = hwmonDevicePath + "/pwm1";

        if (QFile::exists(hwmonDevicePath + "/pwm1_max"))
            filePaths.pwmMaxSpeedPath = hwmonDevicePath + "/pwm1_max";
    } else {
        filePaths.pwmEnablePath = "";
        filePaths.pwmSpeedPath = "";
        filePaths.pwmMaxSpeedPath = "";
    }
}

// method for gather info about clocks from deamon or from debugfs if root
QString dXorg::getClocksRawData(bool resolvingGpuFeatures) {
    QFile clocksFile(filePaths.clocksPath);
    QByteArray data;

    if (clocksFile.open(QIODevice::ReadOnly)) // check for debugfs access
            data = clocksFile.readAll();
    else if (daemonConnected()) {
        if (!globalStuff::globalConfig.daemonAutoRefresh){
            qDebug() << "Asking the daemon to read clocks";
            dcomm->sendCommand(QString(DAEMON_SIGNAL_READ_CLOCKS).append(SEPARATOR)); // SIGNAL_READ_CLOCKS + SEPARATOR
        }

        // fist call, so notihing is in sharedmem and we need to wait for data
        // because we need correctly figure out what is available
        // see: https://stackoverflow.com/questions/3752742/how-do-i-create-a-pause-wait-function-using-qt
        if (resolvingGpuFeatures) {
            QTime delayTime = QTime::currentTime().addMSecs(1000);
            while (QTime::currentTime() < delayTime);
                QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }

       if (sharedMem.lock()) {
            const char *to = (const char*)sharedMem.constData();
            if (to != NULL) {
                qDebug() << "Reading data from shared memory";
                data = QByteArray::fromRawData(to, SHARED_MEM_SIZE);
            } else
                qWarning() << "Shared memory data pointer is invalid: " << sharedMem.errorString();
            sharedMem.unlock();
        } else
            qWarning() << "Unable to lock the shared memory: " << sharedMem.errorString();
    }

    if (data.isEmpty())
        qWarning() << "No data was found";

    return (QString)data.trimmed();
}

globalStuff::gpuClocksStruct dXorg::getClocks(const QString &data) {
    globalStuff::gpuClocksStruct tData(-1); // empty struct

    // if nothing is there returns empty (-1) struct
    if (data.isEmpty()){
        qDebug() << "Can't get clocks, no data available";
        return tData;
    }

    switch (currentPowerMethod) {
    case globalStuff::DPM: {
        QRegExp rx;

        rx.setPattern("power\\slevel\\s\\d");
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty())
            tData.powerLevel = rx.cap(0).split(' ')[2].toShort();

        rx.setPattern("sclk:\\s\\d+");
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty())
            tData.coreClk = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100;

        rx.setPattern("mclk:\\s\\d+");
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty())
            tData.memClk = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100;

        rx.setPattern("vclk:\\s\\d+");
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty()) {
            tData.uvdCClk = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100;
            tData.uvdCClk  = (tData.uvdCClk  == 0) ? -1 :  tData.uvdCClk;
        }

        rx.setPattern("dclk:\\s\\d+");
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty()) {
            tData.uvdDClk = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble() / 100;
            tData.uvdDClk = (tData.uvdDClk == 0) ? -1 : tData.uvdDClk;
        }

        rx.setPattern("vddc:\\s\\d+");
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty())
            tData.coreVolt = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble();

        rx.setPattern("vddci:\\s\\d+");
        rx.indexIn(data);
        if (!rx.cap(0).isEmpty())
            tData.memVolt = rx.cap(0).split(' ',QString::SkipEmptyParts)[1].toDouble();

        return tData;
        break;
    }
    case globalStuff::PROFILE: {
        QStringList clocksData = data.split("\n");
        for (int i=0; i< clocksData.count(); i++) {
            switch (i) {
            case 1: {
                if (clocksData[i].contains("current engine clock")) {
                    tData.coreClk = QString().setNum(clocksData[i].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000).toDouble();
                    break;
                }
            };
            case 3: {
                if (clocksData[i].contains("current memory clock")) {
                    tData.memClk = QString().setNum(clocksData[i].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[3].toFloat() / 1000).toDouble();
                    break;
                }
            }
            case 4: {
                if (clocksData[i].contains("voltage")) {
                    tData.coreVolt = QString().setNum(clocksData[i].split(' ',QString::SkipEmptyParts,Qt::CaseInsensitive)[1].toFloat()).toDouble();
                    break;
                }
            }
            }
        }
        return tData;
        break;
    }
    case globalStuff::PM_UNKNOWN: {
        qWarning() << "Unknown power method, can't get clocks";
        return tData;
        break;
    }
    }
    return tData;
}

float dXorg::getTemperature() {
    QString temp;

    switch (currentTempSensor) {
    case SYSFS_HWMON:
    case CARD_HWMON: {
        QFile hwmon(filePaths.sysfsHwmonTempPath);
        hwmon.open(QIODevice::ReadOnly);
        temp = hwmon.readLine(20);
        hwmon.close();
        return temp.toDouble() / 1000;
        break;
    }
    case PCI_SENSOR: {
        QStringList out = globalStuff::grabSystemInfo("sensors");
        temp = out[sensorsGPUtempIndex+2].split(" ",QString::SkipEmptyParts)[1].remove("+").remove("C").remove("°");
        break;
    }
    case MB_SENSOR: {
        QStringList out = globalStuff::grabSystemInfo("sensors");
        temp = out[sensorsGPUtempIndex].split(" ",QString::SkipEmptyParts)[1].remove("+").remove("C").remove("°");
        break;
    }
    case TS_UNKNOWN: {
        temp = "-1";
        break;
    }
    }
    return temp.toDouble();
}

// Function that returns the human readable output of a property value
// For reference:
// http://cgit.freedesktop.org/xorg/app/xrandr/tree/xrandr.c#n2408
static QString translateProperty(Display * display,
                                 const int propertyDataFormat, // 8 / 16 / 32 bit
                                 const Atom propertyDataType, // ATOM / INTEGER / CARDINAL
                                 const Atom * propertyRawData){ // Pointer to the property value data array
    QString output;
    bool translated = false;

    switch(propertyDataType){
    case ATOM_VALUE: // Text value, like 'off' or 'None'
        if(propertyDataFormat != 32)
            break; // Only 32 bit supported here
        output.append(XGetAtomName(display, propertyRawData[0]));
        translated = true;
        break;

    case INTEGER_VALUE: // Signed numeric value
        switch(propertyDataFormat){
        case 8:
            output.append((signed char) propertyRawData[0]);
            translated = true;
            break;

        case 16:
            output.append(QString("%1").arg((signed short) propertyRawData[0]));
            translated = true;
            break;

        case 32:
            output.append(QString("%1").arg((signed long) propertyRawData[0]));
            translated = true;
        }
        break;

    case CARDINAL_VALUE: // Unsigned numeric value
        switch(propertyDataFormat){
        case 8:
            output.append((unsigned char) propertyRawData[0]);
            translated = true;
            break;

        case 16:
            output.append(QString("%1").arg((unsigned short) propertyRawData[0]));
            translated = true;
            break;

        case 32:
            output.append(QString("%1").arg((unsigned long) propertyRawData[0]));
            translated = true;
        }
    }

    if( ! translated) // Unknown type, print as decimal
        output.append(QString("%1").arg(propertyRawData[0])).append('?');

    return output;
}

// Get the real vendor name from the three-letter PNP ID
// See http://www.uefi.org/pnp_id_list
static QString translatePnpId(const QString pnpId){
    if (pnpId.isEmpty())
        return pnpId;

    // Search a file mapping PnP IDs to vendor names
    // List found through pkgfile --verbose --search pnp.ids
    QStringList possiblePaths = QStringList()
            << "/usr/share/libgnome-desktop-3.0/pnp.ids"
            << "/usr/share/libgnome-desktop/pnp.ids"
            << "/usr/share/libcinnamon-desktop/pnp.ids"
            << "/usr/share/dispcalGUI/pnp.ids"
            << "/usr/share/libmate-desktop/pnp.ids";

    for(int i=0;  i < possiblePaths.length(); i++){ // Cycle until we found the name or finished the possible paths
        QFile pnpIds(possiblePaths.at(i));
        if( ! pnpIds.exists() || ! pnpIds.open(QIODevice::ReadOnly)) // If not available
            continue; // Next file

        while ( ! pnpIds.atEnd()) { // Continue reading the file until the name is found or the file has ended
            QString line = pnpIds.readLine();
            if ( ! line.startsWith(pnpId))
                continue; // Wrong line, go to next line

            QStringList parts = line.split(QLatin1Char('\t'));
            if (parts.size() == 2)
                return parts.at(1).trimmed();
        }
    }

    return pnpId; // No real name found, return the PNP ID instead
}

// Get the human-readable monitor name (vendor + model) from the EDID
// See http://en.wikipedia.org/wiki/Extended_display_identification_data#EDID_1.3_data_format
static QString getMonitorName(const quint8 *EDID){
    QString monitorName;

    //Get the vendor PnP ID
    QString pnpId, modelName;
    pnpId[0] = 'A' + ((EDID[EDID_OFFSET_PNP_ID + 0] & 0x7c) / 4) - 1;
    pnpId[1] = 'A' + ((EDID[EDID_OFFSET_PNP_ID + 0] & 0x3) * 8) + ((EDID[EDID_OFFSET_PNP_ID + 1] & 0xe0) / 32) - 1;
    pnpId[2] = 'A' + (EDID[EDID_OFFSET_PNP_ID + 1] & 0x1f) - 1;

    //Translate the PnP ID into human-readable vendor name
    monitorName.append(translatePnpId(pnpId));

    // Get the model name
    for (uint i = EDID_OFFSET_DATA_BLOCKS; i < EDID_OFFSET_LAST_BLOCK; i += 18)
        if(EDID[i+3] == EDID_DESCRIPTOR_PRODUCT_NAME)
            modelName = QString::fromLocal8Bit((const char*)&EDID[i+5], 12).trimmed();

    if(modelName.isEmpty())
        monitorName.append(" (model unknown)");
    else
        monitorName.append(' ').append(modelName);

    return monitorName;
}

// Function that returns the right info struct for the RRMode it receives
XRRModeInfo * getModeInfo(XRRScreenResources * screenResources, RRMode mode){
    // XRRGetModeInfo does not exist (damn it) so we must manually search the right XRRModeInfo between the info structs of the screen configuration
    for(int modeInfoIndex=0; modeInfoIndex < screenResources->nmode; modeInfoIndex++){ // For each XRRModeInfo available
        if(screenResources->modes[modeInfoIndex].id == mode) // If we found the right modeInfo
            return &screenResources->modes[modeInfoIndex];// We found it, we can exit the loop
    }

    return NULL; // We haven't found it
}

// Function that returns the vertical refresh rate in Hz from a XRRModeInfo
float getVerticalRefreshRate(XRRModeInfo * modeInfo){
    if( ! modeInfo || ! modeInfo->hTotal || ! modeInfo->vTotal) // 'modeInfo' is null or values are absent
        return -1;

    float vTotal = modeInfo->vTotal;

    if(modeInfo->modeFlags & RR_DoubleScan) // https://en.wikipedia.org/wiki/Dual_Scan
        vTotal *= 2;

    if(modeInfo->modeFlags & RR_Interlace) // https://en.wikipedia.org/wiki/Interlaced_video
        vTotal /= 2;

    return (float)modeInfo->dotClock / ((float)modeInfo->hTotal * vTotal);
}

QList<QTreeWidgetItem *> dXorg::getCardConnectors() {
    QList<QTreeWidgetItem *> cardConnectorsList;

#ifdef QT_DEBUG // COMPILED ONLY IN DEBUG MODE
    // Old implementation, parsing the output of xrandr

    QStringList out = globalStuff::grabSystemInfo("xrandr -q --verbose"), screens;
    screens = out.filter(QRegExp("Screen\\s\\d"));

    for (int i = 0; i < screens.count(); i++) {
        QTreeWidgetItem *item = new QTreeWidgetItem(QStringList() << screens[i].split(':')[0] << screens[i].split(",")[1].remove(" current "));
        cardConnectorsList.append(item);
    }
    cardConnectorsList.append(new QTreeWidgetItem(QStringList() << "------"));

    for (int i = 0; i < out.size(); i++) {
        if (!out[i].startsWith("\t") && out[i].contains("connected")) {
            QString connector = out[i].split(' ')[0],
                    status = out[i].split(' ')[1],
                    res = out[i].split(' ')[2].split('+')[0];

            if (status == "connected") {
                QString monitor, edid = monitor = "";

                // find EDID
                for (int i = out.indexOf(QRegExp(".+EDID.+"))+1; i < out.count(); i++)
                    if (out[i].startsWith(("\t\t")))
                        edid += out[i].remove("\t\t");
                    else
                        break;

                // Parse EDID
                // See http://en.wikipedia.org/wiki/Extended_display_identification_data#EDID_1.3_data_format
                if (edid.size() >= 256) {
                    QStringList hex;
                    bool found = false, ok = true;
                    int i2 = 108;

                    for(int i3 = 0; i3 < 4; i3++) {
                        if(edid.mid(i2, 2).toInt(&ok, 16) == 0 && ok &&
                                edid.mid(i2 + 2, 2).toInt(&ok, 16) == 0) {
                            // Other Monitor Descriptor found
                            if(ok && edid.mid(i2 + 6, 2).toInt(&ok, 16) == 0xFC && ok) {
                                // Monitor name found
                                for(int i4 = i2 + 10; i4 < i2 + 34; i4 += 2)
                                    hex << edid.mid(i4, 2);
                                found = true;
                                break;
                            }
                        }
                        if(!ok)
                            break;
                        i2 += 36;
                    }
                    if (ok && found) {
                        // Hex -> String
                        for(i2 = 0; i2 < hex.size(); i2++) {
                            monitor += QString(hex[i2].toInt(&ok, 16));
                            if(!ok)
                                break;
                        }

                        if(ok)
                            monitor = " (" + monitor.left(monitor.indexOf('\n')) + ")";
                    }
                }
                status += monitor + " @ " + QString((res.contains('x')) ? res : "unknown");
            }

            QTreeWidgetItem *item = new QTreeWidgetItem(QStringList() << connector << status);
            cardConnectorsList.append(item);
        }
    }

#endif // #ifdef QT_DEBUG

    // Using libXrandr to get connectors, resolutions and dimensions
    // Offline docs: man XRRGetScreenInfo
    // Official docs: http://www.x.org/releases/current/doc/man/man3/Xrandr.3.html
    // Wiki page: http://www.x.org/wiki/libraries/libxrandr/
    // XRandr source code (useful as reference): http://cgit.freedesktop.org/xorg/app/xrandr/tree/xrandr.c

    Display* display = XOpenDisplay(NULL); // Get the connection to the X server.
    if( ! display){
        qWarning() << "Error loading connectors: can't connect to X server";
        return cardConnectorsList;
    }

    for(int screenIndex = 0; screenIndex < ScreenCount(display); screenIndex++){ // For each screen in this connection
        // Create root QTreeWidgetItem item for this screen
        QTreeWidgetItem * screenItem = new QTreeWidgetItem(QStringList() << QString("Screen configuration ").append(QString::number(screenIndex)));
        cardConnectorsList.append(screenItem);

        // Add resolution
        screenItem->addChild(new QTreeWidgetItem(QStringList()
                                                 << "Resolution"
                                                 << QString::number(DisplayWidth(display,screenIndex)).append('x')
                                                 .append(QString::number(DisplayHeight(display, screenIndex)))));

        // Add screen minimum and maximum resolutions
        int screenMinWidth, screenMinHeight, screenMaxWidth, screenMaxHeight;
        Window screenRoot = RootWindow(display, screenIndex);
        XRRGetScreenSizeRange(display, screenRoot, &screenMinWidth, &screenMinHeight, &screenMaxWidth, &screenMaxHeight);
        screenItem->addChild(new QTreeWidgetItem(QStringList()
                                                 << "Minimum resolution"
                                                 << QString::number(screenMinWidth).append('x').append(QString::number(screenMinHeight))));
        screenItem->addChild(new QTreeWidgetItem(QStringList()
                                                 << "Maximum resolution"
                                                 << QString::number(screenMaxWidth).append('x').append(QString::number(screenMaxHeight))));

        // Adding screen virtual dimension, in millimeters
        screenItem->addChild(new QTreeWidgetItem(QStringList()
                                                 << "Virtual dimensions"
                                                 << QString::number(DisplayWidthMM(display, screenIndex)).append("mm x ")
                                                 .append(QString::number(DisplayHeightMM(display, screenIndex))).append("mm")));

        // Retrieve screen resources (connectors, configurations, timestamps etc.)
        XRRScreenResources * screenResources = XRRGetScreenResources(display, screenRoot);
        if ( ! screenResources){
            qWarning() << "Error loading connectors: can't get resources for screen " << QString::number(screenIndex);
            continue; // Next screen
        }

        // Creating root QTreeWidgetItem for this screen's outputs
        QTreeWidgetItem * outputListItem = new QTreeWidgetItem(QStringList() << "Outputs");
        screenItem->addChild(outputListItem);
        int screenConnectedOutputs = 0, screenActiveOutputs = 0;

        //Cycle through outputs of this screen
        for(int outputIndex = 0; outputIndex < screenResources->noutput; outputIndex++){
            qDebug() << "Analyzing output " << QString::number(outputIndex) << " of screen " << QString::number(screenIndex);

            // Get output info (connection name, current configuration, dimensions, etc)
            XRROutputInfo * outputInfo = XRRGetOutputInfo(display, screenResources, screenResources->outputs[outputIndex]);
            if( ! outputInfo){
                qWarning() << "Error loading connectors: can't retrieve info for output "
                           << QString::number(outputIndex)
                           << " of screen "
                           << QString::number(screenIndex);
                continue; // Next output
            }

            // Creating root QTreeWidgetItem item for this output
            QTreeWidgetItem *outputItem = new QTreeWidgetItem(QStringList() << QString::fromLocal8Bit(outputInfo->name));
            outputListItem->addChild(outputItem);

            // Check the output connection state
            if(outputInfo->connection){ // No connection
                outputItem->setText(1, "Disconnected");
                XRRFreeOutputInfo(outputInfo); // Deallocate the memory of this output's info
                continue; // Next output
            }

            screenConnectedOutputs++;

            // Get configuration info (resolution, offset, modes, and other things available only if the output is active)
            XRRCrtcInfo * configInfo = XRRGetCrtcInfo(display, screenResources, outputInfo->crtc);
            RRMode outputCurrentMode;
            if( ! configInfo) // The screen is disabled via software (likely turned off)
                outputItem->addChild(new QTreeWidgetItem(QStringList() << "Active" << "No"));
            else { // The screen is active
                outputItem->addChild(new QTreeWidgetItem(QStringList() << "Active" << "Yes"));
                screenActiveOutputs++;
                outputCurrentMode = configInfo->mode;

                // Add current resolution
                outputItem->addChild(new QTreeWidgetItem(QStringList()
                                                         << "Resolution"
                                                         << QString::number(configInfo->width).append('x').append(QString::number(configInfo->height))));

                // Add the refresh rate
                outputItem->addChild(new QTreeWidgetItem(QStringList()
                                                         << "Refresh rate"
                                                         << QString::number(getVerticalRefreshRate(getModeInfo(screenResources, outputCurrentMode)), 'g', 3).append(" Hz")));

                // Add the position in the current configuration (useful only in multi-head)
                outputItem->addChild(new QTreeWidgetItem(QStringList()
                                                         << "Offset"
                                                         << QString::number(configInfo->x).append(", ").append(QString::number(configInfo->y))));
            }

            // Add monitor size
            outputItem->addChild(new QTreeWidgetItem(QStringList()
                                                     << "Monitor size"
                                                     << QString::number(outputInfo->mm_width).append("mm x ")
                                                     .append(QString::number(outputInfo->mm_height)).append("mm")));

            // Create the root QTreeWidgetItem of the possible modes (resolution, Refresh rate, HSync, VSync, etc) list
            QTreeWidgetItem * modeListItem = new QTreeWidgetItem(QStringList() << "Supported modes");
            outputItem->addChild(modeListItem);

            for(int modeIndex = 0; modeIndex < outputInfo->nmode; modeIndex++){ // For each possible mode
                XRRModeInfo * modeInfo = getModeInfo(screenResources, outputInfo->modes[modeIndex]); // Get the info struct for this mode
                if( ! modeInfo) // Mode info not found
                    continue; // Proceed to next mode

                QString modeName, modeDetails; // Get the mode resolution (the name)
                modeName.append(QString::fromLocal8Bit(modeInfo->name));

                if(modeInfo->id == outputCurrentMode) // If this mode is the currently active mode
                    modeName.append(" (active)");

                // Gather mode details
                // http://en.tldp.org/HOWTO/XFree86-Video-Timings-HOWTO/basic.html
                if(modeInfo->dotClock && modeInfo->vTotal && modeInfo->hTotal){ // We need those values
                    float verticalRefreshRate = getVerticalRefreshRate(modeInfo),
                            horizontalRefreshRate = ((float)modeInfo->dotClock / (float) modeInfo->hTotal) / 1000.0,
                            dotClock = (float)modeInfo->dotClock / 1000000.0;

                    modeDetails.append(QString::number(verticalRefreshRate, 'g', 3)).append(" Hz vertical, ")
                            .append(QString::number(horizontalRefreshRate, 'g', 3)).append(" KHz horizontal, ")
                            .append(QString::number(dotClock, 'g', 3)).append(" MHz dot clock");
                }

                if(modeInfo->modeFlags & RR_DoubleScan)
                    modeDetails.append(", Double scan");

                if(modeInfo->modeFlags & RR_Interlace)
                    modeDetails.append(", Interlaced");

                if(modeIndex == outputInfo->npreferred)
                    modeDetails.append(" (preferred)");

                modeListItem->addChild(new QTreeWidgetItem(QStringList() << modeName << modeDetails)); // Add the mode to the tree
            }

            // Create the root QTreeWidgetItem of the property list
            QTreeWidgetItem * propertyListItem = new QTreeWidgetItem(QStringList() << "Properties");
            outputItem->addChild(propertyListItem);

            // Get this output properties (EDID, audio, scaling mode, etc)
            int propertyCount;
            Atom * properties = XRRListOutputProperties(display, screenResources->outputs[outputIndex], &propertyCount);

            // Cycle through this output's properties
            for(int propertyIndex = 0; propertyIndex < propertyCount; propertyIndex++){
                // Prepare this property's name and value
                QString propertyName = QString::fromLocal8Bit(XGetAtomName(display, properties[propertyIndex])), propertyValue;

                // Get the property raw value
                Atom propertyDataType;
                int propertyDataFormat;
                unsigned char *propertyRawData;
                unsigned long propertyDataSize, propertyDataBytesAfter;
                XRRGetOutputProperty(display,
                                     screenResources->outputs[outputIndex], // Current output
                                     properties[propertyIndex], // Current property
                                     0, 100, False, False, AnyPropertyType,
                                     &propertyDataType, // Property type will be returned here
                                     &propertyDataFormat,
                                     &propertyDataSize, // Length of the property
                                     &propertyDataBytesAfter,
                                     &propertyRawData); // The raw data content of the property

                // Translate the property value to human readable
                // http://us.download.nvidia.com/XFree86/Linux-x86-ARM/361.16/README/xrandrextension.html#randr-properties
                if( ! propertyName.compare("EDID")){ // EDID found
                    // See http://en.wikipedia.org/wiki/Extended_display_identification_data#EDID_1.3_data_format
                    if(propertyDataSize < 128){ // EDID is invalid
                        qWarning() << "EDID is malformed, skipping";
                        continue;
                    }
                    QByteArray rawEDID;

                    for(unsigned long i = 0; i < propertyDataSize; i++){ // For each uchar in raw data
                        if((i != 0) && ! (i % 16)) // Every 32 chars go on new line
                            propertyValue.append('\n');
                        propertyValue.append(QString("%1").arg(propertyRawData[i], 2, 16, QChar('0'))); // uchar -> readable HEX code
                        rawEDID.append(propertyRawData[i]);
                    }

                    // Get the monitor name from the EDID
                    // https://en.wikipedia.org/wiki/Extended_Display_Identification_Data#EDID_1.3_data_format

                    // This part is inspired by
                    // https://github.com/KDE/libkscreen/blob/master/src/edid.cpp
                    // <3

                    const quint8 *data = (quint8*) rawEDID.constData();
                    if (data[0] != 0x00 || data[1] != 0xff) {
                        qWarning() << "Can't parse EDID: invalid header";
                        continue;
                    }

                    outputItem->setText(1, "Connected: " + getMonitorName(data)); // Add the monitor name to the tree as value of the Output Item

                    // Get the model number
                    quint16 modelNumber = static_cast<quint16>(data[EDID_OFFSET_MODEL_NUMBER]);
                    if(modelNumber > 0)
                        propertyListItem->addChild(new QTreeWidgetItem(QStringList()
                                                                       << "Model number"
                                                                       << QString::number(modelNumber)));

                    // Get the serial number
                    quint32 serialNumber = static_cast<quint32>(data[EDID_OFFSET_SERIAL_NUMBER]);
                    serialNumber += static_cast<quint32>(data[EDID_OFFSET_SERIAL_NUMBER + 1] * 0x100);
                    serialNumber += static_cast<quint32>(data[EDID_OFFSET_SERIAL_NUMBER + 2] * 0x10000);
                    serialNumber += static_cast<quint32>(data[EDID_OFFSET_SERIAL_NUMBER + 3] * 0x1000000);
                    if (serialNumber > 0)
                        propertyListItem->addChild(new QTreeWidgetItem(QStringList()
                                                                       << "Serial number"
                                                                       << QString::number(serialNumber)));
                } else if ( ! propertyName.compare("GUID")){ // GUID found
                    for(unsigned long i = 0; i < propertyDataSize; i++){ // For each uchar
                        propertyValue.append(QString("%1").arg(propertyRawData[i], 2, 16, QChar('0'))); // uchar -> readable HEX code
                        if(i % 2) // Separator every two chars
                            propertyValue.append('-');
                    } // Finished parsing the GUID property
                } else { // Property is not EDID nor GUID

                    // Translate the value ( translatePropertyValue() will handle it)
                    for(unsigned long i = 0; i < propertyDataSize; i++)
                        propertyValue.append(translateProperty(display, propertyDataFormat, propertyDataType, (Atom*)&propertyRawData[i]));

                    // If the property has other possible values, print them
                    // Get the property informations (allows to get ranges)
                    XRRPropertyInfo *propertyInfo = XRRQueryOutputProperty(display, screenResources->outputs[outputIndex], properties[propertyIndex]);

                    // Proceed to print the range or the list alternatives, if they are present
                    if(propertyInfo->num_values > 0){ // Something is present
                        propertyValue.append( propertyInfo->range ? " (Range: " : " (Supported: " );
                        int rangeValuesIndex = 0;
                        while( rangeValuesIndex < propertyInfo->num_values ){ // Until there is another alternative/range available
                            propertyValue.append(translateProperty(display, 32, propertyDataType, (Atom*)&propertyInfo->values[rangeValuesIndex])); // Minimum value
                            rangeValuesIndex++;

                            if(propertyInfo->range){ // The alternative values are part of a range
                                propertyValue.append(" - "); // Separator
                                propertyValue.append(translateProperty(display, 32, propertyDataType, (Atom*)&propertyInfo->values[rangeValuesIndex+1])); // Maximum value
                                rangeValuesIndex ++;
                            }

                            if(rangeValuesIndex < propertyInfo->num_values) // Another range is available
                                propertyValue.append(", ");
                        }
                        propertyValue.append(')');
                    }
                    // We translated the property: deallocate the informations memory and exit
                    free(propertyInfo);
                }
                // We got both name and value for this property: print them and exit
                QTreeWidgetItem *propertyItem = new QTreeWidgetItem(QStringList() << propertyName << propertyValue); // Create the root QTreeWidgetItem for this property
                propertyListItem->addChild(propertyItem);
            }
            // We got all the details of this output: deallocate the memory of configuration and output infos and exit
            XRRFreeCrtcInfo(configInfo);
            XRRFreeOutputInfo(outputInfo);
        }
        // We checked all the outputs of this screen: print how many of them are connected and active, deallocate the screen resources memory and exit
         // Insert the number of connected outputs as value of the output tree item
        outputListItem->setText(1, QString::number(screenConnectedOutputs).append(" connected, ").append(QString::number(screenActiveOutputs).append(" active")));
        XRRFreeScreenResources(screenResources);
    }

    return cardConnectorsList;
}

globalStuff::powerMethod dXorg::getPowerMethod() {
    QFile powerMethodFile(filePaths.powerMethodFilePath);
    if (powerMethodFile.open(QIODevice::ReadOnly)) {
        QString s = powerMethodFile.readLine(20);

        if (s.contains("dpm",Qt::CaseInsensitive))
            return globalStuff::DPM;
        else if (s.contains("profile",Qt::CaseInsensitive))
            return globalStuff::PROFILE;
        else
            return globalStuff::PM_UNKNOWN;
    } else
        return globalStuff::PM_UNKNOWN;
}

dXorg::tempSensor dXorg::testSensor() {
    QFile hwmon(filePaths.sysfsHwmonTempPath);

    // first method, try read temp from sysfs in card dir (path from figureOutGPUDataPaths())
    if (hwmon.open(QIODevice::ReadOnly)) {
        if (!QString(hwmon.readLine(20)).isEmpty())
            return CARD_HWMON;
    } else {
        // second method, try find in system hwmon dir for file labeled VGA_TEMP
        filePaths.sysfsHwmonTempPath = findSysfsHwmonForGPU();
        if (!filePaths.sysfsHwmonTempPath.isEmpty())
            return SYSFS_HWMON;

        // if above fails, use lm_sensors
        QStringList out = globalStuff::grabSystemInfo("sensors");
        if (out.indexOf(QRegExp("radeon-pci.+")) != -1) {
            sensorsGPUtempIndex = out.indexOf(QRegExp("radeon-pci.+"));  // in order to not search for it again in timer loop
            return PCI_SENSOR;
        }
        else if (out.indexOf(QRegExp("VGA_TEMP.+")) != -1) {
            sensorsGPUtempIndex = out.indexOf(QRegExp("VGA_TEMP.+"));
            return MB_SENSOR;
        }
    }
    return TS_UNKNOWN;
}

QString dXorg::findSysfsHwmonForGPU() {
    QStringList hwmonDev = globalStuff::grabSystemInfo("ls /sys/class/hwmon/");
    for (int i = 0; i < hwmonDev.count(); i++) {
        QStringList temp = globalStuff::grabSystemInfo("ls /sys/class/hwmon/"+hwmonDev[i]+"/device/").filter("label");

        for (int o = 0; o < temp.count(); o++) {
            QFile f("/sys/class/hwmon/"+hwmonDev[i]+"/device/"+temp[o]);
            if (f.open(QIODevice::ReadOnly))
                if (f.readLine(20).contains("VGA_TEMP")) {
                    f.close();
                    return f.fileName().replace("label", "input");
                }
        }
    }
    return "";
}

QStringList dXorg::getGLXInfo(QString gpuName, QProcessEnvironment env) {
    return globalStuff::grabSystemInfo("glxinfo",env).filter(QRegExp("direct|OpenGL.+:.+"));
}

QList<QTreeWidgetItem *> dXorg::getModuleInfo() {
    QList<QTreeWidgetItem *> data;
    QStringList modInfo = globalStuff::grabSystemInfo("modinfo -p radeon");
    modInfo.sort();

    for (int i =0; i < modInfo.count(); i++) {
        if (modInfo[i].contains(":")) {
            // show nothing in case of an error
            if (modInfo[i].startsWith("modinfo: ERROR: ")) {
                continue;
            }
            // read module param name and description from modinfo command
            QString modName = modInfo[i].split(":",QString::SkipEmptyParts)[0],
                    modDesc = modInfo[i].split(":",QString::SkipEmptyParts)[1],
                    modValue;

            // read current param values
            QFile mp(filePaths.moduleParamsPath+modName);
            modValue = (mp.open(QIODevice::ReadOnly)) ?  modValue = mp.readLine(20) : "unknown";

            QTreeWidgetItem *item = new QTreeWidgetItem(QStringList() << modName.left(modName.indexOf('\n')) << modValue.left(modValue.indexOf('\n')) << modDesc.left(modDesc.indexOf('\n')));
            data.append(item);
            mp.close();
        }
    }
    return data;
}

QStringList dXorg::detectCards() {
    QStringList data;
    QStringList out = globalStuff::grabSystemInfo("ls /sys/class/drm/").filter("card");
    for (char i = 0; i < out.count(); i++) {
        QFile f("/sys/class/drm/"+out[i]+"/device/uevent");
        if (f.open(QIODevice::ReadOnly)) {
            if (f.readLine(50).contains("DRIVER=radeon"))
                data.append(f.fileName().split('/')[4]);
        }
    }
    return data;
}

QString dXorg::getCurrentPowerProfile() {
    switch (currentPowerMethod) {
    case globalStuff::DPM: {
        QFile dpmProfile(filePaths.dpmStateFilePath);
        if (dpmProfile.open(QIODevice::ReadOnly))
           return QString(dpmProfile.readLine(13));
        else
            return "err";
        break;
    }
    case globalStuff::PROFILE: {
        QFile profile(filePaths.profilePath);
        if (profile.open(QIODevice::ReadOnly))
            return QString(profile.readLine(13));
        break;
    }
    case globalStuff::PM_UNKNOWN:
        break;
    }

    return "err";
}

QString dXorg::getCurrentPowerLevel() {
    QFile forceProfile(filePaths.forcePowerLevelFilePath);
    if (forceProfile.open(QIODevice::ReadOnly))
        return QString(forceProfile.readLine(13));

    return "err";
}

void dXorg::setNewValue(const QString &filePath, const QString &newValue) {
    QFile file(filePath);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text)){
        QTextStream stream(&file);
        stream << newValue + "\n";
        if( ! file.flush() )
            qWarning() << "Failed to flush " << filePath;
        file.close();
    }else
        qWarning() << "Unable to open " << filePath << " to write " << newValue;
}

void dXorg::setPowerProfile(globalStuff::powerProfiles _newPowerProfile) {
    QString newValue;
    switch (_newPowerProfile) {
    case globalStuff::BATTERY:
        newValue = dpm_battery;
        break;
    case globalStuff::BALANCED:
        newValue = dpm_balanced;
        break;
    case globalStuff::PERFORMANCE:
        newValue = dpm_performance;
        break;
    case globalStuff::AUTO:
        newValue = profile_auto;
        break;
    case globalStuff::DEFAULT:
        newValue = profile_default;
        break;
    case globalStuff::HIGH:
        newValue = profile_high;
        break;
    case globalStuff::MID:
        newValue = profile_mid;
        break;
    case globalStuff::LOW:
        newValue = profile_low;
        break;
    default: break;
    }

    if (daemonConnected()) {
        QString command; // SIGNAL_SET_VALUE + SEPARATOR + VALUE + SEPARATOR + PATH + SEPARATOR
        command.append(DAEMON_SIGNAL_SET_VALUE).append(SEPARATOR); // Set value flag
        command.append(newValue).append(SEPARATOR); // Power profile to be set
        command.append(filePaths.dpmStateFilePath).append(SEPARATOR); // The path where the power profile should be written in

        qDebug() << "Sending daemon power profile signal: " << command;
        dcomm->sendCommand(command);
    } else {
        // enum is int, so first three values are dpm, rest are profile
        if (_newPowerProfile <= globalStuff::PERFORMANCE)
            setNewValue(filePaths.dpmStateFilePath,newValue);
        else
            setNewValue(filePaths.profilePath,newValue);
    }
}

void dXorg::setForcePowerLevel(globalStuff::forcePowerLevels _newForcePowerLevel) {
    QString newValue;
    switch (_newForcePowerLevel) {
    case globalStuff::F_AUTO:
        newValue = dpm_auto;
        break;
    case globalStuff::F_HIGH:
        newValue = dpm_high;
        break;
    case globalStuff::F_LOW:
        newValue = dpm_low;
    default:
        break;
    }

    if (daemonConnected()) {
        QString command; // SIGNAL_SET_VALUE + SEPARATOR + VALUE + SEPARATOR + PATH + SEPARATOR
        command.append(DAEMON_SIGNAL_SET_VALUE).append(SEPARATOR); // Set value flag
        command.append(newValue).append(SEPARATOR); // Power profile to be forcibly set
        command.append(filePaths.forcePowerLevelFilePath).append(SEPARATOR); // The path where the power profile should be written in

        qDebug() << "Sending daemon forced power profile signal: " << command;
        dcomm->sendCommand(command);
    } else
        setNewValue(filePaths.forcePowerLevelFilePath, newValue);
}

void dXorg::setPwmValue(int value) {
    if (daemonConnected()) {
        QString command; // SIGNAL_SET_VALUE + SEPARATOR + VALUE + SEPARATOR + PATH + SEPARATOR
        command.append(DAEMON_SIGNAL_SET_VALUE).append(SEPARATOR); // Set value flag
        command.append(QString::number(value)).append(SEPARATOR); // PWM value to be set
        command.append(filePaths.pwmSpeedPath).append(SEPARATOR); // The path where the PWM value should be written in

        qDebug() << "Sending daemon forced power profile signal: " << command;
        dcomm->sendCommand(command);
    } else
        setNewValue(filePaths.pwmSpeedPath,QString().setNum(value));
}

void dXorg::setPwmManuaControl(bool manual) {
    char mode = manual ? pwm_manual : pwm_auto;

    if (daemonConnected()) {
        //  Tell the daemon to set the pwm mode into the right file
        QString command; // SIGNAL_SET_VALUE + SEPARATOR + VALUE + SEPARATOR + PATH + SEPARATOR
        command.append(DAEMON_SIGNAL_SET_VALUE).append(SEPARATOR); // Set value flag
        command.append(mode).append(SEPARATOR); // The PWM mode to set
        command.append(filePaths.pwmEnablePath).append(SEPARATOR); // The path where the PWM mode should be written in

        qDebug() << "Sending daemon forced power profile signal: " << command;
        dcomm->sendCommand(command);
    } else  //  No daemon available
        setNewValue(filePaths.pwmEnablePath, QString(mode));
}

int dXorg::getPwmSpeed() {
    QFile f(filePaths.pwmSpeedPath);

    int val = 0;
    if (f.open(QIODevice::ReadOnly)) {
       val = QString(f.readLine(4)).toInt();
       f.close();
    }

    return val;
}

globalStuff::driverFeatures dXorg::figureOutDriverFeatures() {
    globalStuff::driverFeatures features;
    features.temperatureAvailable =  (currentTempSensor == dXorg::TS_UNKNOWN) ? false : true;

    QString data = getClocksRawData(true);
    globalStuff::gpuClocksStruct test = dXorg::getClocks(data);

    // still, sometimes there is miscomunication between daemon,
    // but vales are there, so look again in the file which daemon has
    // copied to /tmp/
    if (test.coreClk == -1)
        test = getFeaturesFallback();

    features.coreClockAvailable = !(test.coreClk == -1);
    features.memClockAvailable = !(test.memClk == -1);
    features.coreVoltAvailable = !(test.coreVolt == -1);
    features.memVoltAvailable = !(test.memVolt == -1);

    features.pm = currentPowerMethod;

    switch (currentPowerMethod) {
    case globalStuff::DPM: {
        if (daemonConnected())
            features.canChangeProfile = true;
        else {
            QFile f(filePaths.dpmStateFilePath);
            if (f.open(QIODevice::WriteOnly)){
                features.canChangeProfile = true;
                f.close();
            }
        }
        break;
    }
    case globalStuff::PROFILE: {
        QFile f(filePaths.profilePath);
        if (f.open(QIODevice::WriteOnly)) {
            features.canChangeProfile = true;
            f.close();
        }
    }
    case globalStuff::PM_UNKNOWN:
        break;
    }

    if (!filePaths.pwmEnablePath.isEmpty()) {
        QFile f(filePaths.pwmEnablePath);
        f.open(QIODevice::ReadOnly);

        if (QString(f.readLine(2))[0] != pwm_disabled) {
            features.pwmAvailable = true;

            QFile fPwmMax(filePaths.pwmMaxSpeedPath);
            if (fPwmMax.open(QIODevice::ReadOnly)) {
                features.pwmMaxSpeed = fPwmMax.readLine(4).toInt();
                fPwmMax.close();
            }
        }
        f.close();
    }
    return features;
}

globalStuff::gpuClocksStruct dXorg::getFeaturesFallback() {
    QFile f("/tmp/radeon_pm_info");
    if (f.open(QIODevice::ReadOnly)) {
        globalStuff::gpuClocksStruct fallbackFeatures;
        QString s = QString(f.readAll());

        // just look for it, if it is, the value is not important at this point
        if (s.contains("sclk"))
            fallbackFeatures.coreClk = 0;
        if (s.contains("mclk"))
            fallbackFeatures.memClk = 0;
        if (s.contains("vddc"))
            fallbackFeatures.coreClk = 0;
        if (s.contains("vddci"))
            fallbackFeatures.memClk = 0;

        f.close();
        return fallbackFeatures;
    } else
        return globalStuff::gpuClocksStruct(-1);
}
