// copyright marazmista @ 29.03.2014

#include "gpu.h"
#include "radeon_profile.h"

#include <cmath>
#include <QFile>
#include <QDebug>
#include <QtConcurrent/QtConcurrent>

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

#define MILLIMETERS_PER_INCH 0.0393700787402

// List of files mapping PnP IDs to vendor names
// List found through pkgfile --verbose --search pnp.ids (on arch linux)
#define PNP_ID_FILE_COUNT 8
static const char * pnpIdFiles [PNP_ID_FILE_COUNT] = {
    "/usr/share/misc/pnp.ids",
    "/usr/share/libgnome-desktop-3.0/pnp.ids",
    "/usr/share/libgnome-desktop/pnp.ids",
    "/usr/share/libcinnamon-desktop/pnp.ids",
    "/usr/share/dispcalGUI/pnp.ids",
    "/usr/share/DisplayCAL/pnp.ids",
    "/usr/share/libmate-desktop/pnp.ids",
    "/usr/share/hwdata/pnp.ids"
};

void gpu::detectCards() {
    QStringList out = globalStuff::grabSystemInfo("ls /sys/class/drm/").filter(QRegExp("card\\d$"));

    for (int i = 0; i < out.count(); i++) {
        QFile f("/sys/class/drm/"+out[i]+"/device/uevent");

        if (!f.open(QIODevice::ReadOnly))
            continue;

        QStringList ueventContents = QString(f.readAll()).split('\n');
        f.close();

        int driverIdx = ueventContents.indexOf(QRegExp("DRIVER=[radeon|amdgpu]+"));
        if (driverIdx == -1)
            continue;

        GPUSysInfo gsi;
        gsi.driverModuleString = ueventContents[driverIdx].split('=')[1];
        if (gsi.driverModuleString == "radeon")
            gsi.module = DriverModule::RADEON;
        else if (gsi.driverModuleString == "amdgpu")
            gsi.module = DriverModule::AMDGPU;
        else
            gsi.module = DriverModule::MODULE_UNKNOWN;

        gsi.sysName = gsi.name = out[i];

        int pciIdx = ueventContents.indexOf(QRegExp("PCI_SLOT_NAME.+"));
        if (pciIdx != -1)
            gsi.name = globalStuff::grabSystemInfo("lspci -s " + ueventContents[pciIdx].split('=')[1])[0].split(':')[2].trimmed();

        gpuList.append(gsi);

        qDebug() << "Card detected:\n module: " << gsi.driverModuleString <<  "\n sysName(path): "  << gsi.sysName  << "\n name: " <<  gsi.name;
    }
}

bool gpu::initialize(const dXorg::InitializationConfig &config) {
    qDebug() << "Initializing device";

    detectCards();

    if (gpuList.size() == 0) {
        qWarning() << "No cards found!";
        return false;
    }

    driverHandler = new dXorg(gpuList.at(0), config);
    defineAvailableDataContainer();

    return true;
}

bool gpu::isInitialized() {
    return gpuList.count() > 0;
}

void gpu::changeGpu(int index) {
    dXorg::InitializationConfig initConfig = driverHandler->getInitConfig();
    delete driverHandler;

    driverHandler = new dXorg(gpuList.at(index), initConfig);
    driverHandler->configure();
    defineAvailableDataContainer();
}

void gpu::defineAvailableDataContainer() {
    GPUClocks tmpClk = driverHandler->getClocks();

    if (tmpClk.coreClk != -1)
        gpuData.insert(ValueID::CLK_CORE, RPValue(ValueUnit::MEGAHERTZ, tmpClk.coreClk));

    if (tmpClk.coreVolt != -1)
        gpuData.insert(ValueID::VOLT_CORE, RPValue(ValueUnit::MILIVOLT, tmpClk.coreVolt));

    if (tmpClk.memClk != -1)
        gpuData.insert(ValueID::CLK_MEM, RPValue(ValueUnit::MEGAHERTZ, tmpClk.memClk));

    if (tmpClk.memVolt != -1)
        gpuData.insert(ValueID::VOLT_MEM, RPValue(ValueUnit::MILIVOLT, tmpClk.memVolt));

    if (tmpClk.uvdCClk != -1)
        gpuData.insert(ValueID::CLK_UVD, RPValue(ValueUnit::MEGAHERTZ, tmpClk.uvdCClk));

    if (tmpClk.uvdDClk != -1)
        gpuData.insert(ValueID::DCLK_UVD, RPValue(ValueUnit::MEGAHERTZ, tmpClk.uvdDClk));

    if (tmpClk.powerLevel != -1)
        gpuData.insert(ValueID::POWER_LEVEL, RPValue(ValueUnit::NONE, tmpClk.powerLevel));


    GPUFanSpeed tmpPwm = driverHandler->getFanSpeed();

    if (tmpPwm.fanSpeedPercent != -1)
        gpuData.insert(ValueID::FAN_SPEED_PERCENT, RPValue(ValueUnit::PERCENT, tmpPwm.fanSpeedPercent));

    if (tmpPwm.fanSpeedRpm != -1)
        gpuData.insert(ValueID::FAN_SPEED_RPM, RPValue(ValueUnit::RPM, tmpPwm.fanSpeedRpm));


    float tmpTemp = driverHandler->getTemperature();

    if (tmpTemp != -1) {
        gpuData.insert(ValueID::TEMPERATURE_CURRENT, RPValue(ValueUnit::CELSIUS, tmpTemp));
        gpuData.insert(ValueID::TEMPERATURE_BEFORE_CURRENT, RPValue(ValueUnit::CELSIUS, tmpTemp));
        gpuData.insert(ValueID::TEMPERATURE_MIN, RPValue(ValueUnit::CELSIUS, tmpTemp));
        gpuData.insert(ValueID::TEMPERATURE_MAX, RPValue(ValueUnit::CELSIUS, tmpTemp));
    }

    GPUUsage tmpUsage = driverHandler->getGPUUsage();

    if (tmpUsage.gpuUsage != -1)
        gpuData.insert(ValueID::GPU_USAGE_PERCENT, RPValue(ValueUnit::PERCENT, tmpUsage.gpuUsage));

    if (tmpUsage.gpuVramUsage != -1)
        gpuData.insert(ValueID::GPU_VRAM_USAGE_MB, RPValue(ValueUnit::MEGABYTE, tmpUsage.gpuVramUsage));

    if (tmpUsage.gpuVramUsagePercent != -1)
        gpuData.insert(ValueID::GPU_VRAM_USAGE_PERCENT, RPValue(ValueUnit::PERCENT, tmpUsage.gpuVramUsagePercent));


    if (driverHandler->features.isPowerCapAvailable) {
        int tmpPowerCap = driverHandler->getPowerCapSelected();

        if (tmpPowerCap != -1)
            gpuData.insert(ValueID::POWER_CAP_SELECTED, RPValue(ValueUnit::WATT, tmpPowerCap));

        tmpPowerCap = driverHandler->getPowerCapAverage();

        if (tmpPowerCap != -1)
            gpuData.insert(ValueID::POWER_CAP_AVERAGE, RPValue(ValueUnit::WATT, tmpPowerCap));
    }
}

void gpu::getClocks() {
    GPUClocks tmp = driverHandler->getClocks();

    if (gpuData.contains(ValueID::CLK_CORE))
        gpuData[ValueID::CLK_CORE].setValue(tmp.coreClk);

    if (gpuData.contains(ValueID::VOLT_CORE))
         gpuData[ValueID::VOLT_CORE].setValue(tmp.coreVolt);

    if (gpuData.contains(ValueID::CLK_MEM))
         gpuData[ValueID::CLK_MEM].setValue(tmp.memClk);

    if (gpuData.contains(ValueID::VOLT_MEM))
         gpuData[ValueID::VOLT_MEM].setValue(tmp.memVolt);

    if (gpuData.contains(ValueID::CLK_UVD))
         gpuData[ValueID::CLK_UVD].setValue(tmp.uvdCClk);

    if (gpuData.contains(ValueID::DCLK_UVD))
         gpuData[ValueID::DCLK_UVD].setValue(tmp.uvdDClk);

    if (gpuData.contains(ValueID::POWER_LEVEL))
         gpuData[ValueID::POWER_LEVEL].setValue(tmp.powerLevel);
}

void gpu::getTemperature() {
    if (!gpuData.contains(ValueID::TEMPERATURE_CURRENT))
        return;

    gpuData[ValueID::TEMPERATURE_BEFORE_CURRENT].setValue(gpuData.value(ValueID::TEMPERATURE_CURRENT).value);
    gpuData[ValueID::TEMPERATURE_CURRENT].setValue(driverHandler->getTemperature());

    if (gpuData.value(ValueID::TEMPERATURE_MIN, RPValue()).value > gpuData.value(ValueID::TEMPERATURE_CURRENT).value)
        gpuData[ValueID::TEMPERATURE_MIN].setValue(gpuData.value(ValueID::TEMPERATURE_CURRENT).value);

    if (gpuData.value(ValueID::TEMPERATURE_MAX, RPValue()).value < gpuData.value(ValueID::TEMPERATURE_CURRENT).value)
        gpuData[ValueID::TEMPERATURE_MAX].setValue(gpuData.value(ValueID::TEMPERATURE_CURRENT).value);
}

void gpu::getGpuUsage() {

    // getting gpu usage seems to be heavy and cause ui lag, so it is done in another thread
    futureGpuUsage.setFuture(QtConcurrent::run(driverHandler,&dXorg::getGPUUsage));
}

QList<QTreeWidgetItem *> gpu::getModuleInfo() const {
    return driverHandler->getModuleInfo();
}

QStringList gpu::getGLXInfo(QString gpuName) const {
    QStringList data;

    for (auto& gpu : gpuList)
        data << "VGA: " + gpu.name;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!gpuName.isEmpty())
        env.insert("DRI_PRIME",gpuName.at(gpuName.length()-1));
    QStringList driver = globalStuff::grabSystemInfo("xdriinfo",env).filter("Screen 0:",Qt::CaseInsensitive);
    if (!driver.isEmpty())  // because of segfault when no xdriinfo
        data << "OpenGL driver:"+ driver.filter("Screen 0:",Qt::CaseInsensitive)[0].split(":",QString::SkipEmptyParts)[1];

    data << "Kernel driver: " + gpuList.at(currentGpuIndex).driverModuleString;

    data << globalStuff::grabSystemInfo("glxinfo -B",env).filter(QRegExp(".+"));

    return data;
}

QString gpu::getCurrentPowerLevel() {
    return driverHandler->getCurrentPowerLevel();
}

QString gpu::getCurrentPowerProfile()  {
    return driverHandler->getCurrentPowerProfile();
}

void gpu::getPowerLevel() {
    currentPowerLevel = getCurrentPowerLevel();
    currentPowerProfile = getCurrentPowerProfile();
}

void gpu::setPowerProfile(const QString &newPowerProfile) {
    driverHandler->setPowerProfile(newPowerProfile);
}

void gpu::setForcePowerLevel(const QString &newForcePowerLevel) {
    driverHandler->setForcePowerLevel(newForcePowerLevel);
}

void gpu::setPwmValue(unsigned int value) {
    // If the PC is sent to sleep (or hibernate) it can happen that PWM is
    // disabled (by the OS..?) and that we have to re-enable it, if needed,
    // after returning from sleep
    static QFile pwmEnableFile(getDriverFiles().hwmonAttributes.pwm1_enable);
    if (Q_UNLIKELY(pwmEnableFile.open(QFile::ReadOnly)
                   && !pwmEnableFile.read(1).contains(pwm_manual)))
    {
        setPwmManualControl(true);
    }
    pwmEnableFile.close();

    value = getGpuConstParams().pwmMaxSpeed * value / 100;
    driverHandler->setNewValue(getDriverFiles().hwmonAttributes.pwm1, QString::number(value));
}

void gpu::setPwmManualControl(bool manual) {
    driverHandler->setNewValue(getDriverFiles().hwmonAttributes.pwm1_enable, QString(manual ? pwm_manual : pwm_auto));
}

void gpu::getFanSpeed() {
    if (!gpuData.contains(ValueID::FAN_SPEED_PERCENT))
        return;

    GPUFanSpeed tmp = driverHandler->getFanSpeed();

    gpuData[ValueID::FAN_SPEED_PERCENT].setValue(tmp.fanSpeedPercent);

    if (gpuData.contains(ValueID::FAN_SPEED_RPM))
        gpuData[ValueID::FAN_SPEED_RPM].setValue(tmp.fanSpeedRpm);
}

const DriverFeatures& gpu::getDriverFeatures() const {
    return driverHandler->features;
}

const GPUConstParams& gpu::getGpuConstParams() const {
    return driverHandler->params;
}

const DeviceFilePaths& gpu::getDriverFiles() const {
    return driverHandler->driverFiles;
}

void gpu::finalize() {
    if (gpuData.contains(ValueID::FAN_SPEED_PERCENT))
        setPwmManualControl(false);

    if (getDriverFeatures().isPercentCoreOcAvailable)
        resetOverclock();

    if (futureGpuUsage.isRunning())
        futureGpuUsage.waitForFinished();
}

void gpu::setOverclockValue(const QString &file, const int value) {
    driverHandler->setNewValue(file, QString::number(value));
}

void gpu::resetOverclock() {
    driverHandler->setNewValue(getDriverFiles().sysFs.pp_sclk_od, "0");
    driverHandler->setNewValue(getDriverFiles().sysFs.pp_mclk_od, "0");
}

void gpu::resetFrequencyControlStates() {
    QString statesCore, statesMem;

    for (int i = 0; i < driverHandler->features.sclkTable.count(); ++i)
        statesCore.append(QString::number(i) + " ");

    for (int i = 0; i < driverHandler->features.mclkTable.count(); ++i)
        statesMem.append(QString::number(i) + " ");

    setManualFrequencyControlStates(getDriverFiles().sysFs.pp_dpm_sclk, statesCore);
    setManualFrequencyControlStates(getDriverFiles().sysFs.pp_dpm_sclk, statesMem);
}

int gpu::getCurrentPowerPlayTableId(const QString &file) {
    return driverHandler->getCurrentPowerPlayTableId(file);
}

void gpu::refreshPowerPlayTables() {
    driverHandler->refreshPowerPlayTables();
}

void gpu::getPowerCapSelected() {
    if (getDriverFeatures().isPowerCapAvailable)
        gpuData[ValueID::POWER_CAP_SELECTED].setValue(driverHandler->getPowerCapSelected());
}

void gpu::setManualFrequencyControlStates(const QString &file, const QString &states) {
    driverHandler->setNewValue(file, states);
}

void gpu::getPowerCapAverage() {
    if (getDriverFeatures().isPowerCapAvailable)
        gpuData[ValueID::POWER_CAP_AVERAGE].setValue(driverHandler->getPowerCapAverage());
}

void gpu::setPowerCap(const unsigned int value) {
    driverHandler->setNewValue(getDriverFiles().hwmonAttributes.power1_cap, QString::number(value * MICROWATT_DIVIDER));
}

void gpu::setOcTableValue(const QString &type, const QString &tableKey, int powerState, const FreqVoltPair powerStateValues) {
    driverHandler->features.currentStatesTables[tableKey].insert(powerState, powerStateValues);

    driverHandler->setNewValue(getDriverFiles().sysFs.pp_od_clk_voltage,
                               type + " " + QString::number(powerState) + " " +
                               QString::number(powerStateValues.frequency) + " " +
                               QString::number(powerStateValues.voltage));
}

void gpu::sendOcTableCommand(const QString cmd) {
    driverHandler->setNewValue(getDriverFiles().sysFs.pp_od_clk_voltage, cmd);
}

void gpu::setOcRanges(const QString &type, const QString &tableKey, int powerState, int rangeValue) {
    OCRange &ocr = driverHandler->features.ocRages[tableKey];

    if (powerState == 0)
        ocr.min = rangeValue;
    else
        ocr.max = rangeValue;

    sendOcTableCommand(type + " " + QString::number(powerState) + " " + QString::number(rangeValue));
}

void gpu::readOcTableAndRanges() {
    driverHandler->readOcTableAndRanges();
}

void gpu::setOcTable(const QString &tableType, const FVTable &table) {
    driverHandler->setOcTable(tableType, table);

    readOcTableAndRanges();
}

// Function that returns the human readable output of a property value
// For reference:
// http://cgit.freedesktop.org/xorg/app/xrandr/tree/xrandr.c#n2408
QString translateProperty(Display * display,
                                 const int propertyDataFormat, // 8 / 16 / 32 bit
                                 const Atom propertyDataType, // ATOM / INTEGER / CARDINAL
                                 const Atom * propertyRawData){ // Pointer to the property value data array
    QString out;

    switch (propertyDataType) {
    case ATOM_VALUE: // Text value, like 'off' or 'None'
        if(propertyDataFormat == 32){ // Only 32 bit supported here
            char* string = XGetAtomName(display, *propertyRawData);
            if(string) // If it is not NULL
                out = QString(string);
            XFree(string);
        }
        break;

    case INTEGER_VALUE: // Signed numeric value
        switch (propertyDataFormat) {
            case 8: out = QString::number((qint8) *propertyRawData); break;
            case 16: out = QString::number((qint16) *propertyRawData); break;
            case 32: out = QString::number((qint32) *propertyRawData); break;
        }
        break;

    case CARDINAL_VALUE: // Unsigned numeric value
        switch (propertyDataFormat) {
            case 8: out = QString::number((quint8) *propertyRawData); break;
            case 16: out = QString::number((quint16) *propertyRawData); break;
            case 32: out = QString::number((quint32) *propertyRawData); break;
        }
            break;
    }

    return out.isEmpty() ? QString::number(*propertyRawData) : out; // If no match was found, return as number
}

// Get the real vendor name from the three-letter PNP ID
// See http://www.uefi.org/pnp_id_list
QString translatePnpId(const QString pnpId){
    if ( ! pnpId.isEmpty()){
        qDebug() << "Searching PnP ID: " << pnpId;
        for(int i=0;  i < PNP_ID_FILE_COUNT; i++){ // Cycle through the files
            QFile pnpIds(pnpIdFiles[i]);
            if(pnpIds.exists() && pnpIds.open(QIODevice::ReadOnly)){ // File is available
                while ( ! pnpIds.atEnd()){ // Continue reading the file until the file has ended
                    QString line = pnpIds.readLine();
                    if (line.startsWith(pnpId)){ // Right line
                        QStringList parts = line.split(QChar('\t')); // Separate PNP ID from the real name
                        if (parts.size() == 2){
                            qDebug() << "Found PnP ID: " << pnpId << "->" << parts.at(1).simplified();
                            pnpIds.close();
                            return parts.at(1).simplified(); // Get the real name
                        }
                    }
                }
            }
            pnpIds.close();
        }
    }
    return pnpId; // No real name found, return the PNP ID instead
}

// Get the human-readable monitor name (vendor + model) from the EDID
// See http://en.wikipedia.org/wiki/Extended_display_identification_data#EDID_1.3_data_format
// For reference: https://github.com/KDE/libkscreen/blob/master/src/edid.cpp#L262-L286
QString getMonitorName(const quint8 *EDID){
    QString monitorName;

    //Get the vendor PnP ID
    QString pnpId, modelName;
    pnpId[0] = 'A' + ((EDID[EDID_OFFSET_PNP_ID + 0] & 0x7c) / 4) - 1;
    pnpId[1] = 'A' + ((EDID[EDID_OFFSET_PNP_ID + 0] & 0x3) * 8) + ((EDID[EDID_OFFSET_PNP_ID + 1] & 0xe0) / 32) - 1;
    pnpId[2] = 'A' + (EDID[EDID_OFFSET_PNP_ID + 1] & 0x1f) - 1;

    //Translate the PnP ID into human-readable vendor name
    monitorName.append(translatePnpId(pnpId));

    // Get the model name
    for (uint i = EDID_OFFSET_DATA_BLOCKS; i <= EDID_OFFSET_LAST_BLOCK; i += 18)
        if(EDID[i+3] == EDID_DESCRIPTOR_PRODUCT_NAME)
            modelName = QString::fromLocal8Bit((const char*)&EDID[i+5], 13).simplified();

    monitorName.append(modelName.isEmpty() ? " - Model unknown" : ' ' + modelName);

    return monitorName;
}

// Function that returns the right info struct for the RRMode it receives
XRRModeInfo * getModeInfo(XRRScreenResources * screenResources, RRMode mode){
    // Cycle through all the XRRModeInfos of this screen and search the right one
    for(int modeInfoIndex=0; modeInfoIndex < screenResources->nmode; modeInfoIndex++){
        if(screenResources->modes[modeInfoIndex].id == mode) // If we found the right modeInfo
            return &screenResources->modes[modeInfoIndex];// We found it, we can exit the loop
    }

    return NULL; // We haven't found it
}

// Function that returns the horizontal refresh rate in Hz from a XRRModeInfo
float getHorizontalRefreshRate(XRRModeInfo * modeInfo){
    float out = -1;

    if(modeInfo && modeInfo->hTotal > 0 && modeInfo->dotClock > 0) // 'modeInfo' and its values must be present
        out = (float)modeInfo->dotClock / (float) modeInfo->hTotal;

    return out;
}

// Function that returns the vertical refresh rate in Hz from a XRRModeInfo
float getVerticalRefreshRate(XRRModeInfo * modeInfo){
    float out = -1;

    if(modeInfo && modeInfo->hTotal > 0 && modeInfo->vTotal > 0 && modeInfo->dotClock > 0){ // 'modeInfo' and its values must be present
        float vTotal = modeInfo->vTotal;

        if(modeInfo->modeFlags & RR_DoubleScan) // https://en.wikipedia.org/wiki/Dual_Scan
            vTotal *= 2;

        if(modeInfo->modeFlags & RR_Interlace) // https://en.wikipedia.org/wiki/Interlaced_video
            vTotal /= 2;

        out = (float)modeInfo->dotClock / ((float)modeInfo->hTotal * vTotal);
    }

    return out;
}

//Utility function for getAspectRatio
bool ratioEqualsValue(const float ratio, const float value){
    return std::abs(ratio - value) < 0.01;
}

// Function that takes the resolution OR the width/height ratio
// Returns as string the aspect ratio name if it is one of the most common ones
// Returns <ratio>:1 otherwise
QString getAspectRatio(const float width, const float height = 1){
    const float ratio = width / height;
    QString out;

    // https://en.wikipedia.org/wiki/Aspect_ratio_(image)#Some_common_examples
    if(ratioEqualsValue(ratio, 1.78f))
        out = "16:9";
    else if(ratioEqualsValue(ratio, 1.67f))
        out = "5:3";
    else if(ratioEqualsValue(ratio, 1.6f))
        out = "16:10";
    else if(ratioEqualsValue(ratio, 1.5f))
        out = "3:2";
    else if(ratioEqualsValue(ratio, 1.33f))
        out = "4:3";
    else if(ratioEqualsValue(ratio, 1.25f))
        out = "5:4";
    else if(ratioEqualsValue(ratio, 1.2f))
        out = "6:5";

    return out.isEmpty() ? QString::number(ratio, 'g', 3) + ":1" : out;
}

void addItemToTreeList(QTreeWidgetItem * parent, const QString &leftColumn, const QString &rightColumn) {
    parent->addChild(new QTreeWidgetItem(QStringList() << leftColumn << rightColumn));
}

// Returns a detailed list of connections and connected monitors
QList<QTreeWidgetItem *> gpu::getCardConnectors() const {
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

#endif // QT_DEBUG

    // New implementation that uses libXrandr to get the connectors and connected monitors list
    // Official docs: http://www.x.org/releases/current/doc/man/man3/Xrandr.3.html
    // Wiki page: http://www.x.org/wiki/libraries/libxrandr/
    // XRandr source code (useful as reference): http://cgit.freedesktop.org/xorg/app/xrandr/tree/xrandr.c
    Display* display = XOpenDisplay(NULL); // Get the connection to the X server.
    if( ! display){
        qWarning() << "Error loading connectors: can't connect to X server";
        return cardConnectorsList;
    }

    for(int screenIndex = 0; screenIndex < ScreenCount(display); screenIndex++){ // For each screen in this connection
        qDebug() << "Analyzing screen " << screenIndex;

        // Create root QTreeWidgetItem item for this screen
        QTreeWidgetItem * screenItem = new QTreeWidgetItem(QStringList() << QObject::tr("Virtual screen n°%n", NULL, screenIndex));
        cardConnectorsList.append(screenItem);

        // Add resolution
        QString screenResolution = QString::number(DisplayWidth(display,screenIndex));
        screenResolution += " x " + QString::number(DisplayHeight(display, screenIndex));
        addItemToTreeList(screenItem, QObject::tr("Resolution"), screenResolution);

        // Add screen minimum and maximum resolutions
        int screenMinWidth, screenMinHeight, screenMaxWidth, screenMaxHeight;
        Window screenRoot = RootWindow(display, screenIndex);
        XRRGetScreenSizeRange(display, screenRoot, &screenMinWidth, &screenMinHeight, &screenMaxWidth, &screenMaxHeight);

        QString screenMinResolution = QString::number(screenMinWidth) + " x " + QString::number(screenMinHeight);
        addItemToTreeList(screenItem, QObject::tr("Minimum resolution"), screenMinResolution);

        QString screenMaxResolution = QString::number(screenMaxWidth) + " x " + QString::number(screenMaxHeight);
        addItemToTreeList(screenItem, QObject::tr("Maximum resolution"), screenMaxResolution);

        // Adding screen virtual dimension
        addItemToTreeList(screenItem, QObject::tr("Virtual size"), QObject::tr("%n mm x ", NULL, DisplayWidthMM(display, screenIndex)) + QObject::tr("%n mm", NULL, DisplayHeightMM(display, screenIndex)));

        // Retrieve screen resources (connectors, configurations, timestamps etc.)
        XRRScreenResources * screenResources = XRRGetScreenResources(display, screenRoot);
        if (screenResources == NULL){
            qWarning() << "Error loading connectors: can't get resources for screen " << QString::number(screenIndex);
            continue; // Next screen
        }

        // Creating root QTreeWidgetItem for this screen's outputs
        QTreeWidgetItem * outputListItem = new QTreeWidgetItem(QStringList() << QObject::tr("Outputs"));
        screenItem->addChild(outputListItem);
        int screenConnectedOutputs = 0, screenActiveOutputs = 0;

        //Cycle through outputs of this screen
        for(int outputIndex = 0; outputIndex < screenResources->noutput; outputIndex++){
            qDebug() << "  Analyzing output " << outputIndex;

            // Get output info (connection name, current configuration, dimensions, etc)
            XRROutputInfo * outputInfo = XRRGetOutputInfo(display, screenResources, screenResources->outputs[outputIndex]);
            if(outputInfo == NULL){
                qWarning() << screenIndex << "/" << outputIndex << "Can't retrieve info for this output";
                continue; // Next output
            }

            // Creating root QTreeWidgetItem item for this output
            QTreeWidgetItem *outputItem = new QTreeWidgetItem(QStringList() << outputInfo->name);
            outputListItem->addChild(outputItem);

            // Check the output connection state
            if(outputInfo->connection != 0){ // No connection
                outputItem->setText(1, QObject::tr("Disconnected"));
                XRRFreeOutputInfo(outputInfo); // Deallocate the memory of this output's info
                continue; // Next output
            }

            screenConnectedOutputs++;

            // Get active configuration info (resolution, refresh rate, offset and brightness)
            XRRCrtcInfo * configInfo = XRRGetCrtcInfo(display, screenResources, outputInfo->crtc);
            RRMode * activeMode = NULL;

            if(configInfo == NULL) { // This output is not active
                qDebug() << "Output" << outputIndex << "has no active mode";
                addItemToTreeList(outputItem, QObject::tr("Active"), QObject::tr("No"));
            } else { // The output is active: add resolution, refresh rate and the offset
                addItemToTreeList(outputItem, QObject::tr("Active"), QObject::tr("Yes"));
                qDebug() << "    Analyzing active mode";

                screenActiveOutputs++;
                activeMode = &configInfo->mode;

                // Add current resolution
                QString outputResolution = QString::number(configInfo->width) + " x " + QString::number(configInfo->height);
                outputResolution += " (" + getAspectRatio(configInfo->width, configInfo->height) + ')';
                addItemToTreeList(outputItem, QObject::tr("Resolution"), outputResolution);

                //Add refresh rate
                float vRefreshRate = getVerticalRefreshRate(getModeInfo(screenResources, *activeMode));
                if(vRefreshRate != -1){
                    QString outputVRate = QString::number(vRefreshRate, 'g', 3) + " Hz";
                    addItemToTreeList(outputItem, QObject::tr("Refresh rate"), outputVRate);
                }

                // Add the position in the current configuration (useful only in multi-head)
                // It's the offset from the top left corner
                QString outputOffset = QString::number(configInfo->x) + ", " + QString::number(configInfo->y);
                addItemToTreeList(outputItem, QObject::tr("Offset"), outputOffset);

                // Calculate and add the screen software brightness level
                XRRCrtcGamma * gammaInfo = XRRGetCrtcGamma(display, outputInfo->crtc);
                if(gammaInfo){
                    float maxRed = gammaInfo->red[gammaInfo->size-1],
                            maxGreen = gammaInfo->green[gammaInfo->size-1],
                            maxBlue = gammaInfo->blue[gammaInfo->size-1],
                            brightnessPercent = (0.2126 * maxRed + 0.7152 * maxGreen + 0.0722 * maxBlue) * 100 / 0xFFFF;
                    // Source of the brightness formula: https://en.wikipedia.org/wiki/Relative_luminance
                    if(brightnessPercent > 0){
                        QString  softBrightness = QString::number(brightnessPercent, 'g', 3) + " %";
                        addItemToTreeList(outputItem, QObject::tr("Brightness (software)"), softBrightness);
                    }
                    XRRFreeGamma(gammaInfo);
                }
                // We could also insert here the hardware (backlight) brightness level but we need to implement it
                // This would require analyzing and parsing the content of /sys/class/backlight
                // Maybe in a future commit.
                // For more info: https://wiki.archlinux.org/index.php/backlight
                // This is an example implementation:
                // https://git.gnome.org/browse/gnome-settings-daemon/tree/plugins/power/gsd-backlight-helper.c
            }

            // Get other details (monitor size, possible configurations and properties)
            // Add monitor size
            double diagonal = sqrt(pow(outputInfo->mm_width, 2) + pow(outputInfo->mm_height, 2)) * MILLIMETERS_PER_INCH;
            addItemToTreeList(outputItem, QObject::tr("Size"), QObject::tr("%n mm x ", NULL, outputInfo->mm_width) + QObject::tr("%n mm ", NULL, outputInfo->mm_height) + QObject::tr("(%n inches)", NULL, diagonal));

            // Create the root QTreeWidgetItem of the possible modes (resolution, Refresh rate, HSync, VSync, etc) list
            QTreeWidgetItem * modeListItem = new QTreeWidgetItem(QStringList() << QObject::tr("Supported modes"));
            outputItem->addChild(modeListItem);

            for(int modeIndex = 0; modeIndex < outputInfo->nmode; modeIndex++){ // For each possible mode
                XRRModeInfo * modeInfo = getModeInfo(screenResources, outputInfo->modes[modeIndex]);
                if(modeInfo == NULL) // Mode info not found
                    continue; // Proceed to next mode

                // Get the name (the resolution) and prepare the details
                QString modeName = QString::fromLocal8Bit(modeInfo->name);
                if((activeMode != NULL) && (modeInfo->id == *activeMode))
                    modeName += " (active)"; // This is the currently active mode

                // Add the aspect ratio
                QString modeDetails = getAspectRatio(modeInfo->width, modeInfo->height) + ", ";

                // Gather mode vertical and horizontal refresh rate and clock
                // http://en.tldp.org/HOWTO/XFree86-Video-Timings-HOWTO/basic.html
                if(modeInfo->dotClock && modeInfo->vTotal && modeInfo->hTotal){ // We need those values
                    float verticalRefreshRate = getVerticalRefreshRate(modeInfo),
                            horizontalRefreshRate = getHorizontalRefreshRate(modeInfo);

                    if(verticalRefreshRate > 0)
                        modeDetails += QString::number(verticalRefreshRate, 'g', 3) + QObject::tr(" Hz vertical, ");

                    if(horizontalRefreshRate > 0)
                        modeDetails += QString::number(horizontalRefreshRate / 1000, 'g', 3) + QObject::tr(" KHz horizontal, ");

                    if(modeInfo->dotClock > 0)
                        modeDetails += QString::number((float) modeInfo->dotClock / 1000000, 'g', 3) + QObject::tr(" MHz dot clock");
                }

                // Check possible mode flags
                if(modeInfo->modeFlags & RR_DoubleScan)
                    modeDetails += ", Double scan";

                if(modeInfo->modeFlags & RR_Interlace)
                    modeDetails += ", Interlaced";

                // Check if this is the default configuration
                if(modeIndex == outputInfo->npreferred)
                    modeDetails += " (preferred)";

                addItemToTreeList(modeListItem, modeName, modeDetails);
            }

            // Create the root QTreeWidgetItem of the property list
            QTreeWidgetItem * propertyListItem = new QTreeWidgetItem(QStringList() << QObject::tr("Properties"));
            outputItem->addChild(propertyListItem);

            // Get this output properties (EDID, audio, scaling mode, etc)
            int propertyCount;
            Atom * properties = XRRListOutputProperties(display, screenResources->outputs[outputIndex], &propertyCount);

            // Cycle through this output's properties
            for(int propertyIndex = 0; propertyIndex < propertyCount; propertyIndex++){
                // Prepare this property's name and value
                char * propertyAtomName = XGetAtomName(display, properties[propertyIndex]);
                QString propertyName = QString::fromLocal8Bit(propertyAtomName);
                XFree(propertyAtomName);

                // Get the property raw value
                Atom propertyDataType;
                int propertyDataFormat;
                unsigned long propertyDataSize, propertyDataBytesAfter;
                quint8 * propertyRawData;
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
                // http://us.download.nvidia.com/XFree86/Linux-x86-ARM/361.16/README/xrandrextension.html
                if(propertyName.compare("EDID") == 0){ // EDID found
                    qDebug() << "      Property is EDID, parsing it";

                    // See http://en.wikipedia.org/wiki/Extended_display_identification_data#EDID_1.3_data_format
                    if(propertyDataSize < 128){ // EDID is invalid
                        qWarning() << "EDID is malformed, skipping";
                        continue;
                    }

                    QString readableEDID;
                    for(unsigned long i = 0; i < propertyDataSize; i++){ // For each byte in raw data
                        if(((i % 16) == 0) && (i != 0)) // Every 16 bytes (32 HEX symbols) go on new line
                            readableEDID += '\n';

                        // Convert each byte into two readable HEX symbols
                        readableEDID += QString("%1").arg(propertyRawData[i], 2, 16, QChar('0'));
                    }

                    addItemToTreeList(propertyListItem, propertyName, readableEDID);

                    // Parse the EDID to gather info
                    const quint8 header[8] = {0x00,0xff,0xff,0xff,0xff,0xff,0xff,0x00}; // Fixed EDID header
                    if (memcmp(propertyRawData, header, 8) != 0) { // If the header is not valid
                        qWarning() << screenIndex << '/' << outputInfo->name << ": can't parse EDID, invalid header";
                    } else { // Valid header
                        // Add the monitor name to the tree as value of the Output Item
                        outputItem->setText(1, QObject::tr("Connected with ") + getMonitorName(propertyRawData));

                        // Get the serial number
                        // For reference: https://github.com/KDE/libkscreen/blob/master/src/edid.cpp#L288-L295
                        quint32 serialNumber = propertyRawData[EDID_OFFSET_SERIAL_NUMBER];
                        serialNumber += propertyRawData[EDID_OFFSET_SERIAL_NUMBER + 1] * 0x100;
                        serialNumber += propertyRawData[EDID_OFFSET_SERIAL_NUMBER + 2] * 0x10000;
                        serialNumber += propertyRawData[EDID_OFFSET_SERIAL_NUMBER + 3] * 0x1000000;
                        QString serial = (serialNumber > 0) ? QString::number(serialNumber) : QObject::tr("Not available");
                        addItemToTreeList(propertyListItem, QObject::tr("Serial number"), serial);
                    }
                    //End of EDID
                } else { //Not EDID
                    // Translate the value ( translateProperty() will handle it)
                    QString propertyValue;
                    for(unsigned long i = 0; i < propertyDataSize; i++)
                        propertyValue += translateProperty(display,
                                                           propertyDataFormat,
                                                           propertyDataType,
                                                           (Atom*)&propertyRawData[i]);

                    // Get the property informations (allows to get ranges)
                    XRRPropertyInfo *propertyInfo = XRRQueryOutputProperty(display,
                                                                           screenResources->outputs[outputIndex],
                                                                           properties[propertyIndex]);
                    if(propertyInfo == NULL) {
                        qDebug() << screenIndex << '/' << outputInfo->name << ": propertyInfo is NULL";
                    } else if(propertyInfo->num_values > 0) { // A range or a list of alternatives is present
                        // Proceed to print the range or the list alternatives
                        propertyValue += (propertyInfo->range) ? " (Range: " : " (Supported: ";

                        for(int valuesIndex = 0; valuesIndex < propertyInfo->num_values; valuesIndex++){
                            // Until there is another alternative/range available
                            if(propertyInfo->range) { // This is a range, print the maximum value
                                propertyValue += translateProperty(display,
                                                            32, // Value data has 32-bit format
                                                            propertyDataType,
                                                            (Atom*)&propertyInfo->values[valuesIndex]);
                               propertyValue += " - ";
                               valuesIndex++;
                            }

                            propertyValue += translateProperty(display,
                                                               32,
                                                               propertyDataType,
                                                               (Atom*)&propertyInfo->values[valuesIndex]);

                            if(valuesIndex < propertyInfo->num_values - 1) // Another range is available
                                propertyValue.append(", ");
                        }

                        propertyValue += ')';

                        // Property alternatives completed: deallocate propertyInfo
                        free(propertyInfo);
                    }
                    // Print the property
                    addItemToTreeList(propertyListItem, propertyName, propertyValue);
                }
                // Property completed: deallocate the property raw data
                free(propertyRawData);
            }
            // Output completed: deallocate properties, configInfo and outputInfo
            XFree(properties);
            XRRFreeCrtcInfo(configInfo);
            XRRFreeOutputInfo(outputInfo);
        }
        // Screen completed: print output count (connected and active) and deallocate screenResources
        outputListItem->setText(1, QObject::tr("%n connected, ", NULL, screenConnectedOutputs) + QObject::tr("%n active", NULL, screenActiveOutputs));
        XRRFreeScreenResources(screenResources);
    }

    return cardConnectorsList;
}
