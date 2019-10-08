#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
#include <vector>

using namespace std;
using namespace chrono;

class SPNODE {
public:
	int key;
	shared_ptr <SPNODE> next;
	mutex spn_lock;
	bool removed;

	SPNODE() { next = nullptr; removed = false; }

	SPNODE(int key_value) {
		key = key_value;
		next = nullptr;
		removed = false;
	}

	~SPNODE() {}

	void lock() {
		spn_lock.lock();
	}

	void unlock() {
		spn_lock.unlock();
	}
};

class SPLLIST {
	shared_ptr <SPNODE> head, tail;

public:
	SPLLIST() {
		head = make_shared<SPNODE>();
		head->key = 0x80000000;
		tail = make_shared<SPNODE>();
		tail->key = 0x7FFFFFFF;
		head->next = tail;
	}

	~SPLLIST() {}

	void Init() {
		head->next = tail;
	}

	bool Add(int key) {
		while (true) {
			shared_ptr<SPNODE> pred,  curr;
			pred = head; // 이건 가능 head가 있는 자리는 확실하니까
			
			curr = pred->next; // 이건 터질 가능성 있음. head->next는 바뀌기 때문에 data race 생김

			
			while (curr->key < key) {
				pred = curr; // 이건 가능 로컬변수끼리 가능가능
				curr = curr->next; // curr->next는 전역변수 건드는 일이라 터질 가능성 있음. 락 걸던지..
			}

			pred->lock(); curr->lock();

			if (validate(pred, curr)) {
				if (curr->key == key) {
					pred->unlock(); curr->unlock();

					return false;
				}
				else {
					shared_ptr<SPNODE> node = make_shared<SPNODE>(key);
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
			shared_ptr<SPNODE> pred, curr;
			pred = head;

			curr = pred->next;

			
			while (curr->key < key) {
				pred = curr;
				curr = curr->next;
			}

			pred->lock(); curr->lock();

			if (validate(pred, curr)) {
				// 나와 같은 값이면 삭제
				if (key == curr->key) {
					curr->removed = true;
					pred->next = curr->next;

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
		shared_ptr<SPNODE> curr;
		curr = head;


		while (curr->key < key)
		{
			curr = curr->next;
		}

		return (curr->key == key) && (!curr->removed);
	}

	void display20() {
		int c = 20;
		shared_ptr<SPNODE> p = head->next;

		while (p != tail) {
			cout << p->key;
			p = p->next;
			c--;
			if (c == 0) break;
			cout << ", ";
		}
		cout << endl;
	}

private:
	bool validate(shared_ptr<SPNODE> pred, shared_ptr<SPNODE> curr) {
		return (!pred->removed) && (!curr->removed) && (pred->next == curr);
	}
};

const auto NUM_TEST = 40000;
const auto KEY_RANGE = 1000;

SPLLIST spllist;

void Exec23(int num_thread) {
	int key;

	for (int i = 0; i < NUM_TEST / num_thread; ++i)
	{
		switch (rand() % 3) {
		case 0:
			key = rand() % KEY_RANGE;
			spllist.Add(key);
			break;

		case 1:
			key = rand() % KEY_RANGE;
			spllist.Remove(key);
			break;

		case 2:
			key = rand() % KEY_RANGE;
			spllist.Contains(key);
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
		spllist.Init();
		vector<thread> threads;

		auto start_time = high_resolution_clock::now();

		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(Exec23, num_threads);

		for (auto& thread : threads)
			thread.join();

		threads.clear();

		auto end_time = high_resolution_clock::now();
		auto exec_time = end_time - start_time;
		int exec_ms = duration_cast<milliseconds>(exec_time).count();

		spllist.display20();
		cout << "Threads [" << num_threads << "] "
			<< " Exec_time  = " << exec_ms << "ms\n\n";
	}

	system("pause");
}