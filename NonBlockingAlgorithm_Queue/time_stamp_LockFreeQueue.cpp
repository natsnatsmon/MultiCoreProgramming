#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>

using namespace std;
using namespace chrono;

class NODE {
public:
	int key;
	NODE* next;

	NODE() : key(0) { next = NULL; }

	NODE(int key_value) {
		next = NULL;
		key = key_value;
	}

	~NODE() {}
};

// stamped pointer
class SPTR {
public:
	NODE* volatile ptr;
	volatile int stamp;

	SPTR() {
		ptr = nullptr;
		stamp = 0;
	}

	SPTR(NODE* p, int v) {
		ptr = p;
		stamp = v;
	}
};


class SLFQUEUE {
	SPTR head;
	SPTR tail;

public:
	SLFQUEUE() {
		head.ptr = tail.ptr = new NODE(0);
	}

	~SLFQUEUE() {}

	void Init() {
		NODE* ptr;
		while (head.ptr->next != nullptr) {
			ptr = head.ptr->next;
			head.ptr->next = head.ptr->next->next;
			delete ptr;
		}

		tail = head;
	}

	bool CAS(NODE* volatile* addr, NODE* old_node, NODE* new_node)
	{
		return atomic_compare_exchange_strong(reinterpret_cast<volatile atomic_int*>(addr),
			reinterpret_cast<int*>(&old_node),
			reinterpret_cast<int>(new_node));
	}

	bool STAMPCAS(SPTR* addr, NODE* old_node, int old_stamp, NODE* new_node)
	{
		SPTR old_ptr{ old_node, old_stamp };
		SPTR new_ptr{ new_node, old_stamp + 1 };
		return atomic_compare_exchange_strong(reinterpret_cast<atomic_llong*>(addr),
			reinterpret_cast<long long*>(&old_ptr),
			*(reinterpret_cast<long long*>(&new_ptr)));
	}

	void Enq(int key) {
		NODE* e = new NODE(key);
		while (true) {
			SPTR last = tail;
			NODE* next = last.ptr->next;

			if (last.ptr != tail.ptr) continue;
			if (nullptr == next) {
				if (CAS(&(last.ptr->next), nullptr, e)) {
					STAMPCAS(&tail, last.ptr, last.stamp, e);
					return;
				}
			}
			else STAMPCAS(&tail, last.ptr, last.stamp, next);
		}
	}

	int Deq()
	{
		while (true) {
			SPTR first = head;
			NODE* next = first.ptr->next;
			SPTR last = tail;
			NODE* lastnext = last.ptr->next;

			if (first.ptr != head.ptr) continue;
			if (last.ptr == first.ptr) {
				if (lastnext == nullptr) {
					cout << "EMPTY!!! ";
					this_thread::sleep_for(1ms);
					return -1;
				}
				else
				{
					STAMPCAS(&tail, last.ptr, last.stamp, lastnext);
					continue;
				}
			}

			if (nullptr == next) continue;
			int result = next->key;
			if (false == STAMPCAS(&head, first.ptr, first.stamp, next)) continue;
			//first.ptr->next = nullptr;
			delete first.ptr;
			return result;
		}
	}

	int EMPTY_ERROR() {
		cout << "큐가 비어있다!\n";
		return -1;
	}

	void display20() {
		int c = 20;
		NODE* p = head.ptr->next;

		while (p != nullptr) {
			cout << p->key;
			p = p->next;
			c--;
			if (c == 0) break;
			cout << ", ";
		}
		cout << endl;
	}
};

const auto NUM_TEST = 10000000;
const auto KEY_RANGE = 100;

SLFQUEUE my_queue;

void Exec25(int num_thread) {
	for (int i = 0; i < NUM_TEST / num_thread; ++i)
	{
		if (rand() % 2 == 0 || i < (10000 / num_thread))
		{
			my_queue.Enq(i);
		}
		else {
			my_queue.Deq();
		}
	}
}

int main()
{
	for (int num_threads = 1; num_threads <= 16; num_threads *= 2)
	{
		my_queue.Init();
		vector<thread> threads;

		auto start_time = high_resolution_clock::now();

		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(Exec25, num_threads);

		for (auto& thread : threads)
			thread.join();

		threads.clear();

		auto end_time = high_resolution_clock::now();
		auto exec_time = end_time - start_time;
		int exec_ms = duration_cast<milliseconds>(exec_time).count();

		my_queue.display20();
		//nlist.recycle_freeList();
		cout << "Threads [" << num_threads << "] "
			<< " Exec_time  = " << exec_ms << "ms\n\n";
	}

	system("pause");
}