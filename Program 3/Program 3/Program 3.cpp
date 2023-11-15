#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <locale>
#include "windows.h"
#include <condition_variable>
#include <chrono>

using namespace std;

struct ActionGroup {
    int from;
    int to;
    char type;
    int num_actions;
};

struct Action {
    char type;
    int index;
};

vector<Action> actions;
vector<int> done;
int doneCount = 0;
int currentActionIndex = 0;
int completedActions = 0;
mutex mtx;
condition_variable cv;
bool isThreadEnded = false;

void processActions(int threadId) {
    while (!isThreadEnded) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [&]() { return currentActionIndex < actions.size(); });

        int currentIndex = currentActionIndex++;
        lock.unlock();

        if (currentIndex < actions.size()) {
            cout << "З набору " << actions[currentIndex].type << " виконано дію " << actions[currentIndex].index << endl;

            completedActions++;

            if (completedActions == 4) {
                completedActions = 0;
                lock.lock();
                cv.notify_all();
                lock.unlock();
                this_thread::sleep_for(chrono::milliseconds(100));
            }
        }
        else {
            isThreadEnded = true;
            break;
        }

        if (currentIndex == actions.size() - 1) {
            isThreadEnded = true;
            cv.notify_all();
            break;
        }
    }
}

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    setlocale(LC_CTYPE, "ukr");

    ofstream outputFile("results.txt");
    if (!outputFile.is_open()) {
        cerr << "Error opening output file." << endl;
        return 1; // Exit with an error code
    }

    // Redirect cout to the output file
    streambuf* coutBuf = cout.rdbuf();
    cout.rdbuf(outputFile.rdbuf());

    vector<ActionGroup> graph = {
        {1, 3, 'a', 4}, {1, 2, 'b', 4}, {2, 3, 'e', 4}, {1, 4, 'c', 4}, {1, 5, 'd', 4},
        {2, 5, 'f', 5}, {4, 3, 'g', 4}, {4, 5, 'h', 8}, {3, 6, 'i', 5}, {5, 6, 'j', 8}
    };

    done.resize(graph.size());

    while (doneCount < done.size()) {
        for (int i = 0; i < graph.size(); i++) {
            if (done[i] < graph[i].num_actions) {
                bool isConnected = false;
                for (int j = 0; j < graph.size(); j++) {
                    if (i == j || done[j] == graph[j].num_actions) {
                        continue;
                    }
                    if (graph[i].from == graph[j].to) {
                        isConnected = true;
                        break;
                    }
                }
                if (!isConnected) {
                    done[i]++;
                    actions.push_back({ graph[i].type, done[i] });
                    if (done[i] == graph[i].num_actions) {
                        doneCount++;
                    }
                }
            }
        }
    }

    std::cout << "Обчислення розпочато.\n";


    vector<thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(processActions, i);
    }
    try {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [&]() { return isThreadEnded; });
        lock.unlock();
        cv.notify_all();
    }
    catch (const std::exception& e) {

    }

    if (isThreadEnded) {
        std::cout << "Обчислення завершено.\n";
        outputFile.close();
        cout.rdbuf(coutBuf);
        exit(1);
    }

    for (auto& t : threads) {
        t.join();
    }

    return 0;
}
