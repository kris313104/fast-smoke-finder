#include "LineupStore.h"
#include <drogon/drogon.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <sstream>

namespace fs = std::filesystem;

LineupStore& LineupStore::instance() {
    static LineupStore inst;
    return inst;
}

// ---- normalise -------------------------------------------------------

std::string LineupStore::normalise(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s)
        out += static_cast<char>(std::tolower(c));
    return out;
}

// Split a string on any character in 'delims'.
static std::vector<std::string> splitOn(const std::string& s, const std::string& delims) {
    std::vector<std::string> parts;
    std::string cur;
    for (char c : s) {
        if (delims.find(c) != std::string::npos) {
            if (!cur.empty()) { parts.push_back(cur); cur.clear(); }
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) parts.push_back(cur);
    return parts;
}

// ---- load ------------------------------------------------------------

void LineupStore::loadFromDirectory(const std::string& dataDir) {
    dataDir_ = dataDir;
    fs::path dir(dataDir);
    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        LOG_WARN << "LineupStore: data directory not found: " << dataDir;
        return;
    }

    for (auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() != ".json") continue;

        std::ifstream f(entry.path());
        if (!f.is_open()) {
            LOG_WARN << "LineupStore: cannot open " << entry.path();
            continue;
        }

        Json::Value root;
        Json::CharReaderBuilder rb;
        std::string errs;
        if (!Json::parseFromStream(rb, f, &root, &errs)) {
            LOG_WARN << "LineupStore: JSON parse error in " << entry.path() << ": " << errs;
            continue;
        }

        if (!root.isArray()) {
            LOG_WARN << "LineupStore: expected JSON array in " << entry.path();
            continue;
        }

        std::string mapId = entry.path().stem().string();
        auto& vec = data_[mapId];
        vec.reserve(root.size());
        for (const auto& jl : root)
            vec.push_back(lineupFromJson(jl));

        LOG_INFO << "LineupStore: loaded " << vec.size() << " lineups for map '" << mapId << "'";
    }

    buildIndex();
    LOG_INFO << "LineupStore: index built for " << data_.size() << " map(s)";
}

// ---- index -----------------------------------------------------------

void LineupStore::indexLineup(const std::string& mapId, size_t idx) {
    auto& idxMap = tagIndex_[mapId];
    for (const auto& tag : data_[mapId][idx].tags) {
        std::string norm = normalise(tag);
        idxMap[norm].push_back(idx);
        for (const auto& part : splitOn(norm, "-"))
            if (part != norm) idxMap[part].push_back(idx);
    }
}

void LineupStore::buildIndex() {
    for (auto& [mapId, lineups] : data_)
        for (size_t i = 0; i < lineups.size(); ++i)
            indexLineup(mapId, i);
}

void LineupStore::rebuildMapIndex(const std::string& mapId) {
    tagIndex_[mapId].clear();
    auto it = data_.find(mapId);
    if (it == data_.end()) return;
    for (size_t i = 0; i < it->second.size(); ++i)
        indexLineup(mapId, i);
}

// ---- addLineup + persist ---------------------------------------------

std::string LineupStore::generateId(const std::string& map, const std::string& type) {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return map + "_" + type + "_" + std::to_string(ms);
}

void LineupStore::addLineup(Lineup& l) {
    if (l.id.empty())
        l.id = generateId(l.map, l.type);

    std::lock_guard<std::mutex> lock(mutex_);
    auto& vec = data_[l.map];
    size_t newIdx = vec.size();
    vec.push_back(l);
    indexLineup(l.map, newIdx);
    saveMap(l.map);
}

bool LineupStore::updateLineup(const Lineup& updated) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Search all maps for the matching id.
    for (auto& [mapId, vec] : data_) {
        for (size_t i = 0; i < vec.size(); ++i) {
            if (vec[i].id != updated.id) continue;

            if (mapId == updated.map) {
                vec[i] = updated;
                rebuildMapIndex(mapId);
                saveMap(mapId);
            } else {
                // Move to a different map.
                vec.erase(vec.begin() + static_cast<std::ptrdiff_t>(i));
                rebuildMapIndex(mapId);
                saveMap(mapId);

                auto& newVec = data_[updated.map];
                size_t newIdx = newVec.size();
                newVec.push_back(updated);
                indexLineup(updated.map, newIdx);
                saveMap(updated.map);
            }
            return true;
        }
    }
    return false;
}

bool LineupStore::deleteLineup(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [mapId, vec] : data_) {
        for (size_t i = 0; i < vec.size(); ++i) {
            if (vec[i].id != id) continue;
            vec.erase(vec.begin() + static_cast<std::ptrdiff_t>(i));
            rebuildMapIndex(mapId);
            saveMap(mapId);
            return true;
        }
    }
    return false;
}

void LineupStore::saveMap(const std::string& mapId) {
    if (dataDir_.empty()) return;
    auto it = data_.find(mapId);
    if (it == data_.end()) return;

    fs::path filePath = fs::path(dataDir_) / (mapId + ".json");
    Json::Value arr(Json::arrayValue);
    for (const auto& l : it->second)
        arr.append(lineupToJson(l));

    Json::StreamWriterBuilder wb;
    wb["indentCharacters"] = "  ";
    wb["commentStyle"]     = "None";

    std::ofstream f(filePath);
    if (!f.is_open()) {
        LOG_WARN << "LineupStore: cannot write " << filePath;
        return;
    }
    f << Json::writeString(wb, arr);
    LOG_INFO << "LineupStore: saved " << it->second.size()
             << " lineups for map '" << mapId << "' to " << filePath;
}

// ---- search ----------------------------------------------------------

std::vector<const Lineup*> LineupStore::search(
    const std::string& map,
    const std::vector<std::string>& tokens,
    const std::string& typeFilter) const
{
    auto dit = data_.find(map);
    if (dit == data_.end()) return {};
    const auto& lineups = dit->second;

    auto iit = tagIndex_.find(map);
    if (iit == tagIndex_.end()) return {};
    const auto& idx = iit->second;

    // Accumulate hit counts.
    std::unordered_map<size_t, int> scores;
    for (const auto& tok : tokens) {
        auto it = idx.find(tok);
        if (it == idx.end()) continue;
        for (size_t i : it->second)
            scores[i]++;
    }

    // Collect, filter, sort.
    std::vector<std::pair<int, size_t>> ranked; // (score, index)
    ranked.reserve(scores.size());
    for (auto& [i, score] : scores) {
        if (!typeFilter.empty() && lineups[i].type != typeFilter) continue;
        ranked.push_back({score, i});
    }

    std::sort(ranked.begin(), ranked.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });

    constexpr size_t kMaxResults = 50;
    std::vector<const Lineup*> results;
    results.reserve((std::min)(ranked.size(), kMaxResults));
    for (size_t k = 0; k < ranked.size() && k < kMaxResults; ++k)
        results.push_back(&lineups[ranked[k].second]);
    return results;
}

// ---- serialisation ---------------------------------------------------

Json::Value LineupStore::mapToJson(const std::string& map) const {
    auto it = data_.find(map);
    if (it == data_.end()) return Json::Value(Json::nullValue);

    Json::Value arr(Json::arrayValue);
    for (const auto& l : it->second)
        arr.append(lineupToJson(l));
    return arr;
}

std::vector<std::string> LineupStore::mapIds() const {
    std::vector<std::string> ids;
    ids.reserve(data_.size());
    for (auto& [id, _] : data_) ids.push_back(id);
    std::sort(ids.begin(), ids.end());
    return ids;
}

Json::Value LineupStore::lineupToJson(const Lineup& l) {
    Json::Value j;
    j["id"]            = l.id;
    j["map"]           = l.map;
    j["type"]          = l.type;
    j["title"]         = l.title;
    j["description"]   = l.description;
    j["stance"]        = l.stance;
    j["throw_type"]    = l.throw_type;
    j["position_img"]  = l.position_img;
    j["crosshair_img"] = l.crosshair_img;
    j["video_url"]     = l.video_url;
    Json::Value tags(Json::arrayValue);
    for (auto& t : l.tags) tags.append(t);
    j["tags"] = tags;
    return j;
}

Lineup LineupStore::lineupFromJson(const Json::Value& j) {
    Lineup l;
    l.id            = j.get("id",            "").asString();
    l.map           = j.get("map",           "").asString();
    l.type          = j.get("type",          "smoke").asString();
    l.title         = j.get("title",         "").asString();
    l.description   = j.get("description",   "").asString();
    l.stance        = j.get("stance",        "standing").asString();
    l.throw_type    = j.get("throw_type",    "left-click").asString();
    l.position_img  = j.get("position_img",  "").asString();
    l.crosshair_img = j.get("crosshair_img", "").asString();
    l.video_url     = j.get("video_url",     "").asString();
    const Json::Value& tags = j["tags"];
    if (tags.isArray())
        for (const auto& t : tags) l.tags.push_back(t.asString());
    return l;
}
