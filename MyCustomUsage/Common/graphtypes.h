#pragma once

#include <string>

#include "types.h"

namespace my {

inline std::string node_url(NodeOsmId id) {
    return std::string("https://www.openstreetmap.org/node/") + std::to_string(id);
}
using NodeId = std::string;

struct Node {
    inline Node(NodeOsmId osm_id_, osmium::Location const& location_)
        : url{node_url(osm_id_)},
          id{url},  // for OSM nodes, node ids are their URL
          location{location_} {}

    inline Node(NodeId id, osmium::Location const& location_)
        : url{},
          id{id},
          location{location_} {}

    inline double lon() const { return location.lon(); }
    inline double lat() const { return location.lat(); }

    inline bool operator==(Node const& other) const { return this->id == other.id; }  // needed by set<T>

    std::string url;
    NodeId id;
    osmium::Location location;
};

struct NodeHasher {
    size_t operator()(Node const& n) const { return std::hash<std::string>{}(n.id); }
};

// NOTE : a single OSM way can be splitted in several edges.
struct Edge {
    inline Edge(NodeOsmId node_from_, NodeOsmId node_to_, Polyline&& geometry_, float length_m_, float weight_)
        : node_from{node_from_, geometry_.front()},
          node_to{node_to_, geometry_.back()},
          length_m{length_m_},
          weight{weight_},
          geometry{geometry_} {}

    inline Edge(NodeId node_from_, NodeId node_to_, Polyline&& geometry_, float length_m_, float weight_)
        : node_from{node_from_, geometry_.front()},
          node_to{node_to_, geometry_.back()},
          length_m{length_m_},
          weight{weight_},
          geometry{geometry_} {}

    Node node_from;
    Node node_to;
    float length_m;
    float weight;
    Polyline geometry;
};

}
