//
// Created by jeret on 16/11/2022.
//
#include <stdio.h>
#include "math.h"

double movingaverage(double time, double acc_x, double acc_y, double acc_z);
double total_moved(double moving_average_acc_x,
                   double moving_average_acc_y,
                   double moving_average_acc_z);

double paikallaan[16][7] = {
        //(aika, acc_x, acc_y, acc_z, gyro_x, gyro_y, gyro_z
        {0,  0.01,  0.02, -0.98, 0.95, 0.53,  0.51},
        {1,  -0.00, 0.02, -0.98, 1.01, 0.84,  -0.51},
        {2,  -0.01, 0.02, -0.99, 1.03, 0.03,  -0.35},
        {3,  -0.00, 0.00, -1.00, 0.37, 0.03,  0.33},
        {4,  -0.00, 0.02, -1.00, 0.53, 0.01,  1.17},
        {5,  0.00,  0.02, -0.99, 0.63, 0.42,  0.67},
        {6,  0.01,  0.01, -0.97, 0.91, 1.23,  -0.02},
        {7,  -0.00, 0.02, -0.97, 1.25, 0.76,  0.20},
        {8,  -0.01, 0.02, -0.99, 1.43, 0.24,  0.84},
        {9,  -0.01, 0.03, -0.98, 0.92, 0.14,  0.20},
        {10, 0.01,  0.03, -1.00, 0.37, 0.72,  0.48},
        {11, -0.00, 0.02, -0.99, 0.93, 0.82,  1.21},
        {12, 0.02,  0.03, -0.96, 1.76, -0.10, 1.10},
        {13, -0.00, 0.04, -0.98, 1.87, -0.60, -0.22},
        {14, -0.00, 0.03, -0.99, 0.64, 0.96,  0.82},
        {15, -0.01, 0.04, -1.00, 1.16, 0.90,  1.81},
};
double nosto[16][7] = {
        {0,  0.01,  0.03, -0.98, 1.06,  0.37,  0.18},
        {1,  0.01,  0.03, -0.97, 1.40,  0.39,  0.48},
        {2,  0.02,  0.02, -0.98, 1.21,  0.46,  -0.28},
        {3,  0.02,  0.03, 0,     2.14,  1.28,  0.49},
        {4,  0.01,  0.03, 0,     1.50,  0.87,  0.53},
        {5,  0.01,  0.02, 0,     0.47,  0.66,  0.24},
        {6,  0.02,  0.03, 0,     1.17,  1.08,  0.50},
        {7,  0.01,  0.02, 0,     1.51,  0.27,  0.58},
        {8,  0.01,  0.03, 0,     1.22,  1.08,  0.44},
        {9,  0.02,  0.03, 0,     1.63,  1.00,  0.40},
        {10, 0.01,  0.03, 2, 1.11,  0.51,  0.44},
        {11, 0.01,  0.03, 3, 0.85,  0.01,  -0.13},
        {12, 0.00,  0.03, 4, -0.91, 1.05,  0.19},
        {13, 0.00,  0.02, 5, 1.06,  0.46,  0.33},
        {14, 0.02,  0.02, 6, 1.23,  0.32,  0.53},
        {15, -0.00, 0.03, 7, 1.93,  -1.07, -0.95,}
};

int main(){
    int i;
    for (i = 0; i < 16; i++) {
        movingaverage(nosto[i][0], nosto[i][1], nosto[i][2], nosto[i][3]);
    }
}

//aika, acc_x, acc_y, acc_z, gyro_x, gyro_y, gyro_z
double totaltime;
double total_acc_x;
double total_acc_y;
double total_acc_z;
double moving_average_acc_x;
double moving_average_acc_y;
double moving_average_acc_z;
double totalmoved;
double threshold = 1;

double movingaverage(double time, double acc_x, double acc_y, double acc_z) {
    totaltime += time;
    total_acc_x += fabs(acc_x);
    total_acc_y += fabs(acc_y);
    total_acc_z += fabs(acc_z);
    moving_average_acc_x = total_acc_x / totaltime;
    moving_average_acc_y = total_acc_y / totaltime;
    moving_average_acc_z = total_acc_z / totaltime;
    totalmoved = moving_average_acc_x + moving_average_acc_y + moving_average_acc_z;

    //printf("%f %f %f %f %f\n", time, totaltime, moving_average_acc_x, moving_average_acc_y, moving_average_acc_z);
    if (totalmoved > threshold) {
        return 1;
    }
    else {
        return 0;
    }
}