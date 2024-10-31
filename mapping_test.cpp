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

#include <gtest/gtest.h>

using namespace am;

TEST(mapping, SortAndCoalesce) {
    std::vector<Mapping> maps = {
        {90000, 10},
        {1000, 50},
        {100, 900},
        {5000, 8000},
    };
    SortAndCoalesceMaps(maps);

    ASSERT_EQ(maps.size(), 3);
    EXPECT_EQ(maps[0].start, 100);
    EXPECT_EQ(maps[0].size, 950);
    EXPECT_EQ(maps[1].start, 5000);
    EXPECT_EQ(maps[1].size, 8000);
    EXPECT_EQ(maps[2].start, 90000);
    EXPECT_EQ(maps[2].size, 10);
}

TEST(mapping, FindSorted) {
    std::vector<Mapping> maps = {
        {90000, 10},
        {1000, 50},
        {100, 900},
        {5000, 8000},
    };
    SortAndCoalesceMaps(maps);

    EXPECT_EQ(FindAddressInSortedMap(maps, reinterpret_cast<void*>(10)), std::nullopt);
    EXPECT_EQ(FindAddressInSortedMap(maps, reinterpret_cast<void*>(100)),
              std::optional<size_t>{0});
    EXPECT_EQ(FindAddressInSortedMap(maps, reinterpret_cast<void*>(1050)), std::nullopt);
    EXPECT_EQ(FindAddressInSortedMap(maps, reinterpret_cast<void*>(5050)),
              std::optional<size_t>{1});
    EXPECT_EQ(FindAddressInSortedMap(maps, reinterpret_cast<void*>(90001)),
              std::optional<size_t>{2});
}

TEST(mapping, FindUnsorted) {
    std::vector<Mapping> maps = {
        {90000, 10},
        {1000, 50},
        {100, 900},
        {5000, 8000},
    };

    EXPECT_EQ(FindAddressInMap(maps, reinterpret_cast<void*>(10)), std::nullopt);
    EXPECT_EQ(FindAddressInMap(maps, reinterpret_cast<void*>(100)),
              std::optional<size_t>{2});
    EXPECT_EQ(FindAddressInMap(maps, reinterpret_cast<void*>(1050)), std::nullopt);
    EXPECT_EQ(FindAddressInMap(maps, reinterpret_cast<void*>(5050)),
              std::optional<size_t>{3});
    EXPECT_EQ(FindAddressInMap(maps, reinterpret_cast<void*>(90001)),
              std::optional<size_t>{0});
}
