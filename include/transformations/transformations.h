#ifndef TRANSFORMATIONS_H

#define DIMENSIONS 4

float* multiply_matrices(float mat1[], float mat2[]);
float* apply_translation(float translation_value_x, float translation_value_y, float translation_value_z);
float* apply_rotation(float rotation_value_z, float rotation_value_y, float rotation_value_x);
float* apply_scaling(float scaling_value_x, float scaling_value_y, float scaling_value_z);

#endif