#include <drogon/drogon.h>
#include "services/LineupStore.h"

int main() {
    LineupStore::instance().loadFromDirectory("./data/lineups");
    drogon::app()
        .loadConfigFile("./config.json")
        .run();
    return 0;
}
