#include "../Common/WorkerManager.cpp"

int main() {
    try {
        WorkerManager workerManager;
        workerManager.start();
    }
    catch (const std::exception& e) {
        printf("Error: %s\n", e.what());
        return 1;
    }
    return 0;
}