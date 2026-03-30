#ifndef LOCK_FREE_QUEUE_HPP
#define LOCK_FREE_QUEUE_HPP
#include<atomic>
#include<stdexcept>

namespace webserver {
	template<typename T>
	class LockFreeQueue {
	private:
		struct Node {
			T data;
			std::atomic<size_t> id;
		};

		Node* buffer;

		size_t capacity;
		size_t buffer_mask;
		std::atomic<size_t> count;

		static constexpr size_t kCacheLineSize = 64;
		alignas(kCacheLineSize) std::atomic<size_t> head;
		alignas(kCacheLineSize) std::atomic<size_t> tail;

	public:
		explicit LockFreeQueue(size_t capacity = 1024) : capacity(capacity), buffer_mask(capacity - 1), count(0), head(0), tail(0) {
			if (capacity < 2 || (capacity & (capacity - 1)) != 0) {
				throw std::invalid_argument("Capacity must be a power of 2");
			}
			buffer = new Node[capacity];

			for (size_t i = 0; i < capacity; i++) {
				buffer[i].id.store(i, std::memory_order_relaxed);
			}
		}
		~LockFreeQueue() {
			delete[] buffer;
		}

		[[nodiscard]] size_t size() const {
			return (head - tail) & buffer_mask;
		}

		bool enqueue(T&& data) {
			size_t pos = head.load(std::memory_order_relaxed);
			Node* node;

			while (true){
				node = &buffer[pos & buffer_mask];
				size_t seq = node->id.load(std::memory_order_acquire);
				auto diff = static_cast<ptrdiff_t>(seq - pos);
				if (diff == 0) {
					if (head.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
						break;
					}
				}
				else if (diff < 0) {
					return false;
				}
				else {
					pos = head.load(std::memory_order_relaxed);
					continue;
				}
			}

			node->data = std::move(data);
			node->id.store(pos + 1, std::memory_order_release);
			return true;
		}

		bool dequeue(T& data) {
			size_t pos = tail.load(std::memory_order_relaxed);
			Node* node;

			while (true) {
				node = &buffer[pos & buffer_mask];
				size_t seq = node->id.load(std::memory_order_acquire);
				auto diff = static_cast<ptrdiff_t>(seq - (pos+1));
				if (diff == 0) {
					if (tail.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
						break;
					}
				}
				else if (diff < 0) {
					return false;
				}
				else {
					pos = tail.load();
					continue;
				}
			}

			data = std::move(node->data);
			node->id.store(pos + buffer_mask + 1, std::memory_order_release);
			return true;
		}
	};
}

#endif