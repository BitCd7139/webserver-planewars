#ifndef WEBSERVER_BUFFER_H
#define WEBSERVER_BUFFER_H

#include <vector>
#include <string>
#include <string_view>
#include <cstddef>

namespace webserver {

    class Buffer {
    public:
        explicit Buffer(int initBuffSize = 1024) : buffer_(initBuffSize) {}
        ~Buffer() = default;

        [[nodiscard]] std::size_t WritableByteSize() const noexcept;
        [[nodiscard]] std::size_t ReadableByteSize() const noexcept;
        [[nodiscard]] std::size_t PrependableByteSize() const noexcept;

        [[nodiscard]] const char* Peek() const noexcept;

        void EnsureWriteable(std::size_t len);
        void AfterWrite(std::size_t len) noexcept;

        void Retrieve(std::size_t len) noexcept;
        void RetrieveUntil(const char* end) noexcept;

        void RetrieveAll() noexcept;

        [[nodiscard]] std::string RetrieveAllToStr();

        [[nodiscard]] const char* BeginWriteConst() const noexcept;
        [[nodiscard]] char* BeginWrite() noexcept;

        void Append(std::string_view str);
        void Append(const void* data, std::size_t len);
        void Append(const char *str, size_t len);
        void Append(const Buffer& buff);

        [[nodiscard]] ssize_t ReadFd(int fd, int* Errno);
        [[nodiscard]] ssize_t WriteFd(int fd, int* Errno);

    private:
        char* BeginPtr_() noexcept;
        const char* BeginPtr_() const noexcept;
        void MakeSpace_(std::size_t len);

        std::vector<char> buffer_;

        std::size_t read_pos_{0};
        std::size_t write_pos_{0};
    };

} //  webserver

#endif // WEBSERVER_BUFFER_H