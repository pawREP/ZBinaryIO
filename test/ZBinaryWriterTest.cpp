#include "ZBinaryWriter.hpp"
#include "gtest/gtest.h"

using namespace ZBio;
using namespace ZBio::ZBinaryWriter;

namespace {


//template <typename Source>
//class BinaryWriterTest : public testing::Test {
//protected:
//    std::filesystem::path getTempFilePath() {
//        return std::filesystem::temp_directory_path() / "BinaryWriterTestFile.bin";
//    }
//
//    void SetUp() override {
//        const auto testFilePath = getTempFilePath();
//        if(std::filesystem::exists(testFilePath))
//            std::filesystem::remove(testFilePath);
//
//        if constexpr(std::is_same_v<Source, BufferSink>) {
//            br = std::make_unique<BinaryWriter>();
//        } else if constexpr(std::is_same_v<Source, FileSink>)
//            br = std::make_unique<BinaryWriter>(testFilePath);
//        else
//            throw; // unreachable;
//    }
//
//    void TearDown() override {
//        if constexpr(std::is_same_v<Source, FileSink>) {
//            const auto testFilePath = getTempFilePath();
//            if(std::filesystem::exists(testFilePath))
//                std::filesystem::remove(testFilePath);
//        }
//    }
//
//    std::unique_ptr<BinaryWriter> br;
//};
//
//#if GTEST_HAS_TYPED_TEST
//
//using testing::Types;
//
//typedef Types<FileSink, BufferSink> Implementations;
//
//TYPED_TEST_SUITE(BinaryWriterTest, Implementations);
//
//TYPED_TEST(BinaryWriterTest, Dummy) {
//    ASSERT_TRUE(false);
//}
//
//#endif // GTEST_HAS_TYPED_TEST

} // namespace