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

#include <limits>

#include <gtest/gtest.h>

using namespace am;

class TestPlatform final : public IPlatform {
  public:
    TestPlatform() {
        maps_.emplace_back(Mapping{16384, 4096});
        maps_.emplace_back(Mapping{16384 + 4096, 4096});
        maps_.emplace_back(Mapping{32768, 4096});
    }

    int GetPageSize() override { return 4096; }

    bool GetAddressMapping(void* address, Mapping* map) override {
        auto it = FindAddressInMap(maps_, address);
        if (!it)
            return false;
        *map = maps_[it.value()];
        return true;
    }

    void ClearMappings() {
        maps_.clear();
    }

    void AddMapping(uintptr_t start, size_t size) {
        maps_.emplace_back(Mapping{start, size});
    }

  private:
    std::vector<Mapping> maps_;
};

class AddressDictTest : public ::testing::Test {
  protected:
    TestPlatform platform_;
    AddressDict ad_{&platform_};
};

TEST_F(AddressDictTest, Null) {
    ASSERT_EQ(ad_.Make32bitAddress(nullptr), std::optional<uint32_t>{0});
}

TEST_F(AddressDictTest, AddressNotFound) {
    ASSERT_EQ(ad_.Make32bitAddress(50), std::nullopt);
}

TEST_F(AddressDictTest, IdNotFound) {
    ASSERT_EQ(ad_.RecoverAddress(912734873), std::nullopt);
}

TEST_F(AddressDictTest, Basic) {
    void* address = reinterpret_cast<void*>(17000);
    auto id = ad_.Make32bitAddress(address);
    ASSERT_NE(id, std::nullopt);
    EXPECT_EQ(id.value(), 4711);
    EXPECT_EQ(ad_.RecoverAddress(id.value()), std::optional<void*>{address});
}

TEST_F(AddressDictTest, Arithmetic) {
    void* address = reinterpret_cast<void*>(16384);
    constexpr size_t offset = 4096 + 20;

    auto id = ad_.Make32bitAddress(address, offset);
    ASSERT_NE(id, std::nullopt);
    EXPECT_EQ(id.value(), 4095);

    uint32_t new_id = id.value() + offset;
    void* new_address = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(address) + offset);
    EXPECT_EQ(ad_.RecoverAddress(id.value(), offset), std::optional<void*>{address});
    EXPECT_EQ(ad_.RecoverAddress(new_id), std::optional<void*>{new_address});
}

static constexpr size_t kOffsetSlack = 1024 * 1024;

TEST_F(AddressDictTest, TruncateHugeRange) {
    platform_.AddMapping(65536, std::numeric_limits<size_t>::max() - kOffsetSlack);
    void* address = reinterpret_cast<void*>(65536);
    auto id = ad_.Make32bitAddress(address);
    EXPECT_NE(id, std::nullopt);
}

TEST_F(AddressDictTest, OutOfRanges) {
    if (sizeof(void*) == sizeof(uint32_t)) {
        GTEST_SKIP() << "Skipping 64-bit only test.";
    }

    platform_.AddMapping(65536, std::numeric_limits<size_t>::max() - kOffsetSlack);
    platform_.AddMapping(std::numeric_limits<size_t>::max() - kOffsetSlack, 4096);
    platform_.AddMapping(std::numeric_limits<size_t>::max() - kOffsetSlack + 8192, 4096);
    void* address = reinterpret_cast<void*>(std::numeric_limits<size_t>::max() - kOffsetSlack + 4096);
    auto id = ad_.Make32bitAddress(address);
    EXPECT_EQ(id, std::nullopt);
}

TEST_F(AddressDictTest, ExpandedRange) {
    platform_.ClearMappings();
    platform_.AddMapping(4096, 4096);
    platform_.AddMapping(8192, 8192);
    platform_.AddMapping(16384, 16384);

    auto id = ad_.Make32bitAddress(16384, 16384);
    ASSERT_NE(id, std::nullopt);
    EXPECT_EQ(id.value(), 4095);

    // This will create a new range that overlaps the previous one.
    auto id2 = ad_.Make32bitAddress(4096, 8192);
    ASSERT_NE(id2, std::nullopt);
    EXPECT_EQ(id2.value(), 20479);

    // Check that we can recover both addresses.
    EXPECT_EQ(ad_.RecoverAddressValue(id.value(), 16384), 16384);
    EXPECT_EQ(ad_.RecoverAddressValue(id.value() + 16384 - 1), 16384 + 16384 - 1);
    EXPECT_EQ(ad_.RecoverAddressValue(id2.value(), 8192), 4096);
    EXPECT_EQ(ad_.RecoverAddressValue(id2.value() + 8192 - 1), 4096 + 8192 - 1);
}

TEST(AddressDictPlatformText, StackVar) {
    AddressDict ad;

    auto id = ad.Make32bitAddress(&ad, sizeof(ad));
    ASSERT_NE(id, std::nullopt);
    EXPECT_NE(id.value(), 0);

    auto ptr = ad.RecoverAddress(id.value(), sizeof(ad));
    ASSERT_NE(ptr, std::nullopt);
    EXPECT_EQ(ptr, &ad);
}
