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

class FLIST {
	NODE head, tail;
	
public:
	FLIST() {
		head.key = 0x80000000;
		tail.key = 0x7FFFFFFF;
		head.next = &tail;
	}

	~FLIST() {}

	void Init() {
		NODE* ptr;
		while (head.next != &tail) {
			ptr = head.next;
			head.next = head.next->next;
			delete ptr;
		}
	}

	bool Add(int key) {
		head.lock();

		NODE* pred;
		pred = &head;

		NODE* curr = pred->next;
		curr->lock();

		while (curr->key < key) {
			pred->unlock();
			pred = curr;
			curr = curr->next;
			curr->lock();
		}

		// 나와 같은 값이면 추가 실패
		if (key == curr->key) {
			curr->unlock();
			pred->unlock();
			return false;
		}
		else {
			NODE* node = new NODE(key);
			node->lock();
			node->next = curr;
			pred->next = node;

			node->unlock();
			curr->unlock();
			pred->unlock();
			return true;
		}
	}

	bool Remove(int key) {
		head.lock();

		NODE* pred;
		pred = &head;

		NODE* curr = pred->next;
		curr->lock();

		while (curr->key < key) {
			pred->unlock();
			pred = curr;
			curr = curr->next;
			curr->lock();
		}

		// 나와 같은 값이면 삭제
		if (key == curr->key) {
			pred->next = curr->next;
			delete curr;

			pred->unlock();

			return true;
		}
		else {
			curr->unlock();
			pred->unlock();

			return false;
		}

	}

	bool Contains(int key) {
		head.lock();

		NODE* pred;
		pred = &head;

		NODE* curr = pred->next;
		curr->lock();

		while (curr->key < key) {
			pred->unlock();
			pred = curr;
			curr = curr->next;
			curr->lock();
		}

		if (key == curr->key) {
			curr->unlock();
			pred->unlock();

			return true;
		}
		else {
			curr->unlock();
			pred->unlock();

			return false;
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
};

const auto NUM_TEST = 4000000;
const auto KEY_RANGE = 1000;

FLIST flist;

void Exec20(int num_thread) {
	int key;

	for (int i = 0; i < NUM_TEST / num_thread; ++i)
	{
		switch (rand() % 3) {
		case 0:
			key = rand() % KEY_RANGE;
			flist.Add(key);
			break;

		case 1:
			key = rand() % KEY_RANGE;
			flist.Remove(key);
			break;

		case 2:
			key = rand() % KEY_RANGE;
			flist.Contains(key);
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
		flist.Init();
		vector<thread> threads;

		auto start_time = high_resolution_clock::now();

		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(Exec20, num_threads);

		for (auto& thread : threads)
			thread.join();

		threads.clear();

		auto end_time = high_resolution_clock::now();
		auto exec_time = end_time - start_time;
		int exec_ms = duration_cast<milliseconds>(exec_time).count();

		flist.display20();

		cout << "Threads [" << num_threads << "] "
			<< " Exec_time  = " << exec_ms << "ms\n";
	}

	system("pause");
}