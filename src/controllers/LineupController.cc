#include "LineupController.h"
#include "../services/LineupStore.h"

// Map display metadata — id, human label, thumbnail path.
// Add entries here when new maps are seeded in data/lineups/.
static const std::vector<std::pair<std::string, std::string>> kMapMeta = {
    {"ancient",  "Ancient"},
    {"anubis",   "Anubis"},
    {"dust2",    "Dust II"},
    {"inferno",  "Inferno"},
    {"mirage",   "Mirage"},
    {"nuke",     "Nuke"},
    {"vertigo",  "Vertigo"},
};

void LineupController::getMaps(
    const drogon::HttpRequestPtr&,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto& store = LineupStore::instance();

    Json::Value arr(Json::arrayValue);
    for (auto& [id, label] : kMapMeta) {
        Json::Value m;
        m["id"]        = id;
        m["label"]     = label;
        m["thumbnail"] = "/img/maps/" + id + ".png";
        arr.append(m);
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(arr);
    resp->addHeader("Cache-Control", "public, max-age=60");
    callback(resp);
}

void LineupController::getLineups(
    const drogon::HttpRequestPtr&,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    std::string map)
{
    auto& store = LineupStore::instance();
    Json::Value result = store.mapToJson(map);

    if (result.isNull()) {
        Json::Value err;
        err["error"] = "map not found";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(drogon::k404NotFound);
        callback(resp);
        return;
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(result);
    resp->addHeader("Cache-Control", "public, max-age=300");
    callback(resp);
}
