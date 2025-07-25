#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDebug>
#include <QTimer>
#include "influxdbclient.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qDebug() << "=== Trip Detection System Starting ===";
    qDebug() << "This application will analyze InfluxDB vehicle data to detect trips";
    qDebug() << "Trip Detection Rules:";
    qDebug() << "- Trip Start: Speed > 0 for at least 1 minute";
    qDebug() << "- Trip End: Speed = 0 for more than 1 minute";
    qDebug() << "- Trip Timeout: Force end after 5 minutes without data";
    qDebug() << "Analysis runs every 30 seconds";
    qDebug() << "=====================================";

    // Create InfluxDB client
    InfluxDBClient *influxClient = new InfluxDBClient(&app);
    
    // Connect signals for trip events
    QObject::connect(influxClient, &InfluxDBClient::tripStarted, [](int tripId, const QDateTime& startTime) {
        qDebug() << "🚗 TRIP" << tripId << "STARTED at" << startTime.toString();
    });
    
    QObject::connect(influxClient, &InfluxDBClient::tripEnded, 
        [influxClient](int tripId, const QDateTime& endTime, double maxSpeed, double avgSpeed, double distance, qint64 duration) {
        qDebug() << "🏁 TRIP" << tripId << "ENDED at" << endTime.toString();
        qDebug() << "   Max Speed:" << maxSpeed << "m/s (" << (maxSpeed * 3.6) << "km/h)";
        qDebug() << "   Avg Speed:" << avgSpeed << "m/s (" << (avgSpeed * 3.6) << "km/h)";
        qDebug() << "   Distance:" << distance << "km";
        qDebug() << "   Duration:" << (duration / 60) << "min" << (duration % 60) << "sec";
        
        // Print detailed trip information after each trip ends
        influxClient->printDetailedTripInfo();
    });
    
    // Start trip analysis
    influxClient->startDataCollection();
    
    // Setup a timer to periodically show detailed trip information (every 5 minutes)
    QTimer *statusTimer = new QTimer(&app);
    QObject::connect(statusTimer, &QTimer::timeout, [influxClient]() {
        qDebug() << "\n=== PERIODIC STATUS UPDATE ===";
        influxClient->printDetailedTripInfo();
    });
    statusTimer->start(300000); // 5 minutes = 300,000 milliseconds
    
    // Setup a final report timer (after 10 minutes of running)
    QTimer *finalReportTimer = new QTimer(&app);
    finalReportTimer->setSingleShot(true);
    QObject::connect(finalReportTimer, &QTimer::timeout, [influxClient, &app]() {
        qDebug() << "\n=== FINAL TRIP ANALYSIS REPORT ===";
        influxClient->printDetailedTripInfo();
        qDebug() << "\nApplication will continue running. Press Ctrl+C to exit.";
    });
    finalReportTimer->start(600000); // 10 minutes = 600,000 milliseconds
    
    // Optional: Setup QML engine if you want a GUI later
    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    
    // For now, we'll run without loading QML since we're focusing on terminal output
    // engine.loadFromModule("dataHandler", "Main");

    qDebug() << "DataHandler: Application started. Check terminal for InfluxDB data updates...";

    return app.exec();
}
