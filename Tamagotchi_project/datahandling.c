//
// Created by Tatters654 on 16/11/2022.
//
#include "math.h"
#include "stdio.h"

void moving_average(double time, double acc_x, double acc_y, double acc_z);

//  muokkaa [16] kohtaa jos halutaan lisää rivejä. Huom myös data_array_length, jota tulee nostaa saman arvoiseksi rivillä 41.
//  aika, acc_x, acc_y, acc_z
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

double totaltime;
double total_acc_x;
double total_acc_y;
double total_acc_z;
double moving_average_acc_x;
double moving_average_acc_y;
double moving_average_acc_z;
double totalmoved;
double threshold = 1; //maaginen konstantti sille thresholdille, että onko laite liikeessä

int main(){
    int i;
    int data_array_length = 15; // maaginen konstantti, tähän taulukon listojen määrä
    for (i = 0; i <= data_array_length; i++) {
        //  aika, acc_x, acc_y, acc_z
        moving_average(data[i][0], data[i][1], data[i][2], data[i][3]);
    }
    printf("%s %f \n", "Liikkeen keskiarvokiihtyvyys:", totalmoved);
    if (totalmoved > threshold) {
        printf("%s\n", "liikkuu");
        return 1; // Liikkuu
    }
    else {
        printf("%s\n", "ei liiku");
        return 0; // Ei liiku

    }
}

void moving_average(double time, double acc_x, double acc_y, double acc_z) {
    totaltime += time;
    total_acc_x += fabs(acc_x); //float absolute value
    total_acc_y += fabs(acc_y);
    total_acc_z += fabs(acc_z);
    moving_average_acc_x = total_acc_x / totaltime;
    moving_average_acc_y = total_acc_y / totaltime;
    moving_average_acc_z = total_acc_z / totaltime;
    totalmoved = moving_average_acc_x + moving_average_acc_y + moving_average_acc_z;
}