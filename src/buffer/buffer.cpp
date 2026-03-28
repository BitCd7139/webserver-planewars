#include "buffer.h"
#include <cassert>
#include <strings.h>
#include <unistd.h>
#include <sys/uio.h>

namespace webserver {
    std::size_t Buffer::ReadableByteSize() const noexcept {
        return write_pos_ - read_pos_;
    }

    std::size_t Buffer::WritableByteSize() const noexcept {
        return buffer_.size() - write_pos_;
    }

    std::size_t Buffer::PrependableByteSize() const noexcept {
        return read_pos_;
    }

    const char* Buffer::Peek() const noexcept{
        return BeginPtr_() + read_pos_;
    }

    void Buffer::Retrieve(size_t len) noexcept{
        assert(len <= ReadableByteSize());
        read_pos_ += len;
    }

    void Buffer::RetrieveUntil(const char* end) noexcept{
        assert(Peek() <= end );
        Retrieve(end - Peek());
        }

    void Buffer::RetrieveAll() noexcept{
        bzero(&buffer_[0], buffer_.size());
        read_pos_ = 0;
        write_pos_ = 0;
    }

    std::string Buffer::RetrieveAllToStr() {
        std::string str(Peek(), ReadableByteSize());
        RetrieveAll();
        return str;
    }

    const char* Buffer::BeginWriteConst() const noexcept{
        return BeginPtr_() + write_pos_;
    }

    char* Buffer::BeginWrite() noexcept{
        return BeginPtr_() + write_pos_;
    }

    void Buffer::AfterWrite(size_t len) noexcept{
        write_pos_ += len;
    }

    void Buffer::Append(const std::string_view str) {
        Append(str.data(), str.length());
    }

    void Buffer::Append(const void* data, size_t len) {
        assert(data);
        Append(static_cast<const char*>(data), len);
    }

    void Buffer::Append(const char* str, size_t len) {
        assert(str);
        EnsureWriteable(len);
        std::copy(str, str + len, BeginWrite());
        AfterWrite(len);
    }

    void Buffer::Append(const Buffer& buff) {
        Append(buff.Peek(), buff.ReadableByteSize());
    }

    void Buffer::EnsureWriteable(size_t len) {
        if(WritableByteSize() < len) {
            MakeSpace_(len);
        }
        assert(WritableByteSize() >= len);
    }

    ssize_t Buffer::ReadFd(int fd, int* saveErrno) {
        char buff[65535];
        struct iovec iov[2];
        const size_t writable = WritableByteSize();

        iov[0].iov_base = BeginPtr_() + write_pos_;
        iov[0].iov_len = writable;
        iov[1].iov_base = buff;
        iov[1].iov_len = sizeof(buff);

        const ssize_t len = readv(fd, iov, 2);
        if(len < 0) {
            *saveErrno = errno;
        }
        else if(static_cast<size_t>(len) <= writable) {
            write_pos_ += len;
        }
        else {
            write_pos_ = buffer_.size();
            Append(buff, len - writable);
        }
        return len;
    }

    ssize_t Buffer::WriteFd(int fd, int* saveErrno) {
        const size_t readSize = ReadableByteSize();
        const ssize_t len = write(fd, Peek(), readSize);
        if(len < 0) {
            *saveErrno = errno;
            return len;
        }
        read_pos_ += len;
        return len;
    }

    char* Buffer::BeginPtr_() noexcept{
        return &*buffer_.begin();
    }

    const char* Buffer::BeginPtr_() const noexcept{
        return &*buffer_.begin();
    }

    void Buffer::MakeSpace_(size_t len) {
        if(WritableByteSize() + PrependableByteSize() < len) {
            buffer_.resize(write_pos_ + len + 1);
        }
        else {
            size_t readable = ReadableByteSize();
            std::copy(BeginPtr_() + read_pos_, BeginPtr_() + write_pos_, BeginPtr_());
            read_pos_ = 0;
            write_pos_ = read_pos_ + readable;
            assert(readable == ReadableByteSize());
        }
    }

} // webserver