#pragma once
#include "pch.h"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

using namespace std;

extern vector<double> arr;
extern double minVal, maxVal;
extern double averageVal;

void MinMaxThread();

void AverageThread();

bool ReadArrayFromStream(istream& in);