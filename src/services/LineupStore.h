#pragma once
#include "../models/Lineup.h"
#include <json/json.h>
#include <string>
#include <unordered_map>
#include <vector>

class LineupStore {
public:
    static LineupStore& instance();

    void loadFromDirectory(const std::string& dataDir);

    // OR-search: returns lineups for 'map' that match any token, sorted by hit count.
    std::vector<const Lineup*> search(
        const std::string& map,
        const std::vector<std::string>& tokens,
        const std::string& typeFilter) const;

    // Returns all lineups for the map serialised as a JSON array.
    // Returns Json::nullValue if the map is unknown.
    Json::Value mapToJson(const std::string& map) const;

    // Returns the list of all loaded map IDs.
    std::vector<std::string> mapIds() const;

private:
    LineupStore() = default;

    // map id → lineup list
    std::unordered_map<std::string, std::vector<Lineup>> data_;

    // map id → normalised tag → indices into data_[map]
    std::unordered_map<
        std::string,
        std::unordered_map<std::string, std::vector<size_t>>
    > tagIndex_;

    void buildIndex();
    static std::string normalise(const std::string& s);
    static Json::Value lineupToJson(const Lineup& l);
    static Lineup lineupFromJson(const Json::Value& j);
};
