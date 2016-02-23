#include <cstdlib>
#include <iostream>

#include "MinimalPerfectHash.h"

struct Hash0 {
    static uint64_t operator()(uint64_t value) {
        return ((value << 7) + 11) ^ (value >> 57) ^ value;
    }
};

struct Hash1 {
    static uint64_t operator()(uint64_t value) {
        return ((value << 13) + 23) ^ (value >> 57) ^ value;
    }
};

struct Hash2 {
    static uint64_t operator()(uint64_t value) {
        return ((value << 17) + 113) ^ (value >> 47) ^ value;
    }
};

int main() {
    using MPH = MinimalPerfectHash<uint64_t, uint64_t, Hash0, Hash1, Hash2>;

    using Data = MPH::Vector;

    static const size_t N = 10000000;

    Data data(N);
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = std::make_pair( rand(), rand() );
    }

    MPH mph(data);

    return 0;
}
