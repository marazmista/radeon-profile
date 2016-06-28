// copyright marazmista @ 29.03.2014

#include "gpu.h"

#include <cmath>
#include <QFile>
#include <QDebug>

extern "C" {
#include <X11/extensions/Xrandr.h>
}

#define EDID_OFFSET_PNP_ID    0x08
#define EDID_OFFSET_MODEL_NUMBER     0x0a
#define EDID_OFFSET_SERIAL_NUMBER    0x0c
#define EDID_OFFSET_DATA_BLOCKS  0x36
#define EDID_OFFSET_LAST_BLOCK   0x6c

#define EDID_DESCRIPTOR_PRODUCT_NAME 0xfc

#define ATOM_VALUE static_cast<Atom>(4)
#define INTEGER_VALUE static_cast<Atom>(19)
#define CARDINAL_VALUE static_cast<Atom>(6)

#define MILLIMETERS_PER_INCH 0.0393700787402

// List of files mapping PnP IDs to vendor names
// List found through pkgfile --verbose --search pnp.ids (on arch linux)
static const QStringList pnpIdFiles = {
    "/usr/share/libgnome-desktop-3.0/pnp.ids",
    "/usr/share/libgnome-desktop/pnp.ids",
    "/usr/share/libcinnamon-desktop/pnp.ids",
    "/usr/share/dispcalGUI/pnp.ids",
    "/usr/share/libmate-desktop/pnp.ids",
    "/usr/share/hwdata/pnp.ids"
};

gpu::gpu(){
    currentGpuIndex = 0;
    maxCoreFreqAvailable = minCoreFreqAvailable = false;
}

void gpu::reconfigureDaemon() {
    qWarning() << "reconfigureDaemon is not implemented";
}

bool gpu::daemonConnected() const {
    return false;
}


void gpu::convertClocks() {
    if (gpuClocksData.coreClkOk)
        gpuClocksDataString.coreClk.setNum(gpuClocksData.coreClk) += "MHz";

    if (gpuClocksData.memClkOk)
        gpuClocksDataString.memClk.setNum(gpuClocksData.memClk) += "MHz";

    if (gpuClocksData.memVoltOk)
        gpuClocksDataString.memVolt.setNum(gpuClocksData.memVolt) += "mV";

    if (gpuClocksData.coreVoltOk)
        gpuClocksDataString.coreVolt.setNum(gpuClocksData.coreVolt) += "mV";

    if (gpuClocksData.powerLevelOk)
        gpuClocksDataString.powerLevel.setNum(gpuClocksData.powerLevel);

    if (gpuClocksData.uvdCClkOk)
        gpuClocksDataString.uvdCClk.setNum(gpuClocksData.uvdCClk) += "MHz";

    if (gpuClocksData.uvdDClkOk)
        gpuClocksDataString.uvdDClk.setNum(gpuClocksData.uvdDClk) += "MHz";
}

void gpu::convertTemperature() {
    if(features.temperatureAvailable){
        gpuTemeperatureDataString.current.setNum(gpuTemeperatureData.current) += "\u00B0C";
        gpuTemeperatureDataString.max.setNum(gpuTemeperatureData.max) += QString::fromUtf8("\u00B0C");
        gpuTemeperatureDataString.min.setNum(gpuTemeperatureData.min) += QString::fromUtf8("\u00B0C");
    }

    if(features.pwmAvailable)
        gpuTemeperatureDataString.pwmSpeed.setNum(gpuTemeperatureData.pwmSpeed) += "%";
}

driverFeatures gpu::figureOutDriverFeatures(){
    qWarning() << "figureOutDriverFeatures is not implemented";
    return driverFeatures(); // All features are initialized as false
}

void gpu::changeGPU(ushort gpuIndex){
    if(gpuIndex >= gpuList.size())
        qWarning() << "Requested unexisting card " << gpuIndex;
    else {
        currentGpuIndex = gpuIndex;
        features = figureOutDriverFeatures();
    }
}

gpuClocksStruct gpu::getClocks(bool forFeatures){
    qWarning() << "getClocks is not implemented - forFeatures =" << forFeatures;

    return gpuClocksStruct(); // Empty struct
}

temp gpu::getTemperature() const {
    qWarning() << "getTemperature is not implemented";

    return 0;
}

void gpu::updateClocksData(){
    gpuClocksData = getClocks();
    convertClocks();
}

void gpu::updateTemperatureData(){
    gpuTemeperatureData.currentBefore = gpuTemeperatureData.current;

    if(features.temperatureAvailable){
        gpuTemeperatureData.current = getTemperature();

        // update rest of structure with temperature data //
        gpuTemeperatureData.sum += gpuTemeperatureData.current;
        if (std::abs(gpuTemeperatureData.min) < 0.1f) // gpuTemeperatureData.min == 0
            gpuTemeperatureData.min  = gpuTemeperatureData.current;

        gpuTemeperatureData.max = (gpuTemeperatureData.max < gpuTemeperatureData.current) ? gpuTemeperatureData.current : gpuTemeperatureData.max;
        gpuTemeperatureData.min = (gpuTemeperatureData.min > gpuTemeperatureData.current) ? gpuTemeperatureData.current : gpuTemeperatureData.min;
    }

    if(features.pwmAvailable)
        gpuTemeperatureData.pwmSpeed = getPwmSpeed();
    convertTemperature();
}

// Function that returns the human readable output of a property value
// For reference:
// http://cgit.freedesktop.org/xorg/app/xrandr/tree/xrandr.c#n2408
QString translateProperty(Display * display,
                                 const int propertyDataFormat, // 8 / 16 / 32 bit
                                 const Atom propertyDataType, // ATOM / INTEGER / CARDINAL
                                 const Atom * propertyRawData){ // Pointer to the property value data array
    QString out;

    switch(propertyDataType){
    case ATOM_VALUE: // Text value, like 'off' or 'None'
        if(propertyDataFormat == 32){ // Only 32 bit supported here
            char* string = XGetAtomName(display, *propertyRawData);
            if(string) // If it is not NULL
                out = QString(string);
            XFree(string);
        }
        break;

    case INTEGER_VALUE: // Signed numeric value
        switch(propertyDataFormat){
        case 8: out = QString::number(static_cast<qint8>(*propertyRawData));
            break;
        case 16: out = QString::number(static_cast<qint16>(*propertyRawData));
            break;
        case 32: out = QString::number(static_cast<qint32>(*propertyRawData));
        }
        break;

    case CARDINAL_VALUE: // Unsigned numeric value
        switch(propertyDataFormat){
        case 8: out = QString::number(static_cast<quint8>(*propertyRawData));
            break;
        case 16: out = QString::number(static_cast<quint16>(*propertyRawData));
            break;
        case 32: out = QString::number(static_cast<quint32>(*propertyRawData));
        }
    }
    return out.isEmpty() ? QString::number(*propertyRawData) : out; // If no match was found, return as number
}

// Get the real vendor name from the three-letter PNP ID
// See http://www.uefi.org/pnp_id_list
QString translatePnpId(const QString pnpId){
    if ( ! pnpId.isEmpty()){
        for(const QString & path : pnpIdFiles){ // Cycle through the files
            QFile pnpIds(path);
            if(pnpIds.exists() && pnpIds.open(QIODevice::ReadOnly)){ // File is available
                while ( ! pnpIds.atEnd()){ // Continue reading the file until the file has ended
                    QString line = pnpIds.readLine();
                    if (line.startsWith(pnpId)){ // Right line
                        QStringList parts = line.split(QChar('\t')); // Separate PNP ID from the real name
                        if (parts.size() == 2){
                            qDebug() << "      Found PnP ID: " << pnpId << "->" << parts.at(1).simplified();
                            pnpIds.close();
                            return parts.at(1).simplified(); // Get the real name
                        }
                    }
                }
                pnpIds.close();
            }
        }
    }
    qDebug() << "      PnP ID not found: " << pnpId;
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
            modelName = QString::fromLocal8Bit(reinterpret_cast<const char*>(&EDID[i+5]), 13).simplified();

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
        out = static_cast<float>(modeInfo->dotClock) / modeInfo->hTotal;

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

        out = static_cast<float>(modeInfo->dotClock) / (modeInfo->hTotal * vTotal);
    }

    return out;
}

//Utility function for getAspectRatio
bool ratioEqualsValue(const float ratio, const float value){
    return std::abs(ratio - value) < 0.01f;
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

    return out.isEmpty() ? QString::number(static_cast<double>(ratio), 'g', 3) + ":1" : out;
}

void addChild(QTreeWidgetItem * parent, const QString &leftColumn, const QString &rightColumn) {
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
        QTreeWidgetItem * screenItem = new QTreeWidgetItem({tr("Virtual screen %n", NULL, screenIndex)});
        cardConnectorsList.append(screenItem);

        // Add resolution
        QString screenResolution = QString::number(DisplayWidth(display,screenIndex));
        screenResolution += " x " + QString::number(DisplayHeight(display, screenIndex));
        addChild(screenItem, tr("Resolution"), screenResolution);

        // Add screen minimum and maximum resolutions
        int screenMinWidth, screenMinHeight, screenMaxWidth, screenMaxHeight;
        Window screenRoot = RootWindow(display, screenIndex);
        XRRGetScreenSizeRange(display, screenRoot, &screenMinWidth, &screenMinHeight, &screenMaxWidth, &screenMaxHeight);

        QString screenMinResolution = QString::number(screenMinWidth) + " x " + QString::number(screenMinHeight);
        addChild(screenItem, tr("Minimum resolution"), screenMinResolution);

        QString screenMaxResolution = QString::number(screenMaxWidth) + " x " + QString::number(screenMaxHeight);
        addChild(screenItem, tr("Maximum resolution"), screenMaxResolution);

        // Adding screen virtual dimension
        addChild(screenItem, tr("Virtual size"),
                 tr("%n mm x ",NULL,DisplayWidthMM(display,screenIndex))+tr("%n mm",NULL,DisplayHeightMM(display,screenIndex)));

        // Retrieve screen resources (connectors, configurations, timestamps etc.)
        XRRScreenResources * screenResources = XRRGetScreenResourcesCurrent(display, screenRoot);
        if (screenResources == NULL){
            qWarning() << "Error loading connectors: can't get resources for screen " << QString::number(screenIndex);
            continue; // Next screen
        }

        // Creating root QTreeWidgetItem for this screen's outputs
        QTreeWidgetItem * outputListItem = new QTreeWidgetItem({tr("Outputs")});
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
                outputItem->setText(1, tr("Disconnected"));
                XRRFreeOutputInfo(outputInfo); // Deallocate the memory of this output's info
                continue; // Next output
            }

            screenConnectedOutputs++;

            // Get active configuration info (resolution, refresh rate, offset and brightness)
            XRRCrtcInfo * configInfo = XRRGetCrtcInfo(display, screenResources, outputInfo->crtc);
            RRMode * activeMode = NULL;

            if(configInfo == NULL) { // This output is not active
                qDebug() << "    Output" << outputIndex << "has no active mode";
                addChild(outputItem, tr("Active"), tr("No"));
            } else { // The output is active: add resolution, refresh rate and the offset
                addChild(outputItem, tr("Active"), tr("Yes"));
                qDebug() << "    Analyzing active mode";

                screenActiveOutputs++;
                activeMode = &configInfo->mode;

                // Add current resolution
                QString outputResolution = QString::number(configInfo->width) + " x " + QString::number(configInfo->height);
                outputResolution += " (" + getAspectRatio(configInfo->width, configInfo->height) + ')';
                addChild(outputItem, tr("Resolution"), outputResolution);

                //Add refresh rate
                float vRefreshRate = getVerticalRefreshRate(getModeInfo(screenResources, *activeMode));
                if(vRefreshRate > 0){
                    QString outputVRate = QString::number(static_cast<double>(vRefreshRate), 'g', 3) + " Hz";
                    addChild(outputItem, tr("Refresh rate"), outputVRate);
                }

                // Add the position in the current configuration (useful only in multi-head)
                // It's the offset from the top left corner
                QString outputOffset = QString::number(configInfo->x) + ", " + QString::number(configInfo->y);
                addChild(outputItem, tr("Offset"), outputOffset);

                // Calculate and add the screen software brightness level
                XRRCrtcGamma * gammaInfo = XRRGetCrtcGamma(display, outputInfo->crtc);
                if(gammaInfo){
                    float maxRed = gammaInfo->red[gammaInfo->size-1],
                            maxGreen = gammaInfo->green[gammaInfo->size-1],
                            maxBlue = gammaInfo->blue[gammaInfo->size-1],
                            brightnessPercent = (0.2126f * maxRed + 0.7152f * maxGreen + 0.0722f * maxBlue) * 100 / 0xFFFF;
                    // Source of the brightness formula: https://en.wikipedia.org/wiki/Relative_luminance
                    if(brightnessPercent > 0){
                        QString  softBrightness = QString::number(static_cast<double>(brightnessPercent), 'g', 3) + " %";
                        addChild(outputItem, tr("Brightness (software)"), softBrightness);
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
            addChild(outputItem, tr("Size"),
                     QString::number(outputInfo->mm_width) + tr(" mm x ")
                     + QString::number(outputInfo->mm_height) + tr(" mm (")
                     + QString::number(diagonal) + tr(" inches)"));

            // Create the root QTreeWidgetItem of the possible modes (resolution, Refresh rate, HSync, VSync, etc) list
            QTreeWidgetItem * modeListItem = new QTreeWidgetItem({tr("Supported modes")});
            outputItem->addChild(modeListItem);

            qDebug() << "    Analyzing available modes";
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
                        modeDetails += QString::number(static_cast<double>(verticalRefreshRate), 'g', 3) + tr(" Hz vertical, ");

                    if(horizontalRefreshRate > 0)
                        modeDetails += QString::number(static_cast<double>(horizontalRefreshRate) / 1000, 'g', 3) + tr(" KHz horizontal, ");

                    if(modeInfo->dotClock > 0)
                        modeDetails += QString::number(static_cast<double>(modeInfo->dotClock) / 1000000, 'g', 3) + tr(" MHz dot clock");
                }

                // Check possible mode flags
                if(modeInfo->modeFlags & RR_DoubleScan)
                    modeDetails += ", Double scan";

                if(modeInfo->modeFlags & RR_Interlace)
                    modeDetails += ", Interlaced";

                // Check if this is the default configuration
                if(modeIndex == outputInfo->npreferred)
                    modeDetails += " (preferred)";

                addChild(modeListItem, modeName, modeDetails);
            }

            // Create the root QTreeWidgetItem of the property list
            QTreeWidgetItem * propertyListItem = new QTreeWidgetItem({tr("Properties")});
            outputItem->addChild(propertyListItem);

            // Get this output properties (EDID, audio, scaling mode, etc)
            int propertyCount;
            Atom * properties = XRRListOutputProperties(display, screenResources->outputs[outputIndex], &propertyCount);

            // Cycle through this output's properties
            qDebug() << "    Analyzing properties";
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

                    addChild(propertyListItem, propertyName, readableEDID);

                    // Parse the EDID to gather info
                    const quint8 header[8] = {0x00,0xff,0xff,0xff,0xff,0xff,0xff,0x00}; // Fixed EDID header
                    if (memcmp(propertyRawData, header, 8) != 0) { // If the header is not valid
                        qWarning() << screenIndex << '/' << outputInfo->name << ": can't parse EDID, invalid header";
                    } else { // Valid header
                        // Add the monitor name to the tree as value of the Output Item
                        outputItem->setText(1, tr("Connected with ") + getMonitorName(propertyRawData));

                        // Get the serial number
                        // For reference: https://github.com/KDE/libkscreen/blob/master/src/edid.cpp#L288-L295
                        quint32 serialNumber = propertyRawData[EDID_OFFSET_SERIAL_NUMBER];
                        serialNumber += propertyRawData[EDID_OFFSET_SERIAL_NUMBER + 1] * 0x100;
                        serialNumber += propertyRawData[EDID_OFFSET_SERIAL_NUMBER + 2] * 0x10000;
                        serialNumber += propertyRawData[EDID_OFFSET_SERIAL_NUMBER + 3] * 0x1000000;
                        QString serial = (serialNumber > 0) ? QString::number(serialNumber) : tr("Not available");
                        addChild(propertyListItem, tr("Serial number"), serial);
                    }
                    //End of EDID
                } else { //Not EDID
                    // Translate the value ( translateProperty() will handle it)
                    QString propertyValue;
                    for(unsigned long i = 0; i < propertyDataSize; i++)
                        propertyValue += translateProperty(display,
                                                           propertyDataFormat,
                                                           propertyDataType,
                                                           reinterpret_cast<Atom*>(&propertyRawData[i]));

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
                                                            reinterpret_cast<Atom*>(&propertyInfo->values[valuesIndex]));
                               propertyValue += " - ";
                               valuesIndex++;
                            }

                            propertyValue += translateProperty(display,
                                                               32,
                                                               propertyDataType,
                                                               reinterpret_cast<Atom*>(&propertyInfo->values[valuesIndex]));

                            if(valuesIndex < propertyInfo->num_values - 1) // Another range is available
                                propertyValue.append(", ");
                        }

                        propertyValue += ')';

                        // Property alternatives completed: deallocate propertyInfo
                        free(propertyInfo);
                    }
                    // Print the property
                    addChild(propertyListItem, propertyName, propertyValue);
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
        outputListItem->setText(1,tr("%n connected, ",NULL,screenConnectedOutputs)+tr("%n active",NULL,screenActiveOutputs));
        XRRFreeScreenResources(screenResources);
    }

    return cardConnectorsList;
}

QList<QTreeWidgetItem *> gpu::getModuleInfo() const {
    qDebug() << "Getting module info";
    QList<QTreeWidgetItem *> list;

    if(driverModule.isEmpty())
        qWarning() << "getModuleInfo: no module available";
    else {
        // Get the list of the current module parameters
        QStringList modInfo = globalStuff::grabSystemInfo("modinfo -p " + driverModule);
        modInfo.sort();
        const QString parameterPath = "/sys/module/" + driverModule + "/parameters/";

        for (const QString & line : modInfo) { // For each parameter
            if (line.contains(":") && ! line.startsWith("modinfo: ERROR: ")) { // show nothing in case of an error
               const QStringList parts = line.split(":",QString::SkipEmptyParts);
               const QString paramName = parts[0], paramDesc = parts[1];

               // Try reading the value though "systool -m <module> -A <parameter>
               const QStringList info = globalStuff::grabSystemInfo("systool -m " + driverModule + " -A " + paramName).filter(paramName);

               QString paramValue;
               if( ! info.isEmpty()) // Systool available, parse parameter line: <parameter> = "<value>"
                   paramValue = info[0].split("=", QString::SkipEmptyParts).value(1).remove('"');
               else { // Systool not available, read parameter value from /sys/module/<module>/parameters/<parameter>
                    QFile mp(parameterPath + paramName);
                    if(mp.open(QIODevice::ReadOnly)){
                        paramValue = mp.readLine(20).simplified();
                        mp.close();
                    } else
                        paramValue = tr("Unknown");
               }
               list.append(new QTreeWidgetItem({paramName, paramValue, paramDesc}));
            }
        }
    }

    return list;
}

QStringList gpu::getGLXInfo(const QString & gpuName) const {
    qDebug() << "Listing installed cards";
    QStringList data, gpus = globalStuff::grabSystemInfo("lspci").filter(QRegExp(".+VGA.+|.+3D.+"));
    gpus.removeAt(gpus.indexOf(QRegExp(".+Audio.+"))); //remove radeon audio device
    // loop for multi gpu
    for (int i = 0; i < gpus.count(); i++)
        data << tr("VGA:") +gpus[i].split(":",QString::SkipEmptyParts)[2];

    QRegExp noEmptyLines = QRegExp(".+");

    qDebug() << "Getting X DRI driver info";
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!gpuName.isEmpty())
        env.insert("DRI_PRIME",gpuName.at(gpuName.length()-1));
    const QStringList driver = globalStuff::grabSystemInfo("xdriinfo driver 0").filter(noEmptyLines);
    if(!driver.isEmpty())
        data << tr("Driver: ") + driver[0];
    // Old implementation
    // QStringList driver = globalStuff::grabSystemInfo("xdriinfo", env).filter("Screen 0:",Qt::CaseInsensitive);
    // if (!driver.isEmpty())  // because of segfault when no xdriinfo
    //    data << "Driver:"+ driver.filter("Screen 0:",Qt::CaseInsensitive)[0].split(":",QString::SkipEmptyParts)[1];

    qDebug() << "Getting OpenGL info";
    const QStringList glxinfo = globalStuff::grabSystemInfo("glxinfo -B", env).filter(noEmptyLines);
    if( ! glxinfo.isEmpty()){
        data.append("");
        data.append(tr("OpenGL info"));
        data.append(glxinfo);
    }
    // Old dxorg.cpp implementation, using full glxinfo output instead of glxinfo -B
    // Consider only lines containing "direct rendering" or containing "OpenGL ... : ..." but not containing 'none
    // data.append(globalStuff::grabSystemInfo("glxinfo", env).filter(QRegExp("direct rendering|OpenGL.+:..(?!none)")));

    qDebug() << "Getting OpenCL info";
    const QStringList clinfo = globalStuff::grabSystemInfo("clinfo --human").filter(noEmptyLines);
    if( ! clinfo.isEmpty()){
        data.append("");
        data.append(tr("OpenCL info"));
        data.append(clinfo);
    }

    qDebug() << "Getting Vulkan info";
    // http://www.phoronix.com/scan.php?page=news_item&px=Vulkan-1.0-SKL-First-Test
    const QStringList vulkaninfo = globalStuff::grabSystemInfo("vulkaninfo").filter(QRegExp(".+= \\w"));
    if( ! vulkaninfo.isEmpty()){
        data.append("");
        data.append(tr("Vulkan info"));
        for(const QString & line : vulkaninfo)
            data.append(line.trimmed());
    }


    return data;
}

QString gpu::getCurrentPowerLevel() const {
    qWarning() << "getCurrentPowerLevel is not implemented";

    return "";
}

QString gpu::getCurrentPowerProfile() const {
    qWarning() << "getCurrentPowerProfile is not implemented";

    return "";
}

void gpu::refreshPowerLevel() {
    currentPowerLevel = getCurrentPowerLevel();
    currentPowerProfile = getCurrentPowerProfile();
}

void gpu::setPowerProfile(powerProfiles newPowerProfile) {
    qWarning() << "setPowerProfile is not implemented - newPowerProfile" << newPowerProfile;
}

void gpu::setForcePowerLevel(forcePowerLevels newForcePowerLevel) {
    qWarning() << "setForcePowerLevel is not implemented - newForcePowerLevel =" << newForcePowerLevel;
}

void gpu::setPwmValue(ushort value) {
    qWarning() << "setForcePowerLevel is not implemented - value =" << value;
}

void gpu::setPwmManualControl(bool manual) {
    qWarning() << "setPwmManualControl is not implemented - manual =" << manual;
}

ushort gpu::getPwmSpeed() const {
    qWarning() << "getPwmSpeed is not implemented";

    return 0;
}

bool gpu::overclockGPU(const int value){
    qWarning() << "Error overclocking GPU: overclocking is not implemented - value =" << value;

    return false;
}

bool gpu::overclockMemory(const int value){
    qWarning() << "Error overclocking memory: overclocking is not implemented - value=" << value;

    return false;
}

QStringList gpu::detectCards() const {
    const QStringList list = globalStuff::grabSystemInfo("ls /sys/class/drm/").filter("card");
    QStringList cards;

    for(const QString & element : list){
        QFile f("/sys/class/drm/" + element + "/device/uevent");
        if (f.open(QIODevice::ReadOnly)) {
            if (QString(f.readLine(50)).contains("DRIVER=" + driverModule))
                cards.append(f.fileName().split('/').at(4));
        }
    }

    return cards;
}

bool gpu::setNewValue(const QString & filePath, const QString & newValue) const {
    if(newValue.isEmpty()){
        qWarning() << "setNewValue: newValue is empty";
        return false;
    }

    if( ! QFile::exists(filePath)){
        qWarning() << "setNewValue: invalid file path";
        return false;
    }

    QFile file(filePath);
    if( ! file.open(QIODevice::WriteOnly | QIODevice::Text)){
        qWarning() << "Unable to open " + filePath + " to write " + newValue;
        return false;
    }

    qDebug() << "Writing" << newValue << "into" << filePath;
    QTextStream stream(&file);
    stream << newValue + "\n";
    if( ! file.flush() ){
        qWarning() << "Failed to flush " + filePath + " to write " + newValue;
        return false;
    }

    file.close();

    return true;
}

QString gpu::readFile(const QString & filePath) const {
    QFile file(filePath);

    if( ! file.open(QIODevice::ReadOnly | QIODevice::Text)){
        qDebug() << "Failed reading" << filePath;
        return "";
    }

    const QString out = file.readAll().trimmed();

    file.close();
    return out;
}

bool gpu::updateClocksDataIsAvailable() const {
    return features.coreClockAvailable
            || features.memClockAvailable
            || features.coreVoltAvailable
            || features.memVoltAvailable;
}
