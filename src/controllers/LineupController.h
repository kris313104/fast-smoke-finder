#pragma once
#include <drogon/HttpController.h>

class LineupController : public drogon::HttpController<LineupController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(LineupController::getMaps,    "/api/maps",          drogon::Get);
        ADD_METHOD_TO(LineupController::getLineups, "/api/lineups/{map}", drogon::Get);
    METHOD_LIST_END

    void getMaps(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void getLineups(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        std::string map);
};
