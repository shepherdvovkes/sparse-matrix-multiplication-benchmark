
#pragma once
#include<vector>
#include<iostream>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <chrono>
#include <random>

using namespace std;

class SparseFormat {
public: 
	vector<int> col_start_pos;
	vector<int> col_start_neg;
	vector<int> row_index_pos;
	vector<int> row_index_neg;

	SparseFormat(int* matrix, int K, int N) {
		int column_start_pos = 0;
		int column_start_neg = 0;
		for (int n = 0; n < N; n++) {
			this->col_start_pos.push_back(column_start_pos);
			this->col_start_neg.push_back(column_start_neg);
			for (int k = 0; k < K; k++) {
				if (matrix[k * N + n] >= 1) {
					column_start_pos++;
					this->row_index_pos.push_back(k);
				}
				else if (matrix[k * N + n] <= -1) {
					column_start_neg++;
					this->row_index_neg.push_back(k);
				}
			}
		}
		this->col_start_pos.push_back(column_start_pos);
		this->col_start_neg.push_back(column_start_neg);
	}
};

template <typename T>
vector<T> initX(int LEN, int Range) {
	vector<T> X(LEN, 0);
	mt19937 generator(static_cast<unsigned int>(time(0)));
	uniform_int_distribution<int> range(-Range, Range);
	for (int i = 0; i < LEN; i++) {
		X[i] = range(generator);
	}
	return X;
};

template <typename T>
vector<T> generateSparseMatrix(int H, int W, int nonZero, bool uniformDistribution) {
    vector<T> y = vector<T>(H * W, 0);
    if (uniformDistribution) {
        for (int h = 0; h < H; h++) {
            for (int w = 0; w < W; w += nonZero * 2) {
                // Assign +1, -1 to each 2 x nonZero slots
                int randomA = rand() % nonZero * 2;
                int randomB = rand() % nonZero * 2;
                y[h * W + w + randomA] = 1;
                while (randomA == randomB) {
                    randomB = rand() % nonZero * 2;
                }
                y[h * W + w + randomB] = -1;
            }
        }
    }
    else {
        mt19937 generator(static_cast<unsigned int>(time(0)));
        uniform_int_distribution<int> range(0, W - 1);
        uniform_int_distribution<int> variRange(0, int(W / nonZero / 20 + 1)); // The variation among different columns
        for (int h = 0; h < H; h++) {
            int posVari = variRange(generator);
            int limitPos = (W / nonZero) / 2 + posVari;
            int limitNeg = (W / nonZero) / 2 - posVari;

            // Assign +1 to W / nonZero / 2 places
            int count = 0;        
            while (count < limitPos) {
                int randomA = range(generator);
                if (y[h * W + randomA] == 0) {
                    y[h * W + randomA] = 1;
                    count++;
                }
            }

            // Assign -1 to W / nonZero / 2 places
            count = 0;
            while (count < limitNeg) {
                int randomA = range(generator);
                if (y[h * W + randomA] == 0) {
                    y[h * W + randomA] = -1;
                    count++;
                }
            }
        }
    }

    return y;
}

template <typename T>
void sparseGEMM(T* X, int * col_start_pos, int * col_start_neg, int * row_index_pos, int * row_index_neg, T* b, T* Y, int M, int N, int K) {
#pragma omp parallel for
	for (int m = 0; m < M; m++) {
		for (int n = 0; n < N; n++) {
			T y = 0;
			for (int k = col_start_pos[n]; k < col_start_pos[n + 1]; k++) {
				y += X[m * K + row_index_pos[k]];
			}
			for (int k = col_start_neg[n]; k < col_start_neg[n + 1]; k++) {
				y -= X[m * K + row_index_neg[k]];
			}
			Y[m * N + n] = y + b[n];
		}
	}
}

template <typename T>
void GEMM(T* X, T* W, T* b, T* Y, int M, int N, int K) {
#pragma omp parallel for
	for (int m = 0; m < M; m++) {
		for (int n = 0; n < N; n++) {
			T y = 0;
			for (int k = 0; k < K; k++) {
				y += X[m * K + k] * W[k * N + n];
			}
			Y[m * N + n] = y + b[n];
		}
	}
}

template <typename T>
void GEMM_PReLU(T* X, T* W, T* b, T* Y, int M, int N, int K, T a) {
#pragma omp parallel for
    for (int m = 0; m < M; m++) {
        for (int n = 0; n < N; n++) {
            T y = 0;
            for (int k = 0; k < K; k++) {
                y += X[m * K + k] * W[k * N + n];
            }
            y += b[n];

            Y[m * N + n] = (y < 0) ? a*y : y;
        }
    }
}

template <typename T>
void sparseGEMM_PReLU(T* X, int * col_start_pos, int * col_start_neg, int * row_index_pos, int * row_index_neg, T* b, T* Y, int M, int N, int K, T a) {
#pragma omp parallel for
    for (int m = 0; m < M; m++) {
        for (int n = 0; n < N; n++) {
            T y = 0;
            for (int k = col_start_pos[n]; k < col_start_pos[n + 1]; k++) {
                y += X[m * K + row_index_pos[k]];
            }
            for (int k = col_start_neg[n]; k < col_start_neg[n + 1]; k++) {
                y -= X[m * K + row_index_neg[k]];
            }
            y += b[n];

            Y[m * N + n] = (y < 0) ? a*y : y;
        }
    }
}


template <typename T> 
bool compare_results(T* result, T* groundTruth, int H, int W) {
	for (int h = 0; h < H; h++) {
		for (int w = 0; w < W; w++) {
			int i = h * W + w;
			if (abs(result[i] - groundTruth[i]) > 10e-6) {
				cout << "Error at: H=" << h << ", W=" << w << ", result=" << result[i] << ", groundTruth=" << groundTruth[i] << endl;
				return false;
			}
		}
	}

	return true;
}