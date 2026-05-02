#include "AdminController.h"
#include "../services/LineupStore.h"
#include "../models/Lineup.h"
#include <drogon/drogon.h>
#include <drogon/MultiPart.h>
#include <filesystem>
#include <chrono>
#include <algorithm>

namespace fs = std::filesystem;

// ---- helpers ---------------------------------------------------------

static std::string safeExtension(const std::string& filename) {
    std::string ext = fs::path(filename).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".webp")
        return ext;
    return ".jpg";
}

static drogon::HttpResponsePtr jsonError(drogon::HttpStatusCode code,
                                          const std::string& msg) {
    Json::Value e;
    e["error"] = msg;
    auto r = drogon::HttpResponse::newHttpJsonResponse(e);
    r->setStatusCode(code);
    return r;
}

static Lineup lineupFromBody(const Json::Value& body) {
    Lineup l;
    l.map           = body.get("map",           "").asString();
    l.type          = body.get("type",          "smoke").asString();
    l.title         = body.get("title",         "").asString();
    l.description   = body.get("description",   "").asString();
    l.stance        = body.get("stance",        "standing").asString();
    l.throw_type    = body.get("throw_type",    "left-click").asString();
    l.position_img  = body.get("position_img",  "").asString();
    l.crosshair_img = body.get("crosshair_img", "").asString();
    l.video_url     = body.get("video_url",     "").asString();
    const Json::Value& tags = body["tags"];
    if (tags.isArray())
        for (const auto& t : tags) l.tags.push_back(t.asString());
    return l;
}

// ---- POST /api/upload -----------------------------------------------

void AdminController::uploadFile(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    drogon::MultiPartParser parser;
    if (parser.parse(req) != 0) {
        callback(jsonError(drogon::k400BadRequest, "failed to parse multipart request"));
        return;
    }

    auto& files = parser.getFiles();
    if (files.empty()) {
        callback(jsonError(drogon::k400BadRequest, "no file in request"));
        return;
    }

    const auto& file = files[0];
    std::string ext  = safeExtension(file.getFileName());

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::string filename = std::to_string(ms) + ext;

    // Ensure directory exists.
    fs::create_directories("./static/img/lineups");

    // saveAs: path prefixed with "./" is treated as an absolute-ish path,
    // not relative to app().getUploadPath().
    file.saveAs("./static/img/lineups/" + filename);

    Json::Value resp;
    resp["url"] = "/img/lineups/" + filename;
    callback(drogon::HttpResponse::newHttpJsonResponse(resp));
}

// ---- POST /api/lineups ----------------------------------------------

void AdminController::createLineup(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto body = req->getJsonObject();
    if (!body) {
        callback(jsonError(drogon::k400BadRequest, "invalid JSON"));
        return;
    }

    Lineup l = lineupFromBody(*body);

    if (l.map.empty() || l.title.empty()) {
        callback(jsonError(drogon::k400BadRequest, "map and title are required"));
        return;
    }

    LineupStore::instance().addLineup(l);

    Json::Value resp;
    resp["id"]    = l.id;
    resp["saved"] = true;
    callback(drogon::HttpResponse::newHttpJsonResponse(resp));
}

// ---- DELETE /api/lineups/{id} ---------------------------------------

void AdminController::deleteLineup(
    const drogon::HttpRequestPtr&,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    std::string id)
{
    if (!LineupStore::instance().deleteLineup(id)) {
        callback(jsonError(drogon::k404NotFound, "lineup not found"));
        return;
    }
    Json::Value resp;
    resp["deleted"] = true;
    callback(drogon::HttpResponse::newHttpJsonResponse(resp));
}

// ---- PUT /api/lineups/{id} ------------------------------------------

void AdminController::updateLineup(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    std::string id)
{
    auto body = req->getJsonObject();
    if (!body) {
        callback(jsonError(drogon::k400BadRequest, "invalid JSON"));
        return;
    }

    Lineup l  = lineupFromBody(*body);
    l.id      = id;

    if (l.map.empty() || l.title.empty()) {
        callback(jsonError(drogon::k400BadRequest, "map and title are required"));
        return;
    }

    if (!LineupStore::instance().updateLineup(l)) {
        callback(jsonError(drogon::k404NotFound, "lineup not found"));
        return;
    }

    Json::Value resp;
    resp["id"]    = l.id;
    resp["saved"] = true;
    callback(drogon::HttpResponse::newHttpJsonResponse(resp));
}
