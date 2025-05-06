#include "api.hpp"




API::API() {
    qDebug() << "initalizing api";
}

void API::do_request(std::string url, JSONCallback resultFunc) {
    mFunctions[url] = resultFunc;

    request->setUrl(QUrl(url.c_str()));
    auto reply = manager->get(*request);

    connect(manager,
            SIGNAL(finished(QNetworkReply *)),
            this,
            SLOT(handleGetReply(QNetworkReply *)));
}

void API::handleGetReply(QNetworkReply * reply) {
    if (reply->error()) {
        qDebug() << reply->errorString();
        return;
    }

    mFunctions[reply->url().toString().toStdString()]();
    auto wh = reply->readAll().toStdString();

    if(wh == "") {
        qDebug() << "empty";
        return;
    }

    nlohmann::json obj = nlohmann::json::parse(wh);

    qDebug() << obj.dump();
};

void recentSamplesFunc() {
    qDebug() << "got recent samples";
};
void topSamplesFunc() {
    qDebug() << "got top samples";
};

int API::recent_samples() {
    this->do_request("http://192.168.1.201:5000/api/recent_samples", recentSamplesFunc);
    return 0;
};
int API::top_samples() {
    this->do_request("http://192.168.1.201:5000/api/top_samples", topSamplesFunc);
    return 0;
};
int API::samples() {
    // mRequest->setUrl(QUrl("http://192.168.1.201:5000/api/samples"));

    return 0;
};
int API::metadata() {
    // mRequest->setUrl(QUrl("http://192.168.1.201:5000/api/metadata"));

    return 0;
};
int API::search_sources(std::string sort) {
    // mRequest->setUrl(QUrl("http://192.168.1.201:5000/api/search_sources"));

    return 0;
};
int API::source_info(int id) {
    // mRequest->setUrl(QUrl(std::format("http://192.168.1.201:5000/api/source/{}",id).c_str()));

    return 0;
};
int API::sample_info(int id) {
    // mRequest->setUrl(QUrl(std::format("http://192.168.1.201:5000/api/sample/{}",id).c_str()));

    return 0;
};

API::~API() {

}
