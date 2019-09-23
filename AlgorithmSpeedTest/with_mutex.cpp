#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <atomic>

using namespace std;
using namespace chrono;

mutex mylock;
volatile int sum;

void do_work(int num_threads)
{
	for (int i = 0; i < 50000000 / num_threads; ++i)
	{
		mylock.lock();

		sum += 2;

		mylock.unlock();
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
			threads.emplace_back(do_work, num_thread);
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