#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
#include <vector>

using namespace std;
using namespace chrono;

//class nullMutex {
//public:
//	void lock() {};
//	void unlock() {};
//};

class NODE {
public:
	int key;
	NODE* next;
	mutex nlock;

	NODE() { next = NULL; }

	NODE(int key_value) {
		next = NULL;
		key = key_value;
	}

	~NODE() {}

	void lock() {
		nlock.lock();
	}

	void unlock() {
		nlock.unlock();
	}
};

class OLIST {
	NODE head, tail;
	NODE* freeList;
	NODE freeTail;
	mutex fl_lock;

public:
	OLIST() {
		head.key = 0x80000000;
		tail.key = 0x7FFFFFFF;
		head.next = &tail;

		freeTail.key = 0x7FFFFFFF;
		freeList = &freeTail;
	}

	~OLIST() {}

	void Init() {
		NODE* ptr;
		while (head.next != &tail) {
			ptr = head.next;
			head.next = head.next->next;
			delete ptr;
		}
	}

	bool Add(int key) {
		while (true) {
			NODE* pred, * curr;
			pred = &head;
			curr = pred->next;

			while (curr->key < key) {
				pred = curr;
				curr = curr->next;
			}

			pred->lock(); curr->lock();

			if (validate(pred, curr)) {
				if (curr->key == key) {
					pred->unlock(); curr->unlock();

					return false;
				}
				else {
					NODE* node = new NODE(key);
					node->next = curr;
					pred->next = node;

					pred->unlock(); curr->unlock();
					return true;
				}
			}

			pred->unlock(); curr->unlock();
		}
	}

	bool Remove(int key) {
		while (true) {
			NODE* pred, * curr;
			pred = &head;
			curr = pred->next;

			while (curr->key < key) {
				pred = curr;
				curr = curr->next;
			}

			pred->lock(); curr->lock();

			if (validate(pred, curr)) {
				// 나와 같은 값이면 삭제
				if (key == curr->key) {
					pred->next = curr->next;

					fl_lock.lock();
					curr->next = freeList;
					freeList = curr;
					fl_lock.unlock();

					pred->unlock(); curr->unlock();
					return true;
				}
				else {
					pred->unlock(); curr->unlock();
					return false;
				}

			}

			pred->unlock(); curr->unlock();
		}
	}

	bool Contains(int key) {
		while (true) {
			NODE* pred, * curr;

			pred = &head;
			curr = pred->next;

			while (curr->key < key) {
				pred = curr;
				curr = curr->next;
			}

			pred->lock(); curr->lock();

			if (validate(pred, curr)) {
				if (key == curr->key) {
					pred->unlock(); curr->unlock();
					return true;
				}
				else {
					pred->unlock(); curr->unlock();
					return false;
				}

			}

			pred->unlock(); curr->unlock();
		}
	}

	void display20() {
		int c = 20;
		NODE* p = head.next;

		while (p != &tail) {
			cout << p->key;
			p = p->next;
			c--;
			if (c == 0) break;
			cout << ", ";
		}
		cout << endl;
	}

	void recycle_freeList() {
		NODE* p = freeList;
		while (p != &freeTail) {
			NODE* n = p->next;
			delete p;
			p = n;
		}
		freeList = &freeTail;
	}

	private:
		bool validate(NODE* pred, NODE* curr) {
			NODE* node = &head;
			while (node->key <= pred->key) {
				if (node == pred) {
					return pred->next == curr;
				}
				node = node->next;
			}
			return false;
		}
};

const auto NUM_TEST = 4000000;
const auto KEY_RANGE = 1000;

OLIST olist;

void Exec21(int num_thread) {
	int key;

	for (int i = 0; i < NUM_TEST / num_thread; ++i)
	{
		switch (rand() % 3) {
		case 0:
			key = rand() % KEY_RANGE;
			olist.Add(key);
			break;

		case 1:
			key = rand() % KEY_RANGE;
			olist.Remove(key);
			break;

		case 2:
			key = rand() % KEY_RANGE;
			olist.Contains(key);
			break;

		default:
			cout << "Error\n";
			exit(-1);
		}
	}
}

int main()
{
	for (int num_threads = 1; num_threads <= 64; num_threads *= 2)
	{
		olist.Init();
		vector<thread> threads;

		auto start_time = high_resolution_clock::now();

		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(Exec21, num_threads);

		for (auto& thread : threads)
			thread.join();

		threads.clear();

		auto end_time = high_resolution_clock::now();
		auto exec_time = end_time - start_time;
		int exec_ms = duration_cast<milliseconds>(exec_time).count();

		olist.display20();
		olist.recycle_freeList();

		cout << "Threads [" << num_threads << "] "
			<< " Exec_time  = " << exec_ms << "ms\n";
	}

	system("pause");
}