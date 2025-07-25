#include "influxdbclient.h"
#include <iostream>
#include <algorithm>
#include <limits>

InfluxDBClient::InfluxDBClient(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_timer(new QTimer(this))
    , m_speed(0.0)
    , m_charge(0.0)
    , m_autonomyLevel(0.0)
    , m_inTrip(false)
    , m_nextTripId(0)
{
    // InfluxDB Cloud configuration - using your actual credentials
    m_url = "https://eu-central-1-1.aws.cloud2.influxdata.com";
    m_token = "rYtXXREgOrb0Kd5DSkA4b--qI9AC1gHvIGfNK90Ne0yGHsIDAYkvyxKzgxDLonwTVhzclF8ZZoVk7R9atXeHbQ==";
    m_org = "jetracer";
    m_bucket = "jetracer";
    
    // Setup timer for periodic trip analysis
    m_timer->setInterval(300000); // Analyze every 30 seconds
    connect(m_timer, &QTimer::timeout, this, &InfluxDBClient::onTimerTimeout);
    
    // Initialize database
    if (!initializeDatabase()) {
        qDebug() << "Warning: Failed to initialize trip database";
    }
    
    qDebug() << "InfluxDBClient: Initialized with URL:" << m_url;
    qDebug() << "InfluxDBClient: Using org:" << m_org;
    qDebug() << "InfluxDBClient: Using bucket:" << m_bucket;
}

void InfluxDBClient::startDataCollection()
{
    qDebug() << "InfluxDBClient: Starting trip analysis system...";
    m_timer->start();
    // Fetch initial data immediately for trip analysis
    fetchLatestData();
}

void InfluxDBClient::stopDataCollection()
{
    qDebug() << "InfluxDBClient: Stopping data collection...";
    m_timer->stop();
}

void InfluxDBClient::fetchLatestData()
{
    qDebug() << "InfluxDBClient: Fetching latest data from InfluxDB...";
    makeInfluxDBRequest();
}

void InfluxDBClient::onTimerTimeout()
{
    fetchLatestData();
}

void InfluxDBClient::makeInfluxDBRequest()
{
    // Flux query to get both vehicle speed and battery charge data for comprehensive trip analysis
    QString fluxQuery = QString(
        "from(bucket: \"%1\")"
        " |> range(start: -10d)"
        " |> filter(fn: (r) => r[\"_measurement\"] == \"Vehicle/1/qt/speed\" or r[\"_measurement\"] == \"Vehicle/1/qt/charge\")"
        " |> sort(columns: [\"_time\"])"  // Sort by time to ensure chronological order
        " |> yield(name: \"vehicle_data\")"
    ).arg(m_bucket);

    qDebug() << "InfluxDBClient: Fetching Vehicle/1/qt/speed and Vehicle/1/qt/charge data for trip analysis (last 24h):" << fluxQuery;

    QNetworkRequest request;
    request.setUrl(QUrl(m_url + "/api/v2/query?org=" + m_org));
    request.setRawHeader("Authorization", QString("Token %1").arg(m_token).toUtf8());
    request.setRawHeader("Content-Type", "application/vnd.flux");
    request.setRawHeader("Accept", "application/csv");

    QNetworkReply *reply = m_networkManager->post(request, fluxQuery.toUtf8());
    
    connect(reply, &QNetworkReply::finished, this, &InfluxDBClient::onDataReceived);
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
            this, &InfluxDBClient::onNetworkError);
}

void InfluxDBClient::onDataReceived()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    QByteArray data = reply->readAll();
    qDebug() << "InfluxDBClient: Received" << data.size() << "bytes";
    qDebug() << "InfluxDBClient: Full response data:" << QString(data);
    
    if (reply->error() == QNetworkReply::NoError) {
        // Check HTTP status code
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << "InfluxDBClient: HTTP status code:" << statusCode;
        
        parseInfluxDBResponse(data);
    } else {
        qDebug() << "InfluxDBClient: HTTP Error:" << reply->errorString();
    }

    reply->deleteLater();
}

void InfluxDBClient::onNetworkError(QNetworkReply::NetworkError error)
{
    qDebug() << "InfluxDBClient: Network error occurred:" << error;
}

void InfluxDBClient::parseInfluxDBResponse(const QByteArray &data)
{
    QString csvData = QString::fromUtf8(data);
    QStringList lines = csvData.split('\n', Qt::SkipEmptyParts);
    
    qDebug() << "=== TRIP ANALYSIS - PROCESSING DATA ===";
    qDebug() << "Total lines received:" << lines.size();
    
    if (lines.size() < 2) {
        qDebug() << "No vehicle data found for trip analysis";
        return;
    }
    
    // Parse CSV header
    QStringList headers = lines[0].split(',');
    
    // Trim whitespace and carriage returns from headers
    for (int i = 0; i < headers.size(); ++i) {
        headers[i] = headers[i].trimmed();
    }
    
    // Print headers for debugging
    qDebug() << "CSV Headers found:";
    for (int i = 0; i < headers.size(); ++i) {
        qDebug() << "  Index" << i << ":" << headers[i];
    }
    
    // Find the indices we need
    int valueIndex = headers.indexOf("_value");
    int timeIndex = headers.indexOf("_time");
    int measurementIndex = headers.indexOf("_measurement");
    
    if (valueIndex == -1 || timeIndex == -1 || measurementIndex == -1) {
        qDebug() << "Required _value, _time, or _measurement columns not found in CSV";
        qDebug() << "valueIndex:" << valueIndex << "timeIndex:" << timeIndex << "measurementIndex:" << measurementIndex;
        return;
    }
    
    // Clear existing buffer
    m_dataBuffer.clear();
    
    // Separate parsing: Speed and battery measurements have different timestamps
    QMap<QDateTime, double> speedData;
    QMap<QDateTime, double> batteryData;
    
    // First pass: Parse all data and separate speed/battery by timestamp
    for (int i = 1; i < lines.size(); i++) {
        QStringList fields = lines[i].split(',');
        if (fields.size() <= qMax(qMax(valueIndex, timeIndex), measurementIndex)) continue;
        
        QString valueStr = fields[valueIndex];
        QString timeStr = fields[timeIndex];
        QString measurement = fields[measurementIndex].trimmed(); // Remove carriage returns
        
        bool ok;
        double value = valueStr.toDouble(&ok);
        
        if (ok) {
            // Parse timestamp
            QDateTime timestamp = QDateTime::fromString(timeStr, Qt::ISODate);
            if (timestamp.isValid()) {
                // Debug: Show first few raw measurements
                if (speedData.size() + batteryData.size() < 5) {
                    qDebug() << "Raw measurement:" << measurement << "Value:" << value << "Time:" << timeStr;
                }
                
                // Store values separately by measurement type
                if (measurement == "Vehicle/1/qt/speed") {
                    speedData[timestamp] = value;
                    if (speedData.size() <= 3) {
                        qDebug() << "Speed data:" << timestamp.toString() << "=" << value;
                    }
                } else if (measurement == "Vehicle/1/qt/charge") {
                    batteryData[timestamp] = value;
                    if (batteryData.size() <= 3) {
                        qDebug() << "Battery data:" << timestamp.toString() << "=" << value;
                    }
                }
            } else {
                // Debug: Show invalid timestamps
                if (speedData.size() + batteryData.size() < 3) {
                    qDebug() << "Invalid timestamp:" << timeStr << "for measurement:" << measurement;
                }
            }
        } else {
            // Debug: Show invalid values
            if (speedData.size() + batteryData.size() < 3) {
                qDebug() << "Invalid value:" << valueStr << "for measurement:" << measurement;
            }
        }
    }
    
    qDebug() << "Parsed" << speedData.size() << "speed points and" << batteryData.size() << "battery points";
    
    // Second pass: Create combined timeline with interpolated values
    QSet<QDateTime> allTimestamps;
    for (auto it = speedData.begin(); it != speedData.end(); ++it) {
        allTimestamps.insert(it.key());
    }
    for (auto it = batteryData.begin(); it != batteryData.end(); ++it) {
        allTimestamps.insert(it.key());
    }
    
    // Convert to list and sort chronologically
    QList<QDateTime> sortedTimestamps = allTimestamps.values();
    std::sort(sortedTimestamps.begin(), sortedTimestamps.end());
    
    // Third pass: Create data points with interpolated values
    double lastKnownSpeed = 0.0;
    double lastKnownBattery = 0.0;
    
    for (const QDateTime& timestamp : sortedTimestamps) {
        double currentSpeed = lastKnownSpeed;  // Default to last known
        double currentBattery = lastKnownBattery;  // Default to last known
        
        // Use actual values if available at this timestamp
        if (speedData.contains(timestamp)) {
            currentSpeed = speedData[timestamp];
            lastKnownSpeed = currentSpeed;
        }
        if (batteryData.contains(timestamp)) {
            currentBattery = batteryData[timestamp];
            lastKnownBattery = currentBattery;
        }
        
        // Add the data point with both values
        addDataPoint(timestamp, currentSpeed, currentBattery);
    }
    
    qDebug() << "Processed" << m_dataBuffer.size() << "data points for trip analysis";
    
    // Debug: Show first and last timestamps to confirm chronological order
    if (!m_dataBuffer.isEmpty()) {
        qDebug() << "Chronological order verification:";
        qDebug() << "Oldest timestamp:" << m_dataBuffer.first().timestamp.toString() 
                 << "Speed:" << m_dataBuffer.first().speed 
                 << "Battery:" << m_dataBuffer.first().batteryCharge << "%";
        qDebug() << "Newest timestamp:" << m_dataBuffer.last().timestamp.toString() 
                 << "Speed:" << m_dataBuffer.last().speed 
                 << "Battery:" << m_dataBuffer.last().batteryCharge << "%";
    }
    
    // Clear previous trip state for fresh analysis
    m_detectedTrips.clear();
    m_inTrip = false;
    m_nextTripId = 1;
    m_potentialTripStart = QDateTime();
    m_potentialTripEnd = QDateTime();
    
    // Analyze for trips
    analyzeForTrips();
    
    // Print current trip summary
    printTripSummary();
}

void InfluxDBClient::addDataPoint(const QDateTime& timestamp, double speed, double batteryCharge)
{
    m_dataBuffer.append(VehicleDataPoint(timestamp, speed, batteryCharge));
    m_speed = speed; // Update current speed
    m_charge = batteryCharge; // Update current battery charge
}

void InfluxDBClient::analyzeForTrips()
{
    if (m_dataBuffer.size() < 2) return;
    
    qDebug() << "=== ANALYZING FOR TRIPS ===";
    
    // Check for trip timeout first (show debug info always)
    if (!m_dataBuffer.isEmpty()) {
        QDateTime currentTime = QDateTime::currentDateTime();
        QDateTime lastDataTime = m_dataBuffer.last().timestamp;
        qint64 timeSinceLastData = lastDataTime.secsTo(currentTime);
        
        qDebug() << "=== TIMEOUT CHECK ===";
        qDebug() << "Current time:" << currentTime.toString();
        qDebug() << "Last data time:" << lastDataTime.toString();
        qDebug() << "Time since last data:" << timeSinceLastData << "seconds (" << (timeSinceLastData/60.0) << "minutes)";
        qDebug() << "Timeout threshold:" << TRIP_TIMEOUT_SECONDS << "seconds (" << (TRIP_TIMEOUT_SECONDS/60.0) << "minutes)";
        qDebug() << "Currently in trip:" << m_inTrip;
        
        if (timeSinceLastData >= TRIP_TIMEOUT_SECONDS) {
            // Check if we should force a trip end due to timeout
            bool shouldForceEnd = false;
            
            if (m_inTrip) {
                // Currently in trip - definitely timeout
                shouldForceEnd = true;
                qDebug() << "Timeout condition: Currently in trip, forcing end";
            } else if (!m_detectedTrips.isEmpty()) {
                const TripInfo& lastTrip = m_detectedTrips.last();
                if (lastTrip.endTime.isNull()) {
                    // Last trip has no end time - should be ongoing
                    shouldForceEnd = true;
                    qDebug() << "Timeout condition: Last trip has no end time, forcing end";
                }
            }
            
            if (shouldForceEnd) {
                // Force trip end due to timeout
                if (!m_detectedTrips.isEmpty()) {
                    TripInfo& currentTrip = m_detectedTrips.last();
                    currentTrip.endTime = lastDataTime; // Use last data time as trip end
                    currentTrip.calculateStatistics();
                    
                    qDebug() << "*** TRIP" << currentTrip.tripId << "FORCE ENDED due to 5-minute timeout ***";
                    qDebug() << "Last data received at:" << lastDataTime.toString();
                    qDebug() << "Trip duration:" << currentTrip.getFormattedDuration();
                    qDebug() << "Max speed:" << currentTrip.maxSpeed << "m/s";
                    qDebug() << "Average speed:" << currentTrip.averageSpeed << "m/s";
                    qDebug() << "Distance traveled:" << currentTrip.getFormattedDistance();
                    
                    // Save trip to database
                    saveTripToDatabase(currentTrip);
                    
                    emit tripEnded(currentTrip.tripId, currentTrip.endTime, currentTrip.maxSpeed, 
                                 currentTrip.averageSpeed, currentTrip.distanceTraveled, currentTrip.durationSeconds);
                    emit tripsUpdated(); // Notify QML of new trip
                }
                
                m_inTrip = false;
                m_potentialTripEnd = QDateTime();
                return; // Exit early since we've ended the trip
            }
        }
    }
    
    qDebug() << "Processing data from oldest to newest:";
    
    for (int i = 0; i < m_dataBuffer.size(); i++) {
        const VehicleDataPoint& point = m_dataBuffer[i];
        
        // Debug: Show processing order for first few and last few points
        if (i < 3 || i >= m_dataBuffer.size() - 3) {
            qDebug() << "Processing point" << i << ":" << point.timestamp.toString() 
                     << "Speed:" << point.speed << "Battery:" << point.batteryCharge << "%";
        }
        
        if (!m_inTrip) {
            // Looking for trip start
            if (isMoving(point.speed)) {
                if (m_potentialTripStart.isNull()) {
                    m_potentialTripStart = point.timestamp;
                    qDebug() << "Potential trip start detected at:" << m_potentialTripStart.toString();
                } else {
                    // Check if we've been moving for at least 1 minute
                    qint64 movingDuration = m_potentialTripStart.secsTo(point.timestamp);
                    if (movingDuration >= TRIP_START_DURATION_SECONDS) {
                        // Trip confirmed!
                        m_inTrip = true;
                        
                        TripInfo newTrip;
                        newTrip.tripId = m_nextTripId++;
                        newTrip.startTime = m_potentialTripStart;
                        
                        // Find battery charge at trip start time
                        newTrip.startBatteryCharge = getBatteryChargeAtTime(m_potentialTripStart);
                        
                        m_detectedTrips.append(newTrip);
                        
                        qDebug() << "*** TRIP" << newTrip.tripId << "STARTED at" << newTrip.startTime.toString() << "***";
                        qDebug() << "Driver:" << newTrip.driverName;
                        qDebug() << "Notes:" << newTrip.notes;
                        qDebug() << "Start battery charge:" << newTrip.startBatteryCharge << "%";
                        emit tripStarted(newTrip.tripId, newTrip.startTime);
                        
                        m_potentialTripStart = QDateTime();
                    }
                }
                m_lastMovementTime = point.timestamp;
            } else {
                // Reset potential trip start if speed goes to zero
                m_potentialTripStart = QDateTime();
            }
        } else {
            // We're in a trip - add data points and check for trip end
            if (!m_detectedTrips.isEmpty()) {
                m_detectedTrips.last().dataPoints.append(point);
            }
            
            if (isMoving(point.speed)) {
                m_lastMovementTime = point.timestamp;
                m_potentialTripEnd = QDateTime(); // Reset potential end
            } else {
                // Vehicle stopped
                if (m_potentialTripEnd.isNull()) {
                    m_potentialTripEnd = point.timestamp;
                } else {
                    // Check if stopped for more than 3 minutes
                    qint64 stoppedDuration = m_potentialTripEnd.secsTo(point.timestamp);
                    if (stoppedDuration >= TRIP_END_DURATION_SECONDS) {
                        // Trip ended!
                        if (!m_detectedTrips.isEmpty()) {
                            TripInfo& currentTrip = m_detectedTrips.last();
                            currentTrip.endTime = m_potentialTripEnd;
                            
                            // Find battery charge at trip end time
                            currentTrip.endBatteryCharge = getBatteryChargeAtTime(m_potentialTripEnd);
                            
                            currentTrip.calculateStatistics();
                            
                            qDebug() << "*** TRIP" << currentTrip.tripId << "ENDED at" << currentTrip.endTime.toString() << "***";
                            qDebug() << "Driver:" << currentTrip.driverName;
                            qDebug() << "Notes:" << currentTrip.notes;
                            qDebug() << "Images:" << currentTrip.imagePaths;
                            qDebug() << "End battery charge:" << currentTrip.endBatteryCharge << "%";
                            qDebug() << "Battery used:" << currentTrip.batteryUsedPercent << "%";
                            qDebug() << "Energy consumed:" << currentTrip.energyConsumedWh << "Wh";
                            qDebug() << "Energy efficiency:" << currentTrip.energyEfficiencyWhKm << "Wh/km";
                            qDebug() << "Trip duration:" << currentTrip.getFormattedDuration();
                            qDebug() << "Max speed:" << currentTrip.maxSpeed << "m/s";
                            qDebug() << "Average speed:" << currentTrip.averageSpeed << "m/s";
                            qDebug() << "Distance traveled:" << currentTrip.getFormattedDistance();
                            
                            // Save trip to database
                            saveTripToDatabase(currentTrip);
                            
                            emit tripEnded(currentTrip.tripId, currentTrip.endTime, currentTrip.maxSpeed, 
                                         currentTrip.averageSpeed, currentTrip.distanceTraveled, currentTrip.durationSeconds);
                            emit tripsUpdated(); // Notify QML of new trip
                        }
                        
                        m_inTrip = false;
                        m_potentialTripEnd = QDateTime();
                    }
                }
            }
        }
    }
    
    // === POST-PROCESSING TIMEOUT CHECK ===
    // Check if we need to force-end a trip due to timeout after processing all data
    if (!m_dataBuffer.isEmpty()) {
        QDateTime currentTime = QDateTime::currentDateTime();
        QDateTime lastDataTime = m_dataBuffer.last().timestamp;
        qint64 timeSinceLastData = lastDataTime.secsTo(currentTime);
        
        qDebug() << "\n=== POST-PROCESSING TIMEOUT CHECK ===";
        qDebug() << "Current time:" << currentTime.toString();
        qDebug() << "Last data time:" << lastDataTime.toString();
        qDebug() << "Time since last data:" << timeSinceLastData << "seconds (" << (timeSinceLastData / 60.0) << "minutes)";
        qDebug() << "Timeout threshold:" << TRIP_TIMEOUT_SECONDS << "seconds (" << (TRIP_TIMEOUT_SECONDS / 60.0) << "minutes)";
        qDebug() << "Currently in trip:" << m_inTrip;
        
        bool hasOngoingTrip = m_inTrip || (!m_detectedTrips.isEmpty() && m_detectedTrips.last().endTime.isNull());
        qDebug() << "Has ongoing trip:" << hasOngoingTrip;
        
        bool shouldForceEnd = timeSinceLastData >= TRIP_TIMEOUT_SECONDS && hasOngoingTrip;
        qDebug() << "Should force end trip:" << shouldForceEnd;
        
        if (shouldForceEnd) {
            qDebug() << "\n🕐 FORCING TRIP END DUE TO TIMEOUT";
            
            if (!m_detectedTrips.isEmpty()) {
                TripInfo& currentTrip = m_detectedTrips.last();
                if (currentTrip.endTime.isNull()) {
                    // End the trip at the time of the last data point
                    currentTrip.endTime = lastDataTime;
                    
                    // Find battery charge at trip end time
                    currentTrip.endBatteryCharge = getBatteryChargeAtTime(lastDataTime);
                    
                    currentTrip.calculateStatistics();
                    
                    qDebug() << "*** TRIP" << currentTrip.tripId << "FORCE-ENDED at" << currentTrip.endTime.toString() << "(TIMEOUT) ***";
                    qDebug() << "Driver:" << currentTrip.driverName;
                    qDebug() << "Notes:" << currentTrip.notes;
                    qDebug() << "Images:" << currentTrip.imagePaths;
                    qDebug() << "End battery charge:" << currentTrip.endBatteryCharge << "%";
                    qDebug() << "Battery used:" << currentTrip.batteryUsedPercent << "%";
                    qDebug() << "Energy consumed:" << currentTrip.energyConsumedWh << "Wh";
                    qDebug() << "Energy efficiency:" << currentTrip.energyEfficiencyWhKm << "Wh/km";
                    qDebug() << "Trip duration:" << currentTrip.getFormattedDuration();
                    qDebug() << "Max speed:" << currentTrip.maxSpeed << "m/s";
                    qDebug() << "Average speed:" << currentTrip.averageSpeed << "m/s";
                    qDebug() << "Distance traveled:" << currentTrip.getFormattedDistance();
                    
                    // Save trip to database
                    saveTripToDatabase(currentTrip);
                    
                    emit tripEnded(currentTrip.tripId, currentTrip.endTime, currentTrip.maxSpeed, 
                                 currentTrip.averageSpeed, currentTrip.distanceTraveled, currentTrip.durationSeconds);
                    emit tripsUpdated(); // Notify QML of new trip
                }
            }
            
            m_inTrip = false;
            m_potentialTripEnd = QDateTime();
            m_potentialTripStart = QDateTime();
        }
    }
}

void InfluxDBClient::printTripSummary()
{
    std::cout << "\n=== TRIP DETECTION SUMMARY ===" << std::endl;
    std::cout << "Total trips detected: " << m_detectedTrips.size() << std::endl;
    std::cout << "Currently in trip: " << (m_inTrip ? "YES" : "NO") << std::endl;
    std::cout << "Data points analyzed: " << m_dataBuffer.size() << std::endl;
    
    for (const TripInfo& trip : m_detectedTrips) {
        std::cout << "\n--- TRIP " << trip.tripId << " ---" << std::endl;
        std::cout << "Start: " << trip.getFormattedStartTime().toStdString() << std::endl;
        if (!trip.endTime.isNull()) {
            std::cout << "End: " << trip.getFormattedEndTime().toStdString() << std::endl;
            std::cout << "Duration: " << trip.getFormattedDuration().toStdString() << std::endl;
            std::cout << "Max Speed: " << trip.maxSpeed << " m/s (" << (trip.maxSpeed * 3.6) << " km/h)" << std::endl;
            std::cout << "Avg Speed: " << trip.averageSpeed << " m/s (" << (trip.averageSpeed * 3.6) << " km/h)" << std::endl;
            std::cout << "Distance: " << trip.getFormattedDistance().toStdString() << std::endl;
            std::cout << "Battery Usage: " << trip.getFormattedBatteryUsage().toStdString() << std::endl;
            std::cout << "Energy Consumed: " << trip.getFormattedEnergyConsumption().toStdString() << std::endl;
            std::cout << "Data Points: " << trip.dataPoints.size() << std::endl;
        } else {
            std::cout << "Status: ONGOING" << std::endl;
        }
    }
    std::cout << "=============================" << std::endl;
}

void InfluxDBClient::printDetailedTripInfo()
{
    std::cout << "\n=== DETAILED TRIP ANALYSIS ===" << std::endl;
    
    if (m_detectedTrips.isEmpty()) {
        std::cout << "No trips detected yet." << std::endl;
        std::cout << "===============================" << std::endl;
        return;
    }
    
    for (const TripInfo& trip : m_detectedTrips) {
        std::cout << "\n🚗 TRIP " << trip.tripId << " DETAILS:" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        
        std::cout << "📅 Start Date & Time:    " << trip.getFormattedStartTime().toStdString() << std::endl;
        
        if (!trip.endTime.isNull()) {
            std::cout << "📅 End Date & Time:      " << trip.getFormattedEndTime().toStdString() << std::endl;
            std::cout << "⏱️  Trip Duration:        " << trip.getFormattedDuration().toStdString() << std::endl;
            std::cout << "🏃 Average Speed:        " << QString::number(trip.averageSpeed, 'f', 2).toStdString() 
                      << " m/s (" << QString::number(trip.averageSpeed * 3.6, 'f', 2).toStdString() << " km/h)" << std::endl;
            std::cout << "🚀 Maximum Speed:        " << QString::number(trip.maxSpeed, 'f', 2).toStdString() 
                      << " m/s (" << QString::number(trip.maxSpeed * 3.6, 'f', 2).toStdString() << " km/h)" << std::endl;
            std::cout << "📏 Distance Traveled:    " << trip.getFormattedDistance().toStdString() << std::endl;
            std::cout << "� Battery Usage:        " << trip.getFormattedBatteryUsage().toStdString() << std::endl;
            std::cout << "⚡ Energy Consumed:      " << trip.getFormattedEnergyConsumption().toStdString() << std::endl;
            std::cout << "📈 Energy Efficiency:    " << trip.getFormattedEnergyEfficiency().toStdString() << std::endl;
            std::cout << "�📊 Data Points Recorded: " << trip.dataPoints.size() << std::endl;
        } else {
            std::cout << "🔄 Status: TRIP IN PROGRESS" << std::endl;
            std::cout << "📊 Data Points So Far:   " << trip.dataPoints.size() << std::endl;
        }
        
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    }
    
    // Calculate total statistics
    double totalDistance = 0.0;
    qint64 totalDuration = 0;
    double totalMaxSpeed = 0.0;
    int completedTrips = 0;
    
    for (const TripInfo& trip : m_detectedTrips) {
        if (!trip.endTime.isNull()) {
            totalDistance += trip.distanceTraveled;
            totalDuration += trip.durationSeconds;
            if (trip.maxSpeed > totalMaxSpeed) {
                totalMaxSpeed = trip.maxSpeed;
            }
            completedTrips++;
        }
    }
    
    if (completedTrips > 0) {
        std::cout << "\n📈 OVERALL STATISTICS:" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "🏁 Completed Trips:      " << completedTrips << std::endl;
        std::cout << "📏 Total Distance:       " << QString::number(totalDistance, 'f', 2).toStdString() << " km" << std::endl;
        std::cout << "⏱️  Total Driving Time:   " << (totalDuration / 60) << " min " << (totalDuration % 60) << " sec" << std::endl;
        std::cout << "🚀 Highest Speed:        " << QString::number(totalMaxSpeed, 'f', 2).toStdString() 
                  << " m/s (" << QString::number(totalMaxSpeed * 3.6, 'f', 2).toStdString() << " km/h)" << std::endl;
        std::cout << "🏃 Average Trip Distance: " << QString::number(totalDistance / completedTrips, 'f', 2).toStdString() << " km" << std::endl;
        std::cout << "⏱️  Average Trip Duration: " << ((totalDuration / completedTrips) / 60) << " min " << ((totalDuration / completedTrips) % 60) << " sec" << std::endl;
    }
    
    std::cout << "===============================" << std::endl;
}

double InfluxDBClient::getBatteryChargeAtTime(const QDateTime& targetTime) const
{
    if (m_dataBuffer.isEmpty()) {
        return 0.0;
    }
    
    // Find the data point with timestamp closest to the target time
    double closestCharge = 0.0;
    qint64 minTimeDiff = std::numeric_limits<qint64>::max();
    bool foundValidData = false;
    
    for (const VehicleDataPoint& point : m_dataBuffer) {
        qint64 timeDiff = qAbs(point.timestamp.secsTo(targetTime));
        
        // Always consider the closest timestamp, regardless of whether battery charge is > 0
        // since 0 might be a valid battery reading or placeholder
        if (timeDiff < minTimeDiff) {
            minTimeDiff = timeDiff;
            closestCharge = point.batteryCharge;
            foundValidData = true;
        }
    }
    
    qDebug() << "getBatteryChargeAtTime: Target time:" << targetTime.toString() 
             << "Closest charge:" << closestCharge << "% Time diff:" << minTimeDiff << "seconds";
    
    return foundValidData ? closestCharge : 0.0;
}

bool InfluxDBClient::initializeDatabase()
{
    // Create database file in application data directory
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataPath);
    QString dbPath = dataPath + "/vehicle_trips.db";
    
    m_database = QSqlDatabase::addDatabase("QSQLITE");
    m_database.setDatabaseName(dbPath);
    
    if (!m_database.open()) {
        qDebug() << "Failed to open database:" << m_database.lastError().text();
        return false;
    }
    
    qDebug() << "Database opened successfully at:" << dbPath;
    
    // Create trips table with comprehensive schema
    QSqlQuery query(m_database);
    QString createTableSql = R"(
        CREATE TABLE IF NOT EXISTS trips (
            trip_id INTEGER PRIMARY KEY,
            start_time TEXT NOT NULL,
            end_time TEXT,
            max_speed REAL DEFAULT 0.0,
            average_speed REAL DEFAULT 0.0,
            distance_traveled REAL DEFAULT 0.0,
            duration_seconds INTEGER DEFAULT 0,
            start_battery_charge REAL DEFAULT 0.0,
            end_battery_charge REAL DEFAULT 0.0,
            battery_used_percent REAL DEFAULT 0.0,
            energy_consumed_wh REAL DEFAULT 0.0,
            energy_efficiency_whkm REAL DEFAULT 0.0,
            trip_name TEXT DEFAULT '',
            driver_name TEXT DEFAULT '',
            notes TEXT DEFAULT '',
            image_paths TEXT DEFAULT '',
            data_points_count INTEGER DEFAULT 0,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    if (!query.exec(createTableSql)) {
        qDebug() << "Failed to create trips table:" << query.lastError().text();
        return false;
    }
    
    // Add trip_name column if it doesn't exist (for backward compatibility)
    QString addColumnSql = "ALTER TABLE trips ADD COLUMN trip_name TEXT DEFAULT ''";
    query.exec(addColumnSql); // This will fail silently if column already exists
    
    qDebug() << "Database table 'trips' ready";
    return true;
}

bool InfluxDBClient::saveTripToDatabase(const TripInfo& trip)
{
    if (!m_database.isOpen()) {
        qDebug() << "Database not open - cannot save trip";
        return false;
    }
    
    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT OR REPLACE INTO trips (
            trip_id, start_time, end_time, max_speed, average_speed, 
            distance_traveled, duration_seconds, start_battery_charge, 
            end_battery_charge, battery_used_percent, energy_consumed_wh, 
            energy_efficiency_whkm, trip_name, driver_name, notes, image_paths, 
            data_points_count
        ) VALUES (
            :trip_id, :start_time, :end_time, :max_speed, :average_speed,
            :distance_traveled, :duration_seconds, :start_battery_charge,
            :end_battery_charge, :battery_used_percent, :energy_consumed_wh,
            :energy_efficiency_whkm, :trip_name, :driver_name, :notes, :image_paths,
            :data_points_count
        )
    )");
    
    // Bind all the comprehensive trip data
    query.bindValue(":trip_id", trip.tripId);
    query.bindValue(":start_time", trip.startTime.toString(Qt::ISODate));
    query.bindValue(":end_time", trip.endTime.isNull() ? QVariant() : trip.endTime.toString(Qt::ISODate));
    query.bindValue(":max_speed", trip.maxSpeed);
    query.bindValue(":average_speed", trip.averageSpeed);
    query.bindValue(":distance_traveled", trip.distanceTraveled);
    query.bindValue(":duration_seconds", trip.durationSeconds);
    query.bindValue(":start_battery_charge", trip.startBatteryCharge);
    query.bindValue(":end_battery_charge", trip.endBatteryCharge);
    query.bindValue(":battery_used_percent", trip.batteryUsedPercent);
    query.bindValue(":energy_consumed_wh", trip.energyConsumedWh);
    query.bindValue(":energy_efficiency_whkm", trip.energyEfficiencyWhKm);
    query.bindValue(":trip_name", trip.tripName);
    query.bindValue(":driver_name", trip.driverName);
    query.bindValue(":notes", trip.notes);
    query.bindValue(":image_paths", trip.imagePaths);
    query.bindValue(":data_points_count", trip.dataPoints.size());
    
    if (!query.exec()) {
        qDebug() << "Failed to save trip to database:" << query.lastError().text();
        return false;
    }
    
    qDebug() << "🗄️ TRIP" << trip.tripId << "SAVED TO DATABASE";
    qDebug() << "   Database record created with all comprehensive data";
    qDebug() << "   Driver:" << trip.driverName;
    qDebug() << "   Notes:" << trip.notes;
    qDebug() << "   Images:" << trip.imagePaths;
    qDebug() << "   Duration:" << trip.durationSeconds << "seconds";
    qDebug() << "   Distance:" << trip.distanceTraveled << "km";
    qDebug() << "   Energy consumed:" << trip.energyConsumedWh << "Wh";
    qDebug() << "   Data points:" << trip.dataPoints.size();
    
    return true;
}

QList<TripInfo> InfluxDBClient::loadTripsFromDatabase()
{
    QList<TripInfo> trips;
    
    if (!m_database.isOpen()) {
        qDebug() << "Database not open - cannot load trips";
        return trips;
    }
    
    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM trips ORDER BY start_time DESC");
    
    if (!query.exec()) {
        qDebug() << "Failed to load trips from database:" << query.lastError().text();
        return trips;
    }
    
    while (query.next()) {
        TripInfo trip;
        trip.tripId = query.value("trip_id").toInt();
        trip.startTime = QDateTime::fromString(query.value("start_time").toString(), Qt::ISODate);
        
        QString endTimeStr = query.value("end_time").toString();
        if (!endTimeStr.isEmpty()) {
            trip.endTime = QDateTime::fromString(endTimeStr, Qt::ISODate);
        }
        
        trip.maxSpeed = query.value("max_speed").toDouble();
        trip.averageSpeed = query.value("average_speed").toDouble();
        trip.distanceTraveled = query.value("distance_traveled").toDouble();
        trip.durationSeconds = query.value("duration_seconds").toLongLong();
        trip.startBatteryCharge = query.value("start_battery_charge").toDouble();
        trip.endBatteryCharge = query.value("end_battery_charge").toDouble();
        trip.batteryUsedPercent = query.value("battery_used_percent").toDouble();
        trip.energyConsumedWh = query.value("energy_consumed_wh").toDouble();
        trip.energyEfficiencyWhKm = query.value("energy_efficiency_whkm").toDouble();
        trip.tripName = query.value("trip_name").toString();
        trip.driverName = query.value("driver_name").toString();
        trip.notes = query.value("notes").toString();
        trip.imagePaths = query.value("image_paths").toString();
        
        trips.append(trip);
    }
    
    qDebug() << "📚 Loaded" << trips.size() << "trips from database";
    return trips;
}

bool InfluxDBClient::deleteTripFromDatabase(int tripId)
{
    if (!m_database.isOpen()) {
        qDebug() << "Database not open - cannot delete trip";
        return false;
    }
    
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM trips WHERE trip_id = :trip_id");
    query.bindValue(":trip_id", tripId);
    
    if (!query.exec()) {
        qDebug() << "Failed to delete trip from database:" << query.lastError().text();
        return false;
    }
    
    qDebug() << "🗑️ TRIP" << tripId << "DELETED FROM DATABASE";
    return true;
}

// QML-accessible methods
QVariantList InfluxDBClient::getAllTripsFromDatabase()
{
    QVariantList tripList;
    QList<TripInfo> trips = loadTripsFromDatabase();
    
    for (const TripInfo& trip : trips) {
        QVariantMap tripMap;
        tripMap["tripId"] = trip.tripId;
        tripMap["startTime"] = trip.startTime;
        tripMap["endTime"] = trip.endTime;
        tripMap["tripName"] = trip.getFormattedTripName();
        tripMap["driverName"] = trip.driverName;
        tripMap["formattedStartTime"] = trip.getFormattedStartTime();
        tripMap["formattedEndTime"] = trip.endTime.isNull() ? "Ongoing" : trip.getFormattedEndTime();
        tripMap["formattedDuration"] = trip.getFormattedDuration();
        tripMap["formattedDistance"] = trip.getFormattedDistance();
        tripMap["maxSpeed"] = trip.maxSpeed;
        tripMap["averageSpeed"] = trip.averageSpeed;
        tripMap["distanceTraveled"] = trip.distanceTraveled;
        tripMap["durationSeconds"] = trip.durationSeconds;
        tripMap["startBatteryCharge"] = trip.startBatteryCharge;
        tripMap["endBatteryCharge"] = trip.endBatteryCharge;
        tripMap["batteryUsedPercent"] = trip.batteryUsedPercent;
        tripMap["energyConsumedWh"] = trip.energyConsumedWh;
        tripMap["energyEfficiencyWhKm"] = trip.energyEfficiencyWhKm;
        tripMap["notes"] = trip.notes;
        tripMap["imagePaths"] = trip.imagePaths;
        tripMap["formattedBatteryUsage"] = trip.getFormattedBatteryUsage();
        tripMap["formattedEnergyConsumption"] = trip.getFormattedEnergyConsumption();
        tripMap["formattedEnergyEfficiency"] = trip.getFormattedEnergyEfficiency();
        
        tripList.append(tripMap);
    }
    
    return tripList;
}

QVariantMap InfluxDBClient::getTripDetails(int tripId)
{
    QList<TripInfo> trips = loadTripsFromDatabase();
    
    for (const TripInfo& trip : trips) {
        if (trip.tripId == tripId) {
            QVariantMap tripMap;
            tripMap["tripId"] = trip.tripId;
            tripMap["startTime"] = trip.startTime;
            tripMap["endTime"] = trip.endTime;
            tripMap["tripName"] = trip.getFormattedTripName();
            tripMap["driverName"] = trip.driverName;
            tripMap["formattedStartTime"] = trip.getFormattedStartTime();
            tripMap["formattedEndTime"] = trip.endTime.isNull() ? "Ongoing" : trip.getFormattedEndTime();
            tripMap["formattedDuration"] = trip.getFormattedDuration();
            tripMap["formattedDistance"] = trip.getFormattedDistance();
            tripMap["maxSpeed"] = trip.maxSpeed;
            tripMap["averageSpeed"] = trip.averageSpeed;
            tripMap["distanceTraveled"] = trip.distanceTraveled;
            tripMap["durationSeconds"] = trip.durationSeconds;
            tripMap["startBatteryCharge"] = trip.startBatteryCharge;
            tripMap["endBatteryCharge"] = trip.endBatteryCharge;
            tripMap["batteryUsedPercent"] = trip.batteryUsedPercent;
            tripMap["energyConsumedWh"] = trip.energyConsumedWh;
            tripMap["energyEfficiencyWhKm"] = trip.energyEfficiencyWhKm;
            tripMap["notes"] = trip.notes;
            tripMap["imagePaths"] = trip.imagePaths;
            tripMap["formattedBatteryUsage"] = trip.getFormattedBatteryUsage();
            tripMap["formattedEnergyConsumption"] = trip.getFormattedEnergyConsumption();
            tripMap["formattedEnergyEfficiency"] = trip.getFormattedEnergyEfficiency();
            
            return tripMap;
        }
    }
    
    return QVariantMap(); // Return empty map if trip not found
}

bool InfluxDBClient::deleteTrip(int tripId)
{
    bool success = deleteTripFromDatabase(tripId);
    if (success) {
        emit tripsUpdated();
    }
    return success;
}

void InfluxDBClient::refreshTrips()
{
    emit tripsUpdated();
}

bool InfluxDBClient::updateTripName(int tripId, const QString& tripName)
{
    return updateTripInDatabase(tripId, "trip_name", tripName);
}

bool InfluxDBClient::updateTripDriver(int tripId, const QString& driverName)
{
    return updateTripInDatabase(tripId, "driver_name", driverName);
}

bool InfluxDBClient::updateTripNotes(int tripId, const QString& notes)
{
    return updateTripInDatabase(tripId, "notes", notes);
}

bool InfluxDBClient::updateTripInDatabase(int tripId, const QString& field, const QString& value)
{
    if (!m_database.isOpen()) {
        qDebug() << "Database not open - cannot update trip";
        return false;
    }
    
    QSqlQuery query(m_database);
    QString sql = QString("UPDATE trips SET %1 = :value WHERE trip_id = :trip_id").arg(field);
    query.prepare(sql);
    query.bindValue(":value", value);
    query.bindValue(":trip_id", tripId);
    
    if (!query.exec()) {
        qDebug() << "Failed to update trip in database:" << query.lastError().text();
        return false;
    }
    
    emit tripsUpdated();
    qDebug() << "✏️ TRIP" << tripId << field << "UPDATED TO:" << value;
    return true;
}
