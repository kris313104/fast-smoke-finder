#pragma once
#include <drogon/HttpController.h>

class AdminController : public drogon::HttpController<AdminController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(AdminController::createLineup, "/api/lineups",      drogon::Post);
        ADD_METHOD_TO(AdminController::updateLineup, "/api/lineups/{id}", drogon::Put);
        ADD_METHOD_TO(AdminController::deleteLineup, "/api/lineups/{id}", drogon::Delete);
        ADD_METHOD_TO(AdminController::uploadFile,   "/api/upload",       drogon::Post);
    METHOD_LIST_END

    void createLineup(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void updateLineup(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        std::string id);

    void deleteLineup(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        std::string id);

    void uploadFile(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};
