#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>

using namespace std;
using namespace chrono;

class NODE;

// Marked pointer
class MPTR {
	int value; // 주소 + 마크

public:
	void set(NODE* node, bool removed) {
		value = reinterpret_cast<int>(node);

		if (true == value) { value = value | 0x01; }
		else { value = value & 0xFFFFFFFE; }
	}

	bool CAS(int old_v, int new_v) {
		return atomic_compare_exchange_strong(
			reinterpret_cast<atomic_int*>(&value), & old_v, new_v);
	}

	// making은 두면서 next 값만 바꾸는 CAS
	bool CAS(NODE* old_node, NODE* new_node, bool old_mark, bool new_mark) {
		int old_value = reinterpret_cast<int>(old_node);
		if (old_mark) old_value = old_value | 0x01;
		else old_value = old_value & 0xFFFFFFFE;

		int new_value = reinterpret_cast<int>(new_node);
		if (new_mark) new_value = new_value | 0x01;
		else new_value = new_value & 0xFFFFFFFE;

		return CAS(old_value, new_value);
	}

	// next 값은 두면서 making만 바꾸는 CAS
	bool AttemptMark(NODE* old_node, bool new_mark) {
		int old_value = reinterpret_cast<int>(old_node);
		int new_value = old_value;

		if (new_mark) new_value = new_value | 0x01;
		else new_value = new_value & 0xFFFFFFFE;

		return CAS(old_value, new_value);
	}

	// 합성 주소에서 주소만 리턴함
	// == GetReference()
	NODE* GetPtr() {
		return reinterpret_cast<NODE*>(value & 0xFFFFFFFE);
	}

	// 합성 주소에서 주소를 리턴하고 making 여부를 call by reference로 리턴
	NODE* GetPtrNMark(bool &removed) {
		int temp = value;
		if (0 == (temp & 0x1)) removed = false;
		else removed = true;

		return reinterpret_cast<NODE*>(value & 0xFFFFFFFE);
	}

};

class NODE {

public:
	int key;
	MPTR next;

	NODE() : key(0) { next.set(nullptr, false); }

	NODE(int key_value) : key(key_value) {
		next.set(nullptr, false);
	}

	~NODE() {}
};


class NLIST {
	NODE head, tail;

	// freeList는 나중에 새로 만듦
	NODE* freeList;
	NODE freeTail;
	
public:
	NLIST() {
		head.key = 0x80000000;
		tail.key = 0x7FFFFFFF;
		head.next.set(&tail, false);

		//cout << "&tail : " << &tail << endl;
		//cout << "head.next.GetPtr() : " << head.next.GetPtr() << endl;
		freeTail.key = 0x7FFFFFFF;
		freeList = &freeTail;
	}

	~NLIST() {}

	void Init() {
		NODE* ptr;
		while (head.next.GetPtr() != &tail) {
			ptr = head.next.GetPtr();
			head.next = head.next.GetPtr()->next;
			delete ptr;
		}
	}

	// pred와 curr의 값을 변경했을 때 날아가지 않고 바뀌게 하기 위해 레퍼런스로 받음
	void Find(int key, NODE* (&pred), NODE* (&curr)) {
	retry:
		while (true) {
			pred = &head;
			curr = pred->next.GetPtr();

			while (true) {
				bool removed = false;

				NODE* succ = curr->next.GetPtrNMark(removed);

				while (true == removed) {
					if (false == pred->next.CAS(curr, succ, false, false))
						goto retry;
					curr = succ;
					succ = curr->next.GetPtrNMark(removed);
				}

				if (curr->key >= key) return;

				pred = curr;
				curr = succ;

			}
		}
	}

	bool Add(int key) {
		NODE* pred, * curr;

		while (true) {
			Find(key, pred, curr);
			if (curr->key == key)
				return false;
			
			NODE* node = new NODE(key);
			node->next.set(curr, false);

			if (pred->next.CAS(curr, node, false, false)) {
				return true;
			}
		}
	}

	bool Remove(int key) {
		bool snip;
		
		while (true) {
			NODE* pred, * curr;
			Find(key, pred, curr);

			if (curr->key != key) {
				return false;
			}
			else {
				NODE* succ = curr->next.GetPtr();
				snip = curr->next.AttemptMark(succ, true);

				if (!snip) continue;

				// 성공, 실패와 상관없이 걍 끝내버린다..! 뒷수습 안해~ 묻지도 따지지도 않고 return true 때려~
				pred->next.CAS(curr, succ, false, false);
				return true;
			}
		}
	}

	bool Contains(int key) {
		bool marked = false;

		NODE* curr;
		curr = &head;

		while (curr->key < key)
		{
			curr = curr->next.GetPtr();
			NODE* succ = curr->next.GetPtrNMark(marked);
		}

		return (curr->key == key) && (!marked);
	}

	void display20() {
		int c = 20;
		NODE* p = head.next.GetPtr();

		while (p != &tail) {
			cout << p->key;
			p = p->next.GetPtr();
			c--;
			if (c == 0) break;
			cout << ", ";
		}
		cout << endl;
	}

	//void recycle_freeList() {
	//	NODE* p = freeList;
	//	while (p != &freeTail) {
	//		NODE* n = p->next.GetPtr();
	//		delete p;
	//		p = n;
	//	}
	//	freeList = &freeTail;
	//}
};

const auto NUM_TEST = 4000000;
const auto KEY_RANGE = 1000;

NLIST nlist;

void Exec22(int num_thread) {
	int key;

	for (int i = 0; i < NUM_TEST / num_thread; ++i)
	{
		switch (rand() % 3) {
		case 0:
			key = rand() % KEY_RANGE;
			nlist.Add(key);
			break;

		case 1:
			key = rand() % KEY_RANGE;
			nlist.Remove(key);
			break;

		case 2:
			key = rand() % KEY_RANGE;
			nlist.Contains(key);
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
		nlist.Init();
		vector<thread> threads;

		auto start_time = high_resolution_clock::now();

		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(Exec22, num_threads);

		for (auto& thread : threads)
			thread.join();

		threads.clear();

		auto end_time = high_resolution_clock::now();
		auto exec_time = end_time - start_time;
		int exec_ms = duration_cast<milliseconds>(exec_time).count();

		nlist.display20();
		//nlist.recycle_freeList();
		cout << "Threads [" << num_threads << "] "
			<< " Exec_time  = " << exec_ms << "ms\n\n";
	}

	system("pause");
}