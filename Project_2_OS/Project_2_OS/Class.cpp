#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

using namespace std;

vector<double> arr;
double minVal, maxVal;
double averageVal;

void MinMaxThread() {
    minVal = arr[0];
    maxVal = arr[0];
    for (size_t i = 1; i < arr.size(); i++) {
        if (arr[i] < minVal) {
            minVal = arr[i];
        }
        this_thread::sleep_for(chrono::milliseconds(7));
        if (arr[i] > maxVal) {
            maxVal = arr[i];
        }
        this_thread::sleep_for(chrono::milliseconds(7));
    }
    cout << "Min: " << minVal << " , Max: " << maxVal << endl;
}

void AverageThread() {
    double sum = 0;
    for (double x : arr) {
        sum += x;
        this_thread::sleep_for(chrono::milliseconds(12));
    }
    averageVal = sum / arr.size();
    cout << "Average: " << averageVal << endl;
}

bool ReadArrayFromStream(istream& in) {
    int n;
    in >> n;
    if (!in || n <= 0) return false;

    arr.resize(n);
    for (int i = 0; i < n; ++i) {
        in >> arr[i];
        if (!in) return false;
    }
    return true;
}

int main() {
    cout << "Enter array size and elements:\n";

    if (!ReadArrayFromStream(cin)) {
        cerr << "Invalid input. Program terminated.\n";
        return 1;
    }

    thread t1(MinMaxThread);
    thread t2(AverageThread);

    t1.join();
    t2.join();

    for (double& x : arr) {
        if (x == minVal || x == maxVal) {
            x = averageVal;
        }
    }

    cout << "Modified array:\n";
    for (double x : arr) {
        cout << x << " ";
    }
    cout << endl;

    return 0;
}
