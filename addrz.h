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

#pragma once

#include <stdint.h>

#include <optional>
#include <vector>

#include "mapping.h"
#include "platform.h"

namespace am {

class AddressDict final {
  public:
    AddressDict(IPlatform* platform = nullptr);

    // Compress a pointer into a 32-bit value. Optionally, specify the number
    // of bytes to ensure are valid in the id. This is important to make sure
    // additional pages are included if needed, as not all platforms can return
    // a coalesced mapping.
    std::optional<uint32_t> Make32bitAddress(void* address, size_t nbytes = 0);

    // Recover a pointer from a 32-bit value. Optionally, validate that it is
    // possible to read |nbytes|. This is highly recommended to make sure ids
    // are not overflowed past their mapped range.
    std::optional<void*> RecoverAddress(uint32_t id, size_t nbytes = 0);

    std::optional<uint32_t> Make32bitAddress(uintptr_t address, size_t nbytes = 0) {
        return Make32bitAddress(reinterpret_cast<void*>(address), nbytes);
    }
    std::optional<uintptr_t> RecoverAddressValue(uint32_t id, size_t nbytes = 0) {
        if (auto val = RecoverAddress(id, nbytes); val)
            return {reinterpret_cast<uintptr_t>(val.value())};
        return {};
    }

  private:
    bool GetMapForAddress(uintptr_t address, size_t nbytes, Mapping* map);

    struct Range {
        Mapping map;
        uint32_t id;

        uint32_t range_end() const {
            return id + map.size;
        }

        bool operator <(const Range& other) const {
            return map < other.map;
        }
    };

    // Return an index into sorted_maps_.
    std::optional<size_t> FindRangeForAddress(uintptr_t address);
    std::optional<size_t> FindRangeForAddress(uintptr_t address, size_t nbytes);

    // Return an index into ranges_.
    std::optional<size_t> FindRangeForId(uint32_t id);

  private:
    IPlatform* platform_ = nullptr;
    uint32_t next_id_ = 0;
    std::vector<Range> ranges_;
    std::vector<Range> sorted_maps_;
};

} // namespace am
