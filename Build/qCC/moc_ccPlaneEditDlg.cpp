/****************************************************************************
** Meta object code from reading C++ file 'ccPlaneEditDlg.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../trunk-master/qCC/ccPlaneEditDlg.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ccPlaneEditDlg.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_ccPlaneEditDlg_t {
    QByteArrayData data[7];
    char stringdata0[93];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ccPlaneEditDlg_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ccPlaneEditDlg_t qt_meta_stringdata_ccPlaneEditDlg = {
    {
QT_MOC_LITERAL(0, 0, 14), // "ccPlaneEditDlg"
QT_MOC_LITERAL(1, 15, 17), // "pickPointAsCenter"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 17), // "processPickedItem"
QT_MOC_LITERAL(4, 52, 10), // "ccHObject*"
QT_MOC_LITERAL(5, 63, 9), // "CCVector3"
QT_MOC_LITERAL(6, 73, 19) // "saveParamsAndAccept"

    },
    "ccPlaneEditDlg\0pickPointAsCenter\0\0"
    "processPickedItem\0ccHObject*\0CCVector3\0"
    "saveParamsAndAccept"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ccPlaneEditDlg[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   29,    2, 0x0a /* Public */,
       3,    5,   32,    2, 0x0a /* Public */,
       6,    0,   43,    2, 0x09 /* Protected */,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void, 0x80000000 | 4, QMetaType::UInt, QMetaType::Int, QMetaType::Int, 0x80000000 | 5,    2,    2,    2,    2,    2,
    QMetaType::Void,

       0        // eod
};

void ccPlaneEditDlg::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        ccPlaneEditDlg *_t = static_cast<ccPlaneEditDlg *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->pickPointAsCenter((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: _t->processPickedItem((*reinterpret_cast< ccHObject*(*)>(_a[1])),(*reinterpret_cast< uint(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4])),(*reinterpret_cast< const CCVector3(*)>(_a[5]))); break;
        case 2: _t->saveParamsAndAccept(); break;
        default: ;
        }
    }
}

const QMetaObject ccPlaneEditDlg::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_ccPlaneEditDlg.data,
      qt_meta_data_ccPlaneEditDlg,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *ccPlaneEditDlg::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ccPlaneEditDlg::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_ccPlaneEditDlg.stringdata0))
        return static_cast<void*>(const_cast< ccPlaneEditDlg*>(this));
    if (!strcmp(_clname, "Ui::PlaneEditDlg"))
        return static_cast< Ui::PlaneEditDlg*>(const_cast< ccPlaneEditDlg*>(this));
    return QDialog::qt_metacast(_clname);
}

int ccPlaneEditDlg::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
