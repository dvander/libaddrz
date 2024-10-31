// vim: set sts=8 ts=4 sw=4 tw=99 et:
//
// Copyright (C) 2024, David Anderson and AlliedModders LLC
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//  * Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//  * Neither the name of AlliedModders LLC nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "mapping.h"

#include <assert.h>

#include <algorithm>

namespace am {

void SortAndCoalesceMaps(std::vector<Mapping>& maps) {
    std::sort(maps.begin(), maps.end());

    size_t cursor = 0;
    for (size_t i = 1; i < maps.size(); i++) {
        if (maps[i].start == maps[cursor].end()) {
            maps[cursor].size += maps[i].size;
        } else {
            maps[++cursor] = maps[i];
        }
    }
    maps.resize(cursor + 1);
}

std::optional<size_t> FindAddressInSortedMap(const std::vector<Mapping>& maps, void* address) {
    uintptr_t value = reinterpret_cast<uintptr_t>(address);

    size_t lower = 0;
    size_t upper = maps.size();
    while (lower < upper) {
        size_t mid = (lower + upper) / 2;
        const auto& map = maps[mid];
        if (value < map.start) {
            upper = mid;
        } else if (value >= map.end()) {
            lower = mid + 1;
        } else {
            assert(value >= map.start && value < map.end());
            return {mid};
        }
    }
    return {};
}

std::optional<size_t> FindAddressInMap(const std::vector<Mapping>& maps, void* address) {
    uintptr_t value = reinterpret_cast<uintptr_t>(address);
    for (size_t i = 0; i < maps.size(); i++) {
        if (value >= maps[i].start && value < maps[i].end())
            return {i};
    }
    return {};
}

} // namespace am
