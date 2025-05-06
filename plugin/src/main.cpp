#include "main.hpp"
#include "ui_main.h"

YTPMVHomePage::YTPMVHomePage(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::YTPMVSDClient)
{
    ui->setupUi(this);

    setupHomeTab();

}

YTPMVHomePage::~YTPMVHomePage()
{
    delete ui;
}

int qMain(int x, char** y) {
    QApplication app(x,y);
    auto cli = new YTPMVHomePage();
    cli->show();
    return app.exec();
}
