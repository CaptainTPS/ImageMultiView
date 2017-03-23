/********************************************************************************
** Form generated from reading UI file 'dealDepthDlg.ui'
**
** Created by: Qt User Interface Compiler version 5.5.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DEALDEPTHDLG_H
#define UI_DEALDEPTHDLG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_dealDepthDlg
{
public:
    QPushButton *addButton;
    QPushButton *playButton;
    QPushButton *newViewButton;

    void setupUi(QWidget *dealDepthDlg)
    {
        if (dealDepthDlg->objectName().isEmpty())
            dealDepthDlg->setObjectName(QStringLiteral("dealDepthDlg"));
        dealDepthDlg->resize(250, 153);
        addButton = new QPushButton(dealDepthDlg);
        addButton->setObjectName(QStringLiteral("addButton"));
        addButton->setGeometry(QRect(40, 20, 171, 31));
        playButton = new QPushButton(dealDepthDlg);
        playButton->setObjectName(QStringLiteral("playButton"));
        playButton->setGeometry(QRect(40, 100, 171, 31));
        newViewButton = new QPushButton(dealDepthDlg);
        newViewButton->setObjectName(QStringLiteral("newViewButton"));
        newViewButton->setGeometry(QRect(40, 60, 171, 31));

        retranslateUi(dealDepthDlg);

        QMetaObject::connectSlotsByName(dealDepthDlg);
    } // setupUi

    void retranslateUi(QWidget *dealDepthDlg)
    {
        dealDepthDlg->setWindowTitle(QApplication::translate("dealDepthDlg", "Form", 0));
        addButton->setText(QApplication::translate("dealDepthDlg", "add Depth Image", 0));
        playButton->setText(QApplication::translate("dealDepthDlg", "Play Video", 0));
        newViewButton->setText(QApplication::translate("dealDepthDlg", "gen New View", 0));
    } // retranslateUi

};

namespace Ui {
    class dealDepthDlg: public Ui_dealDepthDlg {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DEALDEPTHDLG_H
