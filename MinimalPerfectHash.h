#pragma once

#include <algorithm>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/intrusive/list.hpp>

// Minimal Perfect Hash implementation
// http://cmph.sourceforge.net/bdz.html
// http://cmph.sourceforge.net/papers/wads07.pdf

template<typename K, typename V, typename Hash0, typename Hash1, typename Hash2>
class MinimalPerfectHash {
public:
    using Pair = std::pair<K, V>;
    using Vector = std::vector<Pair>;

#pragma pack(push, 1)
    struct Data {
        K key_;
        V value_;
        char g_;
    };
#pragma pack(pop)

    using GraphId = uint32_t;

    struct GraphVertex : public boost::intrusive::list_base_hook<> {
        GraphId id_;
        GraphId degree_;
    };

    struct GraphEdge : public boost::intrusive::list_base_hook<> {
        GraphId id_;
    };

    MinimalPerfectHash(const Vector& content, const K& invalidKey) {
        size_t m = ((content.size() * 4) / 9 + 1) * 3;
        while (HasCollision(content, m / 3)) {
            m += 3;
        }

        m3_ = m / 3;
        const TripleHashes hashes = FillTripleHashes(content, m3_);

        std::vector<size_t> deleteOrder(content.size());

        {
            std::vector<GraphVertex> vertexes(m);
            for (GraphId v = 0; v < m; ++v) {
                vertexes[v].id_ = v;
                vertexes[v].degree_ = 0;
            }

            std::vector<GraphEdge> edges0(content.size());
            std::vector<GraphEdge> edges1(content.size());
            std::vector<GraphEdge> edges2(content.size());
            for (GraphId e = 0; e < content.size(); ++e) {
                edges0[e].id_ = e;
                edges1[e].id_ = e;
                edges2[e].id_ = e;
            }

            using GraphEdges = boost::intrusive::list<GraphEdge>;
            std::vector<GraphEdges> vertexesToEdges(m);
            using GraphVertexes = boost::intrusive::list<GraphVertex>;
            std::vector<GraphVertexes> degreeToVertexes(content.size());

            auto addEdge = [&](GraphId v, GraphEdge& e) {
                vertexesToEdges[v].push_back(e);
                ++vertexes[v].degree_;
            };

            for (size_t i = 0; i < hashes.size(); ++i) {
                addEdge(hashes[i].hash0_, edges0[i]);
                addEdge(hashes[i].hash1_, edges1[i]);
                addEdge(hashes[i].hash2_, edges2[i]);
            }
            for (size_t i = 0; i < m; ++i) {
                degreeToVertexes[ vertexes[i].degree_ ].push_back(vertexes[i]);
            }

            auto reduceDegree = [&](GraphId v, GraphEdge& e) {
                GraphId tmpDegree = vertexes[v].degree_;
                degreeToVertexes[tmpDegree].erase( GraphVertexes::s_iterator_to(vertexes[v]) );
                --vertexes[v].degree_;
                --tmpDegree;
                degreeToVertexes[tmpDegree].push_back(vertexes[v]);
                vertexesToEdges[v].erase( GraphEdges::s_iterator_to(e) );
            };

            auto deleteEdge = [&](GraphId e) {
                reduceDegree(hashes[e].hash0_, edges0[e]);
                reduceDegree(hashes[e].hash1_, edges1[e]);
                reduceDegree(hashes[e].hash2_, edges2[e]);
            };

            for (size_t i = 0; i < hashes.size(); ++i) {
                if (degreeToVertexes[1].empty()) {
                    throw std::runtime_error("unexpected graph");
                }
                GraphId u = degreeToVertexes[1].begin()->id_;
                if (1 != vertexesToEdges[u].size()) {
                    throw std::runtime_error("graph invariant is busted 1");
                }
                GraphEdge& e = vertexesToEdges[u].front();
                deleteEdge(e.id_);
                deleteOrder[i] = e.id_;
                if (0 != vertexesToEdges[u].size()) {
                    throw std::runtime_error("graph invariant is busted 0");
                }
            }
        }

        data_.resize(m);
        for (auto& item: data_) {
            item.g_ = 3;
        }

        std::vector<bool> visited(m);
        for (ssize_t i = (ssize_t)content.size() - 1; i >= 0; --i) {
            size_t u = deleteOrder[i];
            char choice = 0;
            while ((choice < 3) && visited[hashes[u][choice]]) {
                ++choice;
            }
            if (3 == choice) {
                throw std::runtime_error("all visited");
            }
            char hashSum = 0;
            for (size_t j = 0; j < 3; ++j) {
                hashSum += data_[hashes[u][j]].g_;
            }
            data_[hashes[u][choice]].g_ = (9 + choice - hashSum) % 3;
            for (size_t j = 0; j < 3; ++j) {
                visited[hashes[u][j]] = true;
            }
        }

        for (size_t i = 0; i < content.size(); ++i) {
            const auto& item = content[i];
            auto index = GetIndex(item.first);
            data_[index].key_ = item.first;
            data_[index].value_ = item.second;
        }
    }

    bool Lookup(const K& key, V* value) const {
        auto index = GetIndex(key);
        if (data_[index].key_ == key) {
            *value = data_[index].value_;
            return true;
        } else {
            return false;
        }
    }

private:
    struct TripleHash {
        size_t hash0_;
        size_t hash1_;
        size_t hash2_;

        size_t operator[](size_t index) const {
            if (0 == index) {
                return hash0_;
            }
            if (1 == index) {
                return hash1_;
            }
            if (2 == index) {
                return hash2_;
            }
            throw std::runtime_error("unexpected index");
        }

        bool operator<(const TripleHash& rhs) const {
            if (hash0_ != rhs.hash0_) {
                return hash0_ < rhs.hash0_;
            }
            if (hash1_ != rhs.hash1_) {
                return hash1_ < rhs.hash1_;
            }
            if (hash2_ != rhs.hash2_) {
                return hash2_ < rhs.hash2_;
            }
            return false;
        }

        bool operator==(const TripleHash& rhs) const {
            return (hash0_ == rhs.hash0_) && (hash1_ == rhs.hash1_) && (hash2_ == rhs.hash2_);
        }
    };
    using TripleHashes = std::vector<TripleHash>;

    size_t GetIndex(const K& key) const {
        TripleHash hash = FillHash(key, m3_);
        size_t gIndex = data_[hash.hash0_].g_ + data_[hash.hash1_].g_ + data_[hash.hash2_].g_;
        size_t index = hash[gIndex % 3];
        return index;
    }

    TripleHash FillHash(const K& key, size_t m) const {
        TripleHash result;
        result.hash0_ = hasher0_(key) % m;
        result.hash1_ = (hasher1_(key) % m) + m;
        result.hash2_ = (hasher2_(key) % m) + 2*m;
        return std::move(result);
    }

    TripleHashes FillTripleHashes(const Vector& content, size_t m) const {
        TripleHashes hashes(content.size());
        for (size_t i = 0; i < content.size(); ++i) {
            hashes[i] = FillHash(content[i].first, m);
        }
        return std::move(hashes);
    }

    bool HasCollision(const Vector& content, size_t m) const {
        TripleHashes hashes = FillTripleHashes(content, m);
        std::sort(hashes.begin(), hashes.end());
        for (size_t i = 1; i < hashes.size(); ++i) {
            if (hashes[i - 1] == hashes[i]) {
                return true;
            }
        }
        return false;
    }

    using DataVector = std::vector<Data>;
    DataVector data_;
    size_t m3_;
    Hash0 hasher0_;
    Hash1 hasher1_;
    Hash2 hasher2_;
};
