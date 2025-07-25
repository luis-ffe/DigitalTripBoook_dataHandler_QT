/****************************************************************************
** Meta object code from reading C++ file 'influxdbclient.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../influxdbclient.h"
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'influxdbclient.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.9.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN14InfluxDBClientE_t {};
} // unnamed namespace

template <> constexpr inline auto InfluxDBClient::qt_create_metaobjectdata<qt_meta_tag_ZN14InfluxDBClientE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "InfluxDBClient",
        "tripStarted",
        "",
        "tripId",
        "startTime",
        "tripEnded",
        "endTime",
        "maxSpeed",
        "avgSpeed",
        "distance",
        "duration",
        "tripsUpdated",
        "onDataReceived",
        "onNetworkError",
        "QNetworkReply::NetworkError",
        "error",
        "onTimerTimeout",
        "getAllTripsFromDatabase",
        "QVariantList",
        "getTripDetails",
        "QVariantMap",
        "deleteTrip",
        "refreshTrips",
        "updateTripName",
        "tripName",
        "updateTripDriver",
        "driverName",
        "updateTripNotes",
        "notes"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'tripStarted'
        QtMocHelpers::SignalData<void(int, const QDateTime &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { QMetaType::QDateTime, 4 },
        }}),
        // Signal 'tripEnded'
        QtMocHelpers::SignalData<void(int, const QDateTime &, double, double, double, qint64)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { QMetaType::QDateTime, 6 }, { QMetaType::Double, 7 }, { QMetaType::Double, 8 },
            { QMetaType::Double, 9 }, { QMetaType::LongLong, 10 },
        }}),
        // Signal 'tripsUpdated'
        QtMocHelpers::SignalData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onDataReceived'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onNetworkError'
        QtMocHelpers::SlotData<void(QNetworkReply::NetworkError)>(13, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 14, 15 },
        }}),
        // Slot 'onTimerTimeout'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
        // Method 'getAllTripsFromDatabase'
        QtMocHelpers::MethodData<QVariantList()>(17, 2, QMC::AccessPublic, 0x80000000 | 18),
        // Method 'getTripDetails'
        QtMocHelpers::MethodData<QVariantMap(int)>(19, 2, QMC::AccessPublic, 0x80000000 | 20, {{
            { QMetaType::Int, 3 },
        }}),
        // Method 'deleteTrip'
        QtMocHelpers::MethodData<bool(int)>(21, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::Int, 3 },
        }}),
        // Method 'refreshTrips'
        QtMocHelpers::MethodData<void()>(22, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'updateTripName'
        QtMocHelpers::MethodData<bool(int, const QString &)>(23, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::Int, 3 }, { QMetaType::QString, 24 },
        }}),
        // Method 'updateTripDriver'
        QtMocHelpers::MethodData<bool(int, const QString &)>(25, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::Int, 3 }, { QMetaType::QString, 26 },
        }}),
        // Method 'updateTripNotes'
        QtMocHelpers::MethodData<bool(int, const QString &)>(27, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::Int, 3 }, { QMetaType::QString, 28 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<InfluxDBClient, qt_meta_tag_ZN14InfluxDBClientE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject InfluxDBClient::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14InfluxDBClientE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14InfluxDBClientE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14InfluxDBClientE_t>.metaTypes,
    nullptr
} };

void InfluxDBClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<InfluxDBClient *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->tripStarted((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QDateTime>>(_a[2]))); break;
        case 1: _t->tripEnded((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QDateTime>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[5])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[6]))); break;
        case 2: _t->tripsUpdated(); break;
        case 3: _t->onDataReceived(); break;
        case 4: _t->onNetworkError((*reinterpret_cast< std::add_pointer_t<QNetworkReply::NetworkError>>(_a[1]))); break;
        case 5: _t->onTimerTimeout(); break;
        case 6: { QVariantList _r = _t->getAllTripsFromDatabase();
            if (_a[0]) *reinterpret_cast< QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 7: { QVariantMap _r = _t->getTripDetails((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 8: { bool _r = _t->deleteTrip((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 9: _t->refreshTrips(); break;
        case 10: { bool _r = _t->updateTripName((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 11: { bool _r = _t->updateTripDriver((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 12: { bool _r = _t->updateTripNotes((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply::NetworkError >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (InfluxDBClient::*)(int , const QDateTime & )>(_a, &InfluxDBClient::tripStarted, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (InfluxDBClient::*)(int , const QDateTime & , double , double , double , qint64 )>(_a, &InfluxDBClient::tripEnded, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (InfluxDBClient::*)()>(_a, &InfluxDBClient::tripsUpdated, 2))
            return;
    }
}

const QMetaObject *InfluxDBClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *InfluxDBClient::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14InfluxDBClientE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int InfluxDBClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    }
    return _id;
}

// SIGNAL 0
void InfluxDBClient::tripStarted(int _t1, const QDateTime & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}

// SIGNAL 1
void InfluxDBClient::tripEnded(int _t1, const QDateTime & _t2, double _t3, double _t4, double _t5, qint64 _t6)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2, _t3, _t4, _t5, _t6);
}

// SIGNAL 2
void InfluxDBClient::tripsUpdated()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}
QT_WARNING_POP
