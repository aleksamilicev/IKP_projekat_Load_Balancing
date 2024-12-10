#include "../Common/LoadBalancer.cpp"

int main() {
    try {
        LoadBalancer loadBalancer;
        loadBalancer.start();
    }
    catch (const std::exception& e) {
        printf("Error: %s\n", e.what());
        return 1;
    }
    return 0;
}