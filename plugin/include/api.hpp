#ifndef API_HPP
#define API_HPP

#include <string>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkCookie>
#include <QList>
#include <QEventLoop>
#include "json.hpp"

typedef void (*JSONCallback)();

enum class WaitState {
    None,
    Error,
    Fetching,
};

class API: QObject {
    Q_OBJECT

    WaitState mState;
    QString mResponse;
    QString mError;

    std::map<std::string, JSONCallback> mFunctions;

    QNetworkAccessManager * manager = new QNetworkAccessManager();
    QNetworkRequest *request = new QNetworkRequest();

    void do_request(std::string url, JSONCallback threadFunc);

public slots:
    void handleGetReply(QNetworkReply * reply);

public:
    API();
    int recent_samples();
    int top_samples();
    int samples();
    int metadata();
    int search_sources(std::string sort);
    int source_info(int id);
    int sample_info(int id);
    ~API();
};

#endif // API_HPP
