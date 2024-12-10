#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_Twisting.h"
#include <memory>

class QCefView;
class Twisting : public QMainWindow
{
    Q_OBJECT

public:
    Twisting(QWidget *parent = nullptr);
    ~Twisting();

private:
    Ui::TwistingClass ui;

    /**
     * ä¯ÀÀÆ÷.
     */
    std::unique_ptr < QCefView> cefViewWidget_ = nullptr;
};
