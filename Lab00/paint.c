/* File: paint.c 
   Author: Richard Hsin
   
*/
#define CAN_COVERAGE 200

#include <stdio.h>
#include <math.h>

/* If you do not use the Makefile provided and use gcc,
 * and if you continue to use the math.h library, you
 * will need to include -lm in your gcc compile statement
 * to load the math library */

/* Optional functions, uncomment the next two lines
 * if you want to create these functions after main: */
float readDimension(const char* name) {
    float value;
    printf("Enter %s (feet): ", name);
    scanf("%f", &value);
    return value;
}
float calcArea(float width, float height, float depth) {
    return 2 * width * height + 2 * width * depth + 2 * height * depth;
}

int main(int argc, char *argv[]) {
    float width, height, depth, total_area;
    int num_cans;
    width = readDimension("width");
    height = readDimension("height");
    depth = readDimension("depth");

    total_area = calcArea(width, height, depth);

    num_cans = (int)ceil(total_area / CAN_COVERAGE);

    printf("Total area to cover is: %.2f square feet\n", total_area);
    printf("Number of cans: %d\n", num_cans);
}