#include "pch.h"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include "Header.h"

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
}

void AverageThread() {
    double sum = 0;
    for (double x : arr) {
        sum += x;
        this_thread::sleep_for(chrono::milliseconds(12));
    }
    averageVal = sum / arr.size();
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