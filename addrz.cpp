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

#include "addrz.h"

#include <assert.h>

#include <limits>

#include <amtl/am-bits.h>
#include "platform.h"

namespace am {

AddressDict::AddressDict(IPlatform* platform)
  : platform_(platform)
{
    if (!platform_)
        platform_ = IPlatform::GetDefault();

    // Start at the first valid page.
    next_id_ = platform_->GetPageSize() - 1;
    assert(next_id_ != 0);
}

std::optional<uint32_t> AddressDict::Make32bitAddress(void* address, size_t nbytes) {
    if (address == nullptr)
        return {0};

    Range range;

    uintptr_t value = reinterpret_cast<uintptr_t>(address);
    auto it = FindRangeForAddress(value, nbytes);
    if (it) {
        range = sorted_maps_[it.value()];
    } else {
        // No existing range found, make a new one.
        if (!GetMapForAddress(value, nbytes, &range.map))
            return {};
        if (range.map.size > std::numeric_limits<uint32_t>::max() ||
            !ke::IsUint32AddSafe(next_id_, range.map.size))
        {
            // Can we truncate the range to make room?
            uint32_t remaining = std::numeric_limits<uint32_t>::max() - next_id_;
            if (!remaining || remaining < value - range.map.start)
                return {};
            range.map.size = remaining;
        }

        // Reserve IDs for this mapping.
        range.id = next_id_;
        next_id_ += range.map.size;

        ranges_.emplace_back(range);

        // Resort the addr -> id table for fast lookup.
        sorted_maps_.emplace_back(range);
        std::sort(sorted_maps_.begin(), sorted_maps_.end());
    }

    assert(range.map.owns(address));

    if (!ke::IsUint32AddSafe(range.id, (value - range.map.start)))
        return {};
    return {range.id + uint32_t(value - range.map.start)};
}

std::optional<void*> AddressDict::RecoverAddress(uint32_t id, size_t nbytes) {
    auto r = FindRangeForId(id);
    if (!r)
        return {};

    const auto& range = ranges_[r.value()];
    uintptr_t offset = id - range.id;
    uintptr_t address = range.map.start + offset;
    if (address + nbytes > range.map.end())
        return {};
    return {reinterpret_cast<void*>(address)};
}

// If the user requests multiple pages, they may cross multiple mappings. We
// want to combine them into one contiguous range so that pointer arithmetic
// works as much as possible.
bool AddressDict::GetMapForAddress(uintptr_t address, size_t nbytes, Mapping* map) {
    if (!platform_->GetAddressMapping(reinterpret_cast<void*>(address), map))
        return false;

    while (true) {
        size_t offset_in_map = address - map->start;
        size_t max_read = map->size - offset_in_map;
        if (max_read >= nbytes)
            break;

        Mapping next;
        if (!platform_->GetAddressMapping(reinterpret_cast<void*>(map->end()), &next))
            return false;

        assert(next.start <= map->end());
        assert(next.end() > map->end());

        map->size += next.end() - map->end();
    }
    return true;
}

std::optional<size_t> AddressDict::FindRangeForId(uint32_t id) {
    size_t lower = 0;
    size_t upper = ranges_.size();
    while (lower < upper) {
        size_t mid = (lower + upper) / 2;
        const auto& range = ranges_[mid];
        if (id < range.id) {
            upper = mid;
        } else if (id >= range.range_end()) {
            lower = mid + 1;
        } else {
            assert(id >= range.id && id < range.range_end());
            return {mid};
        }
    }
    return {};
}

std::optional<size_t> AddressDict::FindRangeForAddress(uintptr_t address) {
    size_t lower = 0;
    size_t upper = sorted_maps_.size();
    while (lower < upper) {
        size_t mid = (lower + upper) / 2;
        const auto& range = sorted_maps_[mid];
        if (address < range.map.start) {
            upper = mid;
        } else if (address >= range.map.end()) {
            lower = mid + 1;
        } else {
            assert(address >= range.map.start && address < range.map.end());
            return {mid};
        }
    }
    return {};
}

std::optional<size_t> AddressDict::FindRangeForAddress(uintptr_t address, size_t nbytes) {
    auto r = FindRangeForAddress(address);
    if (!r || nbytes <= 1)
        return r;

    if (!sorted_maps_[r.value()].map.owns(address + nbytes - 1))
        return {};
    return {};
}

} // namespace am
