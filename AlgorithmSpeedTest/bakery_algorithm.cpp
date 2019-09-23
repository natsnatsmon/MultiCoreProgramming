#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>

#define THREAD_NUM 16
using namespace std;
using namespace chrono;

volatile bool flag[THREAD_NUM];
volatile int label[THREAD_NUM];

volatile int sum;

void lock(int num_threads, int thread_id) {
	flag[thread_id] = true;
	int max_label = 0;

	for (int i = 0; i < num_threads; i++)
	{
		int tmp_label = label[i];
		max_label = tmp_label > max_label ? tmp_label : max_label;
	}

	label[thread_id] = max_label + 1;

	flag[thread_id] = false;


	for (int other = 0; other < num_threads; ++other)
	{
		while (flag[other]) {}
		while ( (label[other] != 0) &&
			( (label[other] < label[thread_id]) ||
			  ((label[other] == label[thread_id]) &&
				  (other < thread_id)) )
			) {}
	}
}

void unlock(int num_threads, int thread_id)
{
	label[thread_id] = false;
}

void do_work(int num_threads, int thread_id)
{
	for (int i = 0; i < 50000000 / num_threads; ++i)
	{
		lock(num_threads, thread_id);
		sum += 2;
		unlock(num_threads, thread_id);
	}
}

int main()
{
	for (int num_thread = 1; num_thread <= 16; num_thread *= 2)
	{
		sum = 0;

		vector<thread> threads;

		auto start_time = high_resolution_clock::now();

		for (int i = 0; i < num_thread; ++i)
		{
			threads.emplace_back(do_work, num_thread, i);
		}
		for (auto &th : threads) { th.join(); }

		auto end_time = high_resolution_clock::now();

		threads.clear();

		auto exec_time = end_time - start_time;


		int exec_ms = duration_cast<milliseconds>(exec_time).count();

		cout << "Threads : " << num_thread << ", Sum = " << sum;
		cout << ", Exec_time = " << exec_ms << "ms" << endl;
	}

	system("pause");
}