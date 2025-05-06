#include "main.hpp"

void YTPMVHomePage::setupHomeTab() {
    int topSamples;
    int recentSamples;

    try {
        recentSamples = api.recent_samples();
    } catch(std::exception & ex) {
        qDebug() << "Failed to get recent samples: " << ex.what();
    }

    try {
        topSamples = api.top_samples();
    } catch(std::exception & ex) {
        qDebug() << "Failed to get top samples: " << ex.what();
    }
}
