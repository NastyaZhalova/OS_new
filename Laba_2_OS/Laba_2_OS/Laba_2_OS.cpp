#include <windows.h>
#include <iostream>
#include <vector>

using namespace std;

vector<double> arr;
double minVal, maxVal;
double averageVal;

DWORD WINAPI MinMaxThread(LPVOID lpParam) {
    minVal = arr[0];
    maxVal = arr[0];
    for (int i = 1; i < arr.size(); i++) {
        if (arr[i] < minVal) {
            minVal = arr[i];
        }
        if (arr[i] > maxVal) {
            maxVal = arr[i];
        }
        Sleep(7);
    }
    cout << "Min: " << minVal << ", Max: " << maxVal << endl;
    return 0;
}

DWORD WINAPI AverageThread(LPVOID lpParam) {
    double sum = 0;
    for (int i = 0; i < arr.size(); i++) {
        sum += arr[i];
        Sleep(12);
    }
    averageVal = sum / arr.size();
    cout << "Average: " << averageVal << endl;
    return 0;
}

bool ReadArrayFromStream(std::istream& in) {
    int n;
    in >> n;
    if (!in || n <= 0) return false;

    arr.clear();
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

    HANDLE hMinMax = CreateThread(NULL, 0, MinMaxThread, NULL, 0, NULL);
    HANDLE hAverage = CreateThread(NULL, 0, AverageThread, NULL, 0, NULL);

    WaitForSingleObject(hMinMax, INFINITE);
    WaitForSingleObject(hAverage, INFINITE);

    for (int i = 0; i < arr.size(); i++) {
        if (arr[i] == minVal || arr[i] == maxVal) {
            arr[i] = averageVal;
        }
    }


    cout << "Modified array:\n";
    for (double x : arr) {
        cout << x << " ";
    }
    cout << endl;

    CloseHandle(hMinMax);
    CloseHandle(hAverage);
    return 0;
}
