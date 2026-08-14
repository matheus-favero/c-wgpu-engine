#include <math.h>
#include <stdlib.h>
#include <string.h>

#define DIMENSIONS 4
#define ARRAY_SIZE DIMENSIONS *DIMENSIONS

float *multiply_matrices(float mat1[ARRAY_SIZE], float mat2[ARRAY_SIZE]) {
  float *result = malloc(sizeof(float) * ARRAY_SIZE);
  float n1 = 0;
  float n2 = 0;
  float line_column_result = 0;
  int result_index = 0;

  for (int line_index = 0; line_index < DIMENSIONS; line_index++) {
    line_column_result = 0;
    for (int column_index = 0; column_index < DIMENSIONS; column_index++) {
      for (int number_index = 0; number_index < DIMENSIONS; number_index++) {
        n1 = mat1[DIMENSIONS * line_index + number_index];
        n2 = mat2[DIMENSIONS * number_index + column_index];

        line_column_result += n1 * n2;
      }
      result_index = line_index * DIMENSIONS + column_index;
      result[result_index] = line_column_result;
      line_column_result = 0;
    }
  }

  return result;
}
// the idea behind this function is to normalize Z axis in our matrices.
// Webgpu only allows matrices to be forward 0 in Z axis
void normalize_matrix(float *matrix) {
  for (int i = 2; i < ARRAY_SIZE; i += DIMENSIONS) {
    matrix[i] = matrix[i] + 1;
    matrix[i] = matrix[i] / 2;
    matrix[i] = matrix[i] * 0.1;
  }
}

float *apply_translation(float translation_value_x, float translation_value_y,
                         float translation_value_z) {

  float *result = malloc(sizeof(float) * ARRAY_SIZE);
  float translation_matrix[DIMENSIONS * DIMENSIONS] = {1,
                                                       0,
                                                       0,
                                                       0,
                                                       0,
                                                       1,
                                                       0,
                                                       0,
                                                       0,
                                                       0,
                                                       1,
                                                       0,
                                                       translation_value_x,
                                                       translation_value_y,
                                                       translation_value_z,
                                                       translation_value_z * 2};

  memcpy(result, translation_matrix, sizeof(float) * ARRAY_SIZE);
  return result;
}

float *apply_rotation(float rotation_value_z, float rotation_value_y,
                      float rotation_value_x) {
  float *result = malloc(sizeof(float) * ARRAY_SIZE);

  float z_rotation_matrix[ARRAY_SIZE] = {cos(rotation_value_z),
                                         sin(rotation_value_z),
                                         0,
                                         0,
                                         -sin(rotation_value_z),
                                         cos(rotation_value_z),
                                         0,
                                         0,
                                         0,
                                         0,
                                         1,
                                         0,
                                         0,
                                         0,
                                         0,
                                         1};

  float y_rotation_matrix[ARRAY_SIZE] = {
      cos(rotation_value_y),  0, sin(rotation_value_y), 0, 0, 1, 0, 0,
      -sin(rotation_value_y), 0, cos(rotation_value_y), 0, 0, 0, 0, 1};

  float x_rotation_matrix[ARRAY_SIZE] = {1,
                                         0,
                                         0,
                                         0,
                                         0,
                                         cos(rotation_value_x),
                                         sin(rotation_value_x),
                                         0,
                                         0,
                                         -sin(rotation_value_x),
                                         cos(rotation_value_x),
                                         0,
                                         0,
                                         0,
                                         0,
                                         1};

  // here we mix all rotations into a single array
  memcpy(result, multiply_matrices(z_rotation_matrix, y_rotation_matrix),
         sizeof(float) * ARRAY_SIZE);
  memcpy(result, multiply_matrices(result, x_rotation_matrix),
         sizeof(float) * ARRAY_SIZE);

  return result;
}

float *apply_scaling(float scaling_value_x, float scaling_value_y,
                     float scaling_value_z) {
  float *result = malloc(sizeof(float) * ARRAY_SIZE);

  float scaling_matrix[ARRAY_SIZE] = {
      scaling_value_x,
      0,
      0,
      0,
      0,
      scaling_value_y,
      0,
      0,
      0,
      0,
      scaling_value_z,
      0,
      0,
      0,
      0,
      1,
  };
  memcpy(result, scaling_matrix, sizeof(float) * ARRAY_SIZE);

  return result;
}

void apply_perspective(float *matrix) {
  float perspective[ARRAY_SIZE] = {1, 0, 0, 0, 0, 1, 0, 0,
                                   0, 0, 1, 1, 0, 0, 0, 1};
  multiply_matrices(matrix, perspective);
}