#ifndef MAIN_H
#define MAIN_H

#include <QMainWindow>
#include "api.hpp"

QT_BEGIN_NAMESPACE
namespace Ui {
class YTPMVSDClient;
}
QT_END_NAMESPACE

class YTPMVHomePage : public QMainWindow
{
    Q_OBJECT

    API api;

public:
    void setupHomeTab();
    void setupSamplesTab();
    void setupSourcesTab();
    YTPMVHomePage(QWidget *parent = nullptr);
    ~YTPMVHomePage();

private:
    Ui::YTPMVSDClient *ui;
};
#endif // MAIN_H
