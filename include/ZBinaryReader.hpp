#include <algorithm>
#include <assert.h>
#include <cstring>
#include <fstream>
#include <memory>
#include <vector>
#include <cstdint>
#include <filesystem>

// Check if system is little endian
static_assert(static_cast<const uint8_t&>(0x0B00B135) == 0x35);

namespace ZBinaryReader {

class ISource {
public:
    virtual void read(char* dst, int64_t len) = 0;
    virtual void peek(char* dst, int64_t len) const = 0;
    virtual void seek(int64_t offset) = 0;
    virtual int64_t tell() const noexcept = 0;
    virtual int64_t size() const noexcept = 0;

    virtual ~ISource(){};
};

class FileSource : public ISource {
private:
    int64_t size_;
    mutable std::ifstream ifs;

public:
    explicit FileSource(const std::string& path);
    explicit FileSource(const char* path);
    explicit FileSource(const std::filesystem::path& path);

    ~FileSource();

    void read(char* dst, int64_t len) override;
    void peek(char* dst, int64_t len) const override;
    void seek(int64_t offset) override final;
    [[nodiscard]] int64_t tell() const noexcept override final;
    [[nodiscard]] int64_t size() const noexcept override final;
};

class BufferSource : public ISource {
private:
    std::unique_ptr<char[]> ownedBuffer;

    const char* buffer;
    const int64_t bufferSize;
    int64_t cur;

public:
    // Non owning constructor
    BufferSource(const char* data, int64_t data_size);
    // Owning constructor
    BufferSource(std::unique_ptr<char[]> data, int64_t data_size);

    void read(char* dst, int64_t len) override;
    void peek(char* dst, int64_t len) const override;
    void seek(int64_t offset) override final;
    [[nodiscard]] int64_t tell() const noexcept override final;
    [[nodiscard]] int64_t size() const noexcept override final;
};

class BinaryReader;

template <typename Source>
class CoverageTrackingSource : public Source {
    static_assert(std::is_base_of_v<ISource, Source>);

    std::vector<char> accessPattern; // TODO: replace with more space efficient, interval based, access pattern tracking

    bool completeCoverageInternal() const;

public:
    template <typename... Args>
    CoverageTrackingSource(Args&&... args);

    void read(char* dst, int64_t len) override;

    [[nodiscard]] static bool completeCoverage(const BinaryReader* br);
};

class BinaryReader {
    std::unique_ptr<ISource> source;

public:
    enum class Endianness { LE, BE };

    BinaryReader(BinaryReader& br) = delete;
    BinaryReader(BinaryReader&& br) = delete;

    //File Source constructors
    BinaryReader(const std::filesystem::path& path);
    BinaryReader(const std::string& path);
    BinaryReader(const char* path);

    // Buffer Source constructors
    BinaryReader(const char* data, int64_t data_size);
    BinaryReader(std::unique_ptr<char[]> data, int64_t data_size);
    BinaryReader(std::unique_ptr<ISource> source);

    BinaryReader& operator=(const BinaryReader& br) = delete;
    BinaryReader& operator=(BinaryReader&& br) = delete;

    [[nodiscard]] int64_t tell() const noexcept;
    void seek(int64_t pos);
    [[nodiscard]] int64_t size() const noexcept;

    template <typename T, Endianness en = Endianness::LE>
    void read(T* arr, int64_t len);

    template <typename T, Endianness en = Endianness::LE>
    [[nodiscard]] T read();

    template <typename T, Endianness en = Endianness::LE>
    void peek(T* arr, int64_t len) const;

    template <typename T, Endianness en = Endianness::LE>
    [[nodiscard]] T peek() const;

    template <unsigned int len, Endianness en = Endianness::BE>
    [[nodiscard]] std::string readString();

    template <Endianness en = Endianness::BE>
    [[nodiscard]] std::string readString(size_t charCount);

    template <Endianness en = Endianness::BE>
    [[nodiscard]] std::string readCString();

    template <typename T>
    void sink(int64_t len);

    template <typename T>
    void sink();

    template <unsigned int alignment = 0x10>
    void align();

    template <unsigned int alignment = 0x10>
    void alignZeroPad();

    [[nodiscard]] const ISource* getSource() const noexcept;
};

FileSource::FileSource(const std::string& path) : FileSource(std::filesystem::path(path)) {
}

FileSource::FileSource(const char* path) : FileSource(std::filesystem::path(path)) {
}

FileSource::FileSource(const std::filesystem::path& path) : size_(0) {
    if(!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path))
        throw std::runtime_error("Invalid path: " + path.generic_string());

    size_ = static_cast<uint64_t>(std::filesystem::file_size(path));

    ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    ifs.open(path.generic_string());
    if(!ifs.is_open())
        throw std::runtime_error("Failed to open file");
}

FileSource::~FileSource() {
    if(ifs.is_open())
        ifs.close();
}

void FileSource::read(char* dst, int64_t len) {
    ifs.read(dst, len);
    if(ifs.bad())
        throw std::runtime_error("Read fail");
}

void FileSource::peek(char* dst, int64_t len) const {
    auto o = tell();
    ifs.read(dst, len);
    ifs.seekg(o, std::ios::beg);
    if(ifs.bad())
        throw std::runtime_error("Peek fail");
}

void FileSource::seek(int64_t offset) {
    ifs.seekg(offset, std::ios::beg);
}

int64_t FileSource::tell() const noexcept {
    return ifs.tellg();
}

int64_t FileSource::size() const noexcept {
    return size_;
}

// Non owning constructor
BufferSource::BufferSource(const char* data, int64_t data_size)
: ownedBuffer(nullptr), buffer(data), bufferSize(data_size), cur(0) {
}

// Owning constructor
BufferSource::BufferSource(std::unique_ptr<char[]> data, int64_t data_size)
: ownedBuffer(std::move(data)), buffer(nullptr), bufferSize(data_size), cur(0) {
    buffer = ownedBuffer.get();
}

void BufferSource::read(char* dst, int64_t len) {
    if(cur + len > bufferSize)
        throw std::runtime_error("Out of bounds read");
    memcpy(dst, &(buffer[cur]), static_cast<size_t>(len));
    cur += len;
}

void BufferSource::peek(char* dst, int64_t len) const {
    if(cur + len > bufferSize)
        throw std::runtime_error("Out of bounds read");
    memcpy(dst, &(buffer[cur]), len);
}

void BufferSource::seek(int64_t offset) {
    cur = offset; // Seeking to OOB is not an error
}

int64_t BufferSource::tell() const noexcept {
    return cur;
}

int64_t BufferSource::size() const noexcept {
    return bufferSize;
}

void reverseEndianness(char* data, size_t size) {
    std::reverse(data, data + size);
}

template <size_t TypeSize>
void reverseEndianness(char* data) {
    std::reverse(data, data + TypeSize);
}

template <typename T>
void reverseEndianness(T& t) {
    static_assert(std::is_fundamental_v<T>);
    std::reverse(reinterpret_cast<char*>(&t), reinterpret_cast<char*>(&t) + sizeof(T));
}

template <>
void reverseEndianness<std::string>(std::string& str) {
    std::reverse(str.begin(), str.end());
}

BinaryReader::BinaryReader(const std::filesystem::path& path) {
    source = std::make_unique<FileSource>(path);
}

BinaryReader::BinaryReader(const std::string& path) {
    source = std::make_unique<FileSource>(path);
}

BinaryReader::BinaryReader(const char* path) {
    source = std::make_unique<FileSource>(path);
}

BinaryReader::BinaryReader(const char* data, int64_t data_size) {
    source = std::make_unique<BufferSource>(data, data_size);
}

BinaryReader::BinaryReader(std::unique_ptr<char[]> data, int64_t data_size) {
    source = std::make_unique<BufferSource>(std::move(data), data_size);
}

BinaryReader::BinaryReader(std::unique_ptr<ISource> source) : source(std::move(source)) {
}

int64_t BinaryReader::tell() const noexcept {
    return source->tell();
}

void BinaryReader::seek(int64_t pos) {
    source->seek(pos);
}

int64_t BinaryReader::size() const noexcept {
    return source->size();
}

template <typename T, BinaryReader::Endianness en>
void BinaryReader::read(T* arr, int64_t len) {
    source->read(reinterpret_cast<char*>(arr), sizeof(T) * len);
    if constexpr((en == Endianness::BE) && (sizeof(T) > sizeof(char))) {
        for(int i = 0; i < len; ++i)
            reverseEndianness(arr[i]);
    }
}

template <typename T, BinaryReader::Endianness en>
void BinaryReader::peek(T* arr, int64_t len) const {
    source->peek(reinterpret_cast<char*>(arr), sizeof(T) * len);
    if constexpr((en == Endianness::BE) && (sizeof(T) > sizeof(char))) {
        for(int i = 0; i < len; ++i)
            reverseEndianness(arr[i]);
    }
}

template <typename T, BinaryReader::Endianness en>
T BinaryReader::read() {
    static_assert(std::is_trivially_copyable_v<T>);

    if constexpr(en == BinaryReader::Endianness::BE)
        static_assert(std::is_fundamental_v<T>);

    T value;
    read<T, en>(&value, 1);
    return value;
}

template <typename T>
void BinaryReader::sink(int64_t len) {
    for(int i = 0; i < len; ++i)
        static_cast<void>(read<T>());
}

template <typename T>
void BinaryReader::sink() {
    static_cast<void>(read<T>());
}

template <typename T, BinaryReader::Endianness en>
T BinaryReader::peek() const {
    static_assert(std::is_trivially_copyable_v<T>);

    T value;
    peek<T, en>(&value, 1);
    return value;
}

template <unsigned int len, BinaryReader::Endianness en>
std::string BinaryReader::readString() {
    std::string str(len, '\0');
    read(str.data(), len);
    if constexpr(en == Endianness::LE)
        reverseEndianness(str);

    if(std::find(str.begin(), str.end(), '\0') != str.end())
        throw std::runtime_error("Read fixed size string containing null characters");

    return str;
}

template <BinaryReader::Endianness en>
std::string BinaryReader::readString(size_t charCount) {
    std::string str(charCount, '\0');
    read(str.data(), charCount);
    if constexpr(en == Endianness::LE)
        reverseEndianness(str);
    return str;
}

template <BinaryReader::Endianness en>
std::string BinaryReader::readCString() {
    std::vector<char> readBuffer;

    char c = '\0';
    do {
        c = read<char>();
        readBuffer.push_back(c);
    } while(c != '\0');

    std::string str(readBuffer.begin(), readBuffer.end() - 1);

    if constexpr(en == Endianness::LE)
        reverseEndianness(str);

    return str;
}

template <unsigned int alignment>
void BinaryReader::align() {
    char zero[alignment];
    uint64_t pos = tell();
    uint64_t padding_len = (alignment - pos) % alignment;
    read(zero, padding_len);
}

template <unsigned int alignment>
void BinaryReader::alignZeroPad() {
    char zero[alignment];
    uint64_t pos = tell();
    uint64_t padding_len = (alignment - pos) % alignment;
    read(zero, padding_len);

    auto it = std::find_if(std::begin(zero), &zero[padding_len], [](const char& c) { return c != 0; });

    if(it != &zero[padding_len])
        throw std::runtime_error("Non zero padding encountered");
}

const ISource* BinaryReader::getSource() const noexcept {
    return source.get();
}

template <typename Source>
template <typename... Args>
CoverageTrackingSource<Source>::CoverageTrackingSource(Args&&... args)
: Source(std::forward<Args>(args)...) {
    accessPattern.resize(Source::size(), 0);
}

template <typename Source>
void CoverageTrackingSource<Source>::read(char* dst, int64_t len) {
    auto cur = Source::tell();
    Source::read(dst, len);
    for(auto i = cur; i < cur + len; ++i) {
        if(accessPattern[i]++)
            throw std::runtime_error("Double read");
    }
}

template <typename Source>
bool CoverageTrackingSource<Source>::completeCoverage(const BinaryReader* br) {
    auto coverageReader = dynamic_cast<const CoverageTrackingSource<Source>*>(br->getSource());
    if(!coverageReader)
        throw std::bad_cast();
    return coverageReader->completeCoverageInternal();
}

template <typename Source>
bool CoverageTrackingSource<Source>::completeCoverageInternal() const {
    auto it = std::find(accessPattern.begin(), accessPattern.end(), 0);
    if(it == accessPattern.end())
        return true;
    return false;
}

} // namespace ZBinaryReader