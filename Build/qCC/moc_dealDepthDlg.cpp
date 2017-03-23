/****************************************************************************
** Meta object code from reading C++ file 'dealDepthDlg.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../trunk-master/qCC/db_tree/dealDepthDlg.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'dealDepthDlg.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_DealDepthDlg_t {
    QByteArrayData data[25];
    char stringdata0[245];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_DealDepthDlg_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_DealDepthDlg_t qt_meta_stringdata_DealDepthDlg = {
    {
QT_MOC_LITERAL(0, 0, 12), // "DealDepthDlg"
QT_MOC_LITERAL(1, 13, 13), // "addDepthImage"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 10), // "Buttonplay"
QT_MOC_LITERAL(4, 39, 9), // "playVideo"
QT_MOC_LITERAL(5, 49, 12), // "getMultiView"
QT_MOC_LITERAL(6, 62, 14), // "doMarchingCube"
QT_MOC_LITERAL(7, 77, 8), // "ccImage*"
QT_MOC_LITERAL(8, 86, 5), // "nview"
QT_MOC_LITERAL(9, 92, 14), // "lengthPerPixel"
QT_MOC_LITERAL(10, 107, 11), // "cameraPara&"
QT_MOC_LITERAL(11, 119, 5), // "m_cam"
QT_MOC_LITERAL(12, 125, 14), // "dilateVoxelNum"
QT_MOC_LITERAL(13, 140, 10), // "getNewView"
QT_MOC_LITERAL(14, 151, 8), // "ccImage&"
QT_MOC_LITERAL(15, 160, 6), // "result"
QT_MOC_LITERAL(16, 167, 9), // "dealHoles"
QT_MOC_LITERAL(17, 177, 7), // "newView"
QT_MOC_LITERAL(18, 185, 7), // "QImage&"
QT_MOC_LITERAL(19, 193, 8), // "newImage"
QT_MOC_LITERAL(20, 202, 8), // "depthImg"
QT_MOC_LITERAL(21, 211, 7), // "maskImg"
QT_MOC_LITERAL(22, 219, 10), // "connectOBJ"
QT_MOC_LITERAL(23, 230, 10), // "ccHObject*"
QT_MOC_LITERAL(24, 241, 3) // "obj"

    },
    "DealDepthDlg\0addDepthImage\0\0Buttonplay\0"
    "playVideo\0getMultiView\0doMarchingCube\0"
    "ccImage*\0nview\0lengthPerPixel\0cameraPara&\0"
    "m_cam\0dilateVoxelNum\0getNewView\0"
    "ccImage&\0result\0dealHoles\0newView\0"
    "QImage&\0newImage\0depthImg\0maskImg\0"
    "connectOBJ\0ccHObject*\0obj"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DealDepthDlg[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   54,    2, 0x0a /* Public */,
       3,    0,   55,    2, 0x0a /* Public */,
       4,    0,   56,    2, 0x0a /* Public */,
       5,    0,   57,    2, 0x0a /* Public */,
       6,    4,   58,    2, 0x0a /* Public */,
      13,    1,   67,    2, 0x0a /* Public */,
      16,    6,   70,    2, 0x0a /* Public */,
      22,    1,   83,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 7, QMetaType::Double, 0x80000000 | 10, QMetaType::Int,    8,    9,   11,   12,
    QMetaType::Void, 0x80000000 | 14,   15,
    QMetaType::Void, 0x80000000 | 7, 0x80000000 | 18, 0x80000000 | 18, 0x80000000 | 18, QMetaType::Double, 0x80000000 | 14,   17,   19,   20,   21,    9,   15,
    QMetaType::Void, 0x80000000 | 23,   24,

       0        // eod
};

void DealDepthDlg::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        DealDepthDlg *_t = static_cast<DealDepthDlg *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->addDepthImage(); break;
        case 1: _t->Buttonplay(); break;
        case 2: _t->playVideo(); break;
        case 3: _t->getMultiView(); break;
        case 4: _t->doMarchingCube((*reinterpret_cast< ccImage*(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2])),(*reinterpret_cast< cameraPara(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        case 5: _t->getNewView((*reinterpret_cast< ccImage(*)>(_a[1]))); break;
        case 6: _t->dealHoles((*reinterpret_cast< ccImage*(*)>(_a[1])),(*reinterpret_cast< QImage(*)>(_a[2])),(*reinterpret_cast< QImage(*)>(_a[3])),(*reinterpret_cast< QImage(*)>(_a[4])),(*reinterpret_cast< double(*)>(_a[5])),(*reinterpret_cast< ccImage(*)>(_a[6]))); break;
        case 7: _t->connectOBJ((*reinterpret_cast< ccHObject*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject DealDepthDlg::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_DealDepthDlg.data,
      qt_meta_data_DealDepthDlg,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *DealDepthDlg::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DealDepthDlg::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_DealDepthDlg.stringdata0))
        return static_cast<void*>(const_cast< DealDepthDlg*>(this));
    if (!strcmp(_clname, "Ui::dealDepthDlg"))
        return static_cast< Ui::dealDepthDlg*>(const_cast< DealDepthDlg*>(this));
    return QWidget::qt_metacast(_clname);
}

int DealDepthDlg::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 8;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
