#include "ZBinaryWriter.hpp"
#include "gtest/gtest.h"

using namespace ZBio;
using namespace ZBio::ZBinaryWriter;

namespace {

template <typename Source>
class BinaryWriterTest : public testing::Test {
private:
    std::filesystem::path tmpFile;

    void printVector(const std::vector<char>& v) {
        std::stringstream ss;
        for(const auto c : v)
            ss << std::hex << std::setw(2) << std::setfill('0') << std::uppercase
               << static_cast<const int>(c) << ", ";
        printf("%s\n", ss.str().c_str());
    }

    bool vectorEq(const std::vector<char>& v0, const std::vector<char>& v1) {
        printVector(v0);
        printVector(v1);

        if(v0.size() != v1.size())
            return false;
        if(memcmp(v0.data(), v1.data(), v0.size()))
            return false;
        return true;
    }

protected:
    void SetUp() override {
        this->tmpFile = std::filesystem::temp_directory_path() / "ZBinaryWriterTmpFile.bin";
        if constexpr(std::is_same_v<Source, BufferSink>) {
            this->br = std::make_unique<BinaryWriter>();
        } else if constexpr(std::is_same_v<Source, FileSink>)
            this->br = std::make_unique<BinaryWriter>(this->tmpFile);
        else
            throw; // unreachable;
    }

    void TearDown() override {
        if constexpr(std::is_same_v<Source, FileSink>) {
            this->br->release();
            if(std::filesystem::exists(this->tmpFile))
                std::filesystem::remove(this->tmpFile);
        }
    }

    bool validate(const std::vector<char>& soll) {
        std::vector<char> is;
        if constexpr(std::is_same_v<Source, FileSink>) {
            this->br->release();

            const auto fileSize = std::filesystem::file_size(this->tmpFile);
            is.resize(fileSize);

            std::ifstream ifs(this->tmpFile, std::ios::binary);
            ifs.read(is.data(), fileSize);

            ifs.close();
        } else if constexpr(std::is_same_v<Source, BufferSink>) {
            is = this->br->release().value();
        } else
            throw;

        return vectorEq(is, soll);
    }


    std::unique_ptr<BinaryWriter> br;
};

#if GTEST_HAS_TYPED_TEST

using testing::Types;

typedef Types<FileSink, BufferSink> Implementations;

TYPED_TEST_SUITE(BinaryWriterTest, Implementations);

TYPED_TEST(BinaryWriterTest, SeekTell) {
    constexpr int offset0 = 0x06;
    constexpr int offset1 = 0x03;
    constexpr int offset2 = 0x07;
    constexpr int offset3 = 0x05;

    // test tell on construction
    ASSERT_EQ(this->br->tell(), 0);

    // seek forward
    this->br->seek(offset0);
    ASSERT_EQ(this->br->tell(), offset0);

    // seek backwards
    this->br->seek(offset1);
    ASSERT_EQ(this->br->tell(), offset1);

    // seek to end - 1
    this->br->seek(offset3);
    ASSERT_EQ(this->br->tell(), offset3);

    // seek to end
    this->br->seek(offset0);
    ASSERT_EQ(this->br->tell(), offset0);

    // seek to end + 1
    this->br->seek(offset2);
    ASSERT_EQ(this->br->tell(), offset2);

    // TODO: Seek negative

    ASSERT_TRUE(this->validate(std::vector<char>(offset2, '\0')));
}

TYPED_TEST(BinaryWriterTest, WriteTrivial) {
    this->br->write(0x11223344);
    this->br->template write<char>(0x66);
    ASSERT_TRUE(this->validate(std::vector<char>{ 0x44, 0x33, 0x22, 0x11, 0x66 }));
}

TYPED_TEST(BinaryWriterTest, WriteTrivialBE) {
    this->br->template write<int, Endianness::BE>(0x11223344);
    this->br->template write<char>(0x66);
    ASSERT_TRUE(this->validate(std::vector<char>{ 0x11, 0x22, 0x33, 0x44, 0x66 }));
}

TYPED_TEST(BinaryWriterTest, WriteCompound) {
    struct T {
        int a = 0x11223344;
        int b = 0x12233445;
    } t;
    this->br->write(t);
    ASSERT_TRUE(this->validate(std::vector<char>{ 0x44, 0x33, 0x22, 0x11, 0x45, 0x34, 0x23, 0x12 }));
}

TYPED_TEST(BinaryWriterTest, WriteCharArray) {
    const std::vector<char> data{ 0x44, 0x33, 0x22, 0x11, 0x66 };
    this->br->write(data.data(), data.size());
    ASSERT_TRUE(this->validate(data));
}

TYPED_TEST(BinaryWriterTest, WriteNonCharArrayLE) {
    const std::vector<int> srcData{ 0x11223344, 0x12233445 };
    this->br->write(srcData.data(), srcData.size());
    ASSERT_TRUE(this->validate(std::vector<char>{ 0x44, 0x33, 0x22, 0x11, 0x45, 0x34, 0x23, 0x12 }));
}

TYPED_TEST(BinaryWriterTest, WriteNonCharArrayBE) {
    const std::vector<int> srcData{ 0x11223344, 0x12233445 };
    this->br->template write<int, Endianness::BE>(srcData.data(), srcData.size());
    ASSERT_TRUE(this->validate(std::vector<char>{ 0x11, 0x22, 0x33, 0x44, 0x12, 0x23, 0x34, 0x45 }));
}

TYPED_TEST(BinaryWriterTest, SeekBackAndOverride) {
    this->br->seek(4);
    this->br->template write<char>(0x66);
    this->br->seek(0);
    this->br->write(0x11223344);
    ASSERT_TRUE(this->validate(std::vector<char>{ 0x44, 0x33, 0x22, 0x11, 0x66 }));
}

TYPED_TEST(BinaryWriterTest, AlignAlreadyAligned) {
    this->br->align();
    ASSERT_TRUE(this->validate(std::vector<char>{}));
}

TYPED_TEST(BinaryWriterTest, Align) {
    this->br->seek(1);
    this->br->align();
    ASSERT_TRUE((this->validate(std::vector<char>(0x10, '\0'))));
}

TYPED_TEST(BinaryWriterTest, WriteStrings) {
    this->br->writeString("Test");
    this->br->template writeString<Endianness::LE>("Test");
    this->br->writeCString("Test");
    this->br->template writeCString<Endianness::LE>("Test");

    const std::vector<char> soll{ 0x54, 0x65, 0x73, 0x74, 0x74, 0x73, 0x65, 0x54, 0x54,
                                  0x65, 0x73, 0x74, 0x00, 0x74, 0x73, 0x65, 0x54, 0x00 };

    ASSERT_TRUE(this->validate(soll));
}

#endif // GTEST_HAS_TYPED_TEST

} // namespace