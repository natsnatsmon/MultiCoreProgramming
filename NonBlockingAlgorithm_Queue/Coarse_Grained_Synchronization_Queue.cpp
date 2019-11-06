#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <queue>

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

class nullmutex {
public:
	void lock() {}
	void unlock() {}
};
class CQUEUE {
	NODE *head, *tail;
	nullmutex glock;

public:
	CQUEUE() {
		head = tail = new NODE(0);
	}

	~CQUEUE() {}

	void Init() {
		NODE* ptr;
		while (head ->next != nullptr) {
			ptr = head->next;
			head->next = head->next->next;
			delete ptr;

		}

		tail = head;
	}

	void Enq(int key) {
		glock.lock();

		NODE* e = new NODE(key);
		tail->next = e;
		tail = e;

		glock.unlock();
	}

	int Deq() {
		int result{ 0 };

		glock.lock();

		if (head->next != nullptr) {
			result = head->next->key;
			head = head->next;
		}
		

		glock.unlock();
		
		return result;
	}


	void display20() {
		int c = 20;
		NODE* p = head->next;

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
//const auto KEY_RANGE = 1000;

CQUEUE cqueue;

void Exec24(int num_thread) {
	int key;
	//for (int i = 0; i < 10000 / num_thread; ++i) {
	//	cqueue.Enq(i);
	//}

	for (int i = 0; i < NUM_TEST / num_thread; ++i)
	{
		if (rand() % 2 == 0 || i < 10000)
		{
			cqueue.Enq(i);
		}
		else {
			cqueue.Deq();
		}
	}
}

int main()
{
	for (int num_threads = 1; num_threads <= 16; num_threads *= 2)
	{
		cqueue.Init();
		vector<thread> threads;

		auto start_time = high_resolution_clock::now();

		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(Exec24, num_threads);

		for (auto& thread : threads)
			thread.join();

		threads.clear();

		auto end_time = high_resolution_clock::now();
		auto exec_time = end_time - start_time;
		int exec_ms = duration_cast<milliseconds>(exec_time).count();

		cqueue.display20();

		cout << "Threads [" << num_threads << "] "
			<< " Exec_time  = " << exec_ms << "ms\n\n";
	}

	system("pause");
}