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

#include "platform.h"

#include <iostream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "proc_maps.h"

namespace am {

class WindowsPlatform final : public IPlatform {
  public:
    int GetPageSize() override {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        return si.dwPageSize;
    }
    bool GetAddressMapping(void* address, Mapping* map) override {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(address, &mbi, sizeof(mbi)) == 0)
            return false;

        if (mbi.State == MEM_FREE)
            return false;

        uintptr_t base_address = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        uintptr_t alloc_base = reinterpret_cast<uintptr_t>(mbi.AllocationBase);

        map->start = alloc_base;
        map->size = (base_address + mbi.RegionSize) - alloc_base;
        return true;
    }
};

IPlatform* IPlatform::GetDefault() {
    static WindowsPlatform sPlatform;
    return &sPlatform;
}

} // namespace am

