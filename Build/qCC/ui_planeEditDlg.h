/********************************************************************************
** Form generated from reading UI file 'planeEditDlg.ui'
**
** Created by: Qt User Interface Compiler version 5.5.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PLANEEDITDLG_H
#define UI_PLANEEDITDLG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_PlaneEditDlg
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *groupBox;
    QGridLayout *gridLayout;
    QDoubleSpinBox *dipDoubleSpinBox;
    QLabel *label_3;
    QLabel *label_4;
    QDoubleSpinBox *dipDirDoubleSpinBox;
    QCheckBox *upwardCheckBox;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout_2;
    QLabel *label;
    QDoubleSpinBox *wDoubleSpinBox;
    QLabel *label_2;
    QDoubleSpinBox *hDoubleSpinBox;
    QGroupBox *groupBox_3;
    QGridLayout *gridLayout_3;
    QDoubleSpinBox *cxAxisDoubleSpinBox;
    QToolButton *pickCenterToolButton;
    QDoubleSpinBox *cyAxisDoubleSpinBox;
    QDoubleSpinBox *czAxisDoubleSpinBox;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *PlaneEditDlg)
    {
        if (PlaneEditDlg->objectName().isEmpty())
            PlaneEditDlg->setObjectName(QStringLiteral("PlaneEditDlg"));
        PlaneEditDlg->resize(300, 380);
        verticalLayout = new QVBoxLayout(PlaneEditDlg);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        groupBox = new QGroupBox(PlaneEditDlg);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        groupBox->setFlat(true);
        gridLayout = new QGridLayout(groupBox);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        dipDoubleSpinBox = new QDoubleSpinBox(groupBox);
        dipDoubleSpinBox->setObjectName(QStringLiteral("dipDoubleSpinBox"));
        dipDoubleSpinBox->setDecimals(3);
        dipDoubleSpinBox->setMinimum(0);
        dipDoubleSpinBox->setMaximum(90);

        gridLayout->addWidget(dipDoubleSpinBox, 0, 1, 1, 1);

        label_3 = new QLabel(groupBox);
        label_3->setObjectName(QStringLiteral("label_3"));

        gridLayout->addWidget(label_3, 0, 0, 1, 1);

        label_4 = new QLabel(groupBox);
        label_4->setObjectName(QStringLiteral("label_4"));

        gridLayout->addWidget(label_4, 1, 0, 1, 1);

        dipDirDoubleSpinBox = new QDoubleSpinBox(groupBox);
        dipDirDoubleSpinBox->setObjectName(QStringLiteral("dipDirDoubleSpinBox"));
        dipDirDoubleSpinBox->setDecimals(4);
        dipDirDoubleSpinBox->setMinimum(0);
        dipDirDoubleSpinBox->setMaximum(360);

        gridLayout->addWidget(dipDirDoubleSpinBox, 1, 1, 1, 1);

        upwardCheckBox = new QCheckBox(groupBox);
        upwardCheckBox->setObjectName(QStringLiteral("upwardCheckBox"));
        upwardCheckBox->setChecked(true);

        gridLayout->addWidget(upwardCheckBox, 2, 0, 1, 1);


        verticalLayout->addWidget(groupBox);

        groupBox_2 = new QGroupBox(PlaneEditDlg);
        groupBox_2->setObjectName(QStringLiteral("groupBox_2"));
        groupBox_2->setFlat(true);
        gridLayout_2 = new QGridLayout(groupBox_2);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        label = new QLabel(groupBox_2);
        label->setObjectName(QStringLiteral("label"));

        gridLayout_2->addWidget(label, 0, 0, 1, 1);

        wDoubleSpinBox = new QDoubleSpinBox(groupBox_2);
        wDoubleSpinBox->setObjectName(QStringLiteral("wDoubleSpinBox"));
        wDoubleSpinBox->setDecimals(6);
        wDoubleSpinBox->setMinimum(1e-06);
        wDoubleSpinBox->setMaximum(1e+06);
        wDoubleSpinBox->setValue(10);

        gridLayout_2->addWidget(wDoubleSpinBox, 0, 1, 1, 1);

        label_2 = new QLabel(groupBox_2);
        label_2->setObjectName(QStringLiteral("label_2"));

        gridLayout_2->addWidget(label_2, 1, 0, 1, 1);

        hDoubleSpinBox = new QDoubleSpinBox(groupBox_2);
        hDoubleSpinBox->setObjectName(QStringLiteral("hDoubleSpinBox"));
        hDoubleSpinBox->setDecimals(6);
        hDoubleSpinBox->setMinimum(1e-06);
        hDoubleSpinBox->setMaximum(1e+06);
        hDoubleSpinBox->setValue(10);

        gridLayout_2->addWidget(hDoubleSpinBox, 1, 1, 1, 1);


        verticalLayout->addWidget(groupBox_2);

        groupBox_3 = new QGroupBox(PlaneEditDlg);
        groupBox_3->setObjectName(QStringLiteral("groupBox_3"));
        groupBox_3->setFlat(true);
        gridLayout_3 = new QGridLayout(groupBox_3);
        gridLayout_3->setObjectName(QStringLiteral("gridLayout_3"));
        cxAxisDoubleSpinBox = new QDoubleSpinBox(groupBox_3);
        cxAxisDoubleSpinBox->setObjectName(QStringLiteral("cxAxisDoubleSpinBox"));
        cxAxisDoubleSpinBox->setDecimals(8);
        cxAxisDoubleSpinBox->setMinimum(-1e+09);
        cxAxisDoubleSpinBox->setMaximum(1e+09);

        gridLayout_3->addWidget(cxAxisDoubleSpinBox, 0, 1, 1, 1);

        pickCenterToolButton = new QToolButton(groupBox_3);
        pickCenterToolButton->setObjectName(QStringLiteral("pickCenterToolButton"));
        QIcon icon;
        icon.addFile(QStringLiteral(":/CC/images/ccPointPicking.png"), QSize(), QIcon::Normal, QIcon::Off);
        pickCenterToolButton->setIcon(icon);
        pickCenterToolButton->setCheckable(true);

        gridLayout_3->addWidget(pickCenterToolButton, 1, 0, 1, 1);

        cyAxisDoubleSpinBox = new QDoubleSpinBox(groupBox_3);
        cyAxisDoubleSpinBox->setObjectName(QStringLiteral("cyAxisDoubleSpinBox"));
        cyAxisDoubleSpinBox->setDecimals(8);
        cyAxisDoubleSpinBox->setMinimum(-1e+09);
        cyAxisDoubleSpinBox->setMaximum(1e+09);

        gridLayout_3->addWidget(cyAxisDoubleSpinBox, 1, 1, 1, 1);

        czAxisDoubleSpinBox = new QDoubleSpinBox(groupBox_3);
        czAxisDoubleSpinBox->setObjectName(QStringLiteral("czAxisDoubleSpinBox"));
        czAxisDoubleSpinBox->setDecimals(8);
        czAxisDoubleSpinBox->setMinimum(-1e+09);
        czAxisDoubleSpinBox->setMaximum(1e+09);

        gridLayout_3->addWidget(czAxisDoubleSpinBox, 2, 1, 1, 1);


        verticalLayout->addWidget(groupBox_3);

        verticalSpacer = new QSpacerItem(20, 24, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        buttonBox = new QDialogButtonBox(PlaneEditDlg);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(PlaneEditDlg);
        QObject::connect(buttonBox, SIGNAL(rejected()), PlaneEditDlg, SLOT(reject()));

        QMetaObject::connectSlotsByName(PlaneEditDlg);
    } // setupUi

    void retranslateUi(QDialog *PlaneEditDlg)
    {
        PlaneEditDlg->setWindowTitle(QApplication::translate("PlaneEditDlg", "Plane properties", 0));
        groupBox->setTitle(QApplication::translate("PlaneEditDlg", "Orientation", 0));
        dipDoubleSpinBox->setSuffix(QApplication::translate("PlaneEditDlg", " deg.", 0));
        label_3->setText(QApplication::translate("PlaneEditDlg", "dip", 0));
        label_4->setText(QApplication::translate("PlaneEditDlg", "dip direction", 0));
        dipDirDoubleSpinBox->setSuffix(QApplication::translate("PlaneEditDlg", " deg.", 0));
#ifndef QT_NO_TOOLTIP
        upwardCheckBox->setToolTip(QApplication::translate("PlaneEditDlg", "Whether the plane normal should point upward (Z+) or backward (Z-)", 0));
#endif // QT_NO_TOOLTIP
        upwardCheckBox->setText(QApplication::translate("PlaneEditDlg", "upward", 0));
        groupBox_2->setTitle(QApplication::translate("PlaneEditDlg", "Dimensions", 0));
        label->setText(QApplication::translate("PlaneEditDlg", "width", 0));
#ifndef QT_NO_TOOLTIP
        wDoubleSpinBox->setToolTip(QApplication::translate("PlaneEditDlg", "Plane width", 0));
#endif // QT_NO_TOOLTIP
        label_2->setText(QApplication::translate("PlaneEditDlg", "height", 0));
#ifndef QT_NO_TOOLTIP
        hDoubleSpinBox->setToolTip(QApplication::translate("PlaneEditDlg", "Plane height", 0));
#endif // QT_NO_TOOLTIP
        groupBox_3->setTitle(QApplication::translate("PlaneEditDlg", "Center", 0));
        cxAxisDoubleSpinBox->setPrefix(QApplication::translate("PlaneEditDlg", "X:", 0));
#ifndef QT_NO_TOOLTIP
        pickCenterToolButton->setToolTip(QApplication::translate("PlaneEditDlg", "Pick the plane center (click again to cancel)", 0));
#endif // QT_NO_TOOLTIP
        cyAxisDoubleSpinBox->setPrefix(QApplication::translate("PlaneEditDlg", "Y:", 0));
        czAxisDoubleSpinBox->setPrefix(QApplication::translate("PlaneEditDlg", "Z:", 0));
    } // retranslateUi

};

namespace Ui {
    class PlaneEditDlg: public Ui_PlaneEditDlg {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PLANEEDITDLG_H
