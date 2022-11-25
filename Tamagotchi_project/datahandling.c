//
// Created by jeret on 16/11/2022.
//
#include <stdio.h>
#include "math.h"
#include "stdlib.h"

double movingaverage(double time, double acc_x, double acc_y, double acc_z);
double total_moved(double moving_average_acc_x,
                   double moving_average_acc_y,
                   double moving_average_acc_z);

double data[16][7] = {
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

int main(){
    int i; // viimeisen datan indeksi
    int data_window; // data ikkuna ts. montako datapistettä lasketaan taaksepäin
    int last_index = abs(last_data_index - data_window); // laskee viimeisen data pisteen indeksin, jos se on scrollaantunut pois
    for (i = i, i <= last_index, i++) {
        movingaverage(data[i][0], data[i][1], data[i][2], data[i][3]);
    }
    //printf("%f %f %f %f %f\n", time, totaltime, moving_average_acc_x, moving_average_acc_y, moving_average_acc_z);
    if (totalmoved > threshold) {
        return 1; // Liikkuu, voi vaihtaa ENUM:iksi jos halaa
    }
    else {
        return 0; // Ei liiku
    }
}

double movingaverage(double time, double acc_x, double acc_y, double acc_z) {
    totaltime += time;
    total_acc_x += fabs(acc_x);
    total_acc_y += fabs(acc_y);
    total_acc_z += fabs(acc_z);
    moving_average_acc_x = total_acc_x / totaltime;
    moving_average_acc_y = total_acc_y / totaltime;
    moving_average_acc_z = total_acc_z / totaltime;
    totalmoved = moving_average_acc_x + moving_average_acc_y + moving_average_acc_z;

}