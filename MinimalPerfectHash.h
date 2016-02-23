#pragma once

#include <vector>
#include <utility>
#include <hash_set>
#include <algorithm>

// Minimal Perfect Hash implementation
// http://cmph.sourceforge.net/bdz.html

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

    MinimalPerfectHash(const Vector& content) {
        size_t m = (content.size() * 4) / 3 + 1;
        while (HasCollision(content, m / 3)) {
            m += 3;
        }

        m3_ = m / 3;
        data_.resize(m);
        TripleHashes hashes = FillTripleHashes(content, m3_);

        std::vector<std::vector<size_t>> edges(m);
        std::vector<size_t> nodeDegree(m);
        std::vector<std::hash_set<size_t>> degree2vertexes(m);

        auto reduceDegree = [&](size_t u, size_t edge) {
            size_t tmpDegree = nodeDegree[u];
            degree2vertexes[tmpDegree].erase(u);
            --nodeDegree[u];
            --tmpDegree;
            degree2vertexes[tmpDegree].insert(u);
            edges[u].erase(edge);
        };

        auto deleteEdge = [&](size_t edge) {
            reduceDegree(hashes[edge].hash0_, edge);
            reduceDegree(hashes[edge].hash1_, edge);
            reduceDegree(hashes[edge].hash2_, edge);
        };

        for (size_t i = 0; i < hashes.size(); ++i) {
            edges[hashes[i].hash0_].emplace_back(i);
            edges[hashes[i].hash1_].emplace_back(i);
            edges[hashes[i].hash2_].emplace_back(i);
            ++nodeDegree[hashes[i].hash0_];
            ++nodeDegree[hashes[i].hash1_];
            ++nodeDegree[hashes[i].hash2_];
        }
        for (size_t i = 0; i < m; ++i) {
            degree2vertexes[ nodeDegree[i] ].insert(i);
        }

        std::vector<size_t> deleteOrder;

        for (size_t i = 0; i < hashes.size(); ++i) {
            size_t u;
            hash_set<size_t>::iterator toV = degree[1].begin();
            if (toV != degree[1].end()) {
                u = *toV;
            } else {
                throw std::exception("unexpected graph");
            }
            size_t e = *edges[u].begin();
            deleteEdge(e);
            deleteOrder.emplace_back(e);
        }

        std::vector<bool> visited(m);
        for (size_t i = 0; i < m; ++i) {
            data_[i].g_ = 3;
        }

        for (ssize_t i = (ssize_t)content.size() - 1; i >= 0; --i) {
            size_t u = deleteOrder[i];
            char choice = 0;
            for (int j = 0; j < 3; ++j) {
                if (!visited[hashes[u][j]]) {
                    choice = j;
                    break;
                }
            }
            size_t hashSum = 0;
            for (size_t j = 0; j < 3; ++j) {
                hashSum += data_[hashes[u][j]].g_;
            }
            data_[hashes[u][choice]].g_ = ((choice - hashSum) % 3 + 3) % 3;
            visited[hashes[u][choice]] = true;
        }
    }

    bool Lookup(const K& key, V* value) const {
        TripleHash hash = FillHash(key, m3_);
        size_t gIndex = data_[hash.hash0_].g_ + data_[hash.hash1_].g_ + data_[hash.hash2_].g_;
        size_t index = hash[gIndex % 3];
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

        size_t operator[](size_t index) {
            if (0 == index) {
                return hash0_;
            }
            if (1 == index) {
                return hash1_;
            }
            if (2 == index) {
                return hash2_;
            }
            throw std::exception("unexpected index");
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

    static TripleHash FillHash(const K& key, size_t m) {
        TripleHash result;
        result.hash0_ = hasher0_(K) % m;
        result.hash1_ = (hasher1_(K) % m) + m;
        result.hash2_ = (hasher2_(K) % m) + 2*m;
        return std::move(result);
    }

    static TripleHashes FillTripleHashes(const Vector& content, size_t m) {
        std::vector<TripleHash> hashes(content.size());
        for (size_t i = 0; i < content.size(); ++i) {
            hashes[i] = FillHash(content[i].key_, m);
        }
        return std::move(hashes);
    }

    static bool HasCollision(const Vector& content, size_t m) {
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
