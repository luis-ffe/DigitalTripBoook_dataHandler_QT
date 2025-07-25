#ifndef INFLUXDBCLIENT_H
#define INFLUXDBCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>
#include <QList>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QVariantList>
#include <QVariantMap>

// Battery configuration constants
const double BATTERY_CAPACITY_MAH = 6 * 3200; // Six 3200 mAh batteries = 19200 mAh total
const double BATTERY_VOLTAGE = 12.0; // Typical 12V system
const double BATTERY_CAPACITY_WH = (BATTERY_CAPACITY_MAH / 1000.0) * BATTERY_VOLTAGE; // Watt-hours

// Structure to hold individual data points (speed + battery)
struct VehicleDataPoint {
    QDateTime timestamp;
    double speed;        // m/s
    double batteryCharge; // percentage (0-100)
    
    VehicleDataPoint() : speed(0.0), batteryCharge(0.0) {}
    VehicleDataPoint(const QDateTime& time, double spd, double charge = 0.0) 
        : timestamp(time), speed(spd), batteryCharge(charge) {}
};

// Legacy alias for backward compatibility
using SpeedDataPoint = VehicleDataPoint;

// Enhanced structure to hold comprehensive trip information with battery data
struct TripInfo {
    int tripId;
    QDateTime startTime;
    QDateTime endTime;
    double maxSpeed;
    double averageSpeed;
    double distanceTraveled;
    qint64 durationSeconds;
    
    // Battery and energy data
    double startBatteryCharge;    // % at trip start
    double endBatteryCharge;      // % at trip end
    double batteryUsedPercent;    // % consumed during trip
    double energyConsumedWh;      // Watt-hours consumed
    double energyEfficiencyWhKm;  // Wh per km
    
    // Trip metadata
    QString tripName;
    QString driverName;
    QString notes;
    QString imagePaths;
    
    QList<VehicleDataPoint> dataPoints;
    
    TripInfo() : tripId(-1), maxSpeed(0.0), averageSpeed(0.0), distanceTraveled(0.0), 
                durationSeconds(0), startBatteryCharge(0.0), endBatteryCharge(0.0),
                batteryUsedPercent(0.0), energyConsumedWh(0.0), energyEfficiencyWhKm(0.0),
                tripName(""), driverName(""), notes(""), imagePaths("") {}
    
    void calculateStatistics() {
        if (dataPoints.isEmpty()) return;
        
        double totalSpeed = 0.0;
        maxSpeed = 0.0;
        distanceTraveled = 0.0;
        
        // Get battery data from first and last data points
        startBatteryCharge = dataPoints.first().batteryCharge;
        endBatteryCharge = dataPoints.last().batteryCharge;
        batteryUsedPercent = startBatteryCharge - endBatteryCharge;
        
        // Calculate speed statistics
        for (const auto& point : dataPoints) {
            totalSpeed += point.speed;
            if (point.speed > maxSpeed) {
                maxSpeed = point.speed;
            }
        }
        
        averageSpeed = totalSpeed / dataPoints.size();
        
        // Calculate duration
        if (!startTime.isNull() && !endTime.isNull()) {
            durationSeconds = startTime.secsTo(endTime);
        }
        
        // Calculate distance (using average speed * time)
        // Convert average speed from m/s to km/h for distance calculation
        double avgSpeedKmh = averageSpeed * 3.6;
        double durationHours = durationSeconds / 3600.0;
        distanceTraveled = avgSpeedKmh * durationHours; // km
        
        // Calculate energy consumption
        if (batteryUsedPercent > 0) {
            energyConsumedWh = (batteryUsedPercent / 100.0) * BATTERY_CAPACITY_WH;
            
            // Calculate energy efficiency (Wh per km)
            if (distanceTraveled > 0) {
                energyEfficiencyWhKm = energyConsumedWh / distanceTraveled;
            }
        }
    }
    
    QString getFormattedDuration() const {
        int minutes = durationSeconds / 60;
        int seconds = durationSeconds % 60;
        return QString("%1 min %2 sec").arg(minutes).arg(seconds);
    }
    
    QString getFormattedDistance() const {
        return QString("%1 km").arg(distanceTraveled, 0, 'f', 2);
    }
    
    QString getFormattedStartTime() const {
        return startTime.toString("yyyy-MM-dd hh:mm:ss");
    }
    
    QString getFormattedEndTime() const {
        return endTime.toString("yyyy-MM-dd hh:mm:ss");
    }
    
    QString getFormattedBatteryUsage() const {
        return QString("%1% → %2% (-%3%)")
            .arg(startBatteryCharge, 0, 'f', 1)
            .arg(endBatteryCharge, 0, 'f', 1)
            .arg(batteryUsedPercent, 0, 'f', 1);
    }
    
    QString getFormattedEnergyConsumption() const {
        return QString("%1 Wh").arg(energyConsumedWh, 0, 'f', 1);
    }
    
    QString getFormattedEnergyEfficiency() const {
        if (energyEfficiencyWhKm > 0) {
            return QString("%1 Wh/km").arg(energyEfficiencyWhKm, 0, 'f', 1);
        }
        return "N/A";
    }
    
    QString getFormattedTripName() const {
        if (tripName.isEmpty()) {
            return QString("Trip %1").arg(tripId);
        }
        return tripName;
    }
};

class InfluxDBClient : public QObject
{
    Q_OBJECT

public:
    explicit InfluxDBClient(QObject *parent = nullptr);
    void startDataCollection();
    void stopDataCollection();
    void fetchLatestData();
    
    // Trip detection methods
    QList<TripInfo> getDetectedTrips() const { return m_detectedTrips; }
    void printTripSummary();
    void printDetailedTripInfo();
    
    // Database methods
    bool initializeDatabase();
    bool saveTripToDatabase(const TripInfo& trip);
    QList<TripInfo> loadTripsFromDatabase();
    bool deleteTripFromDatabase(int tripId);
    
    // QML-accessible methods
    Q_INVOKABLE QVariantList getAllTripsFromDatabase();
    Q_INVOKABLE QVariantMap getTripDetails(int tripId);
    Q_INVOKABLE bool deleteTrip(int tripId);
    Q_INVOKABLE void refreshTrips();
    Q_INVOKABLE bool updateTripName(int tripId, const QString& tripName);
    Q_INVOKABLE bool updateTripDriver(int tripId, const QString& driverName);
    Q_INVOKABLE bool updateTripNotes(int tripId, const QString& notes);

signals:
    void tripStarted(int tripId, const QDateTime& startTime);
    void tripEnded(int tripId, const QDateTime& endTime, double maxSpeed, double avgSpeed, double distance, qint64 duration);
    void tripsUpdated();

private slots:
    void onDataReceived();
    void onNetworkError(QNetworkReply::NetworkError error);
    void onTimerTimeout();

private:
    void makeInfluxDBRequest();
    void parseInfluxDBResponse(const QByteArray &data);
    void addDataPoint(const QDateTime& timestamp, double speed, double batteryCharge = 0.0);
    void analyzeForTrips();
    bool isMoving(double speed) const { return speed > 0.0; }
    double getBatteryChargeAtTime(const QDateTime& targetTime) const;
    
    // Trip detection logic
    void checkTripStart();
    void checkTripEnd();
    void finalizeTripIfComplete();
    bool updateTripInDatabase(int tripId, const QString& field, const QString& value);

    QNetworkAccessManager *m_networkManager;
    QTimer *m_timer;
    QString m_url;
    QString m_token;
    QString m_org;
    QString m_bucket;
    
    // Database
    QSqlDatabase m_database;
    
    // Latest values
    double m_speed;
    double m_charge;
    double m_autonomyLevel;
    
    // Data storage and trip detection
    QList<VehicleDataPoint> m_dataBuffer;
    QList<TripInfo> m_detectedTrips;
    
    // Trip detection state
    bool m_inTrip;
    int m_nextTripId;
    QDateTime m_potentialTripStart;
    QDateTime m_lastMovementTime;
    QDateTime m_potentialTripEnd;
    
    // Trip detection parameters
    static const int TRIP_START_DURATION_SECONDS = 60;    // 1 minute of movement to start trip
    static const int TRIP_END_DURATION_SECONDS = 60;      // 1 minute of no movement to end trip
    static const int TRIP_TIMEOUT_SECONDS = 300;          // 5 minutes without data to force trip end
};

#endif // INFLUXDBCLIENT_H
