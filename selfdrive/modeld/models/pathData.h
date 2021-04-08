#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <eigen3/Eigen/Dense>

#include "common/timing.h"
#include "common/params.h"
#include "clutil.h"

const int  POLYFIT_DEGREE = 4;
const int MODEL_PATH_DISTANCE = 192;
const int MIN_VALID_LEN = 10;


Eigen::Matrix<float, MODEL_PATH_DISTANCE, POLYFIT_DEGREE - 1> vander;

void  model_matrix_init() 
{
  // Build Vandermonde matrix
  for(int i = 0; i < TRAJECTORY_SIZE; i++) {
    for(int j = 0; j < POLYFIT_DEGREE - 1; j++) {
      //X_IDXS[i] = (TRAJECTORY_DISTANCE/1024.0) * (pow(i,2));
      //T_IDXS[i] = (TRAJECTORY_TIME/1024.0) * (pow(i,2));
      vander(i, j) = pow(X_IDXS[i], POLYFIT_DEGREE-j-1);
    }
  }
}

void poly_fit(float *in_pts, float *in_stds, float *out, int valid_len) 
{
  // References to inputs
  Eigen::Map<Eigen::Matrix<float, Eigen::Dynamic, 1> > pts(in_pts, valid_len);
  Eigen::Map<Eigen::Matrix<float, Eigen::Dynamic, 1> > std(in_stds, valid_len);
  Eigen::Map<Eigen::Matrix<float, POLYFIT_DEGREE - 1, 1> > p(out, POLYFIT_DEGREE - 1);

  float y0 = pts[0];
  pts = pts.array() - y0;

  // Build Least Squares equations
  Eigen::Matrix<float, Eigen::Dynamic, POLYFIT_DEGREE - 1> lhs = vander.topRows(valid_len).array().colwise() / std.array();
  Eigen::Matrix<float, Eigen::Dynamic, 1> rhs = pts.array() / std.array();

  // Improve numerical stability
  Eigen::Matrix<float, POLYFIT_DEGREE - 1, 1> scale = 1. / (lhs.array()*lhs.array()).sqrt().colwise().sum();
  lhs = lhs * scale.asDiagonal();

  // Solve inplace
  p = lhs.colPivHouseholderQr().solve(rhs);

  // Apply scale to output
  p = p.transpose() * scale.asDiagonal();
  out[3] = y0;
}



void fill_path(cereal::ModelDataV2::PathData::Builder path, const float *data, const float prob,
               float valid_len, int valid_len_idx, int ll_idx) {
  float points[TRAJECTORY_SIZE] = {};
  float stds[TRAJECTORY_SIZE] = {};
  float poly[POLYFIT_DEGREE] = {};

  for (int i=0; i<TRAJECTORY_SIZE; i++) {
    // negative sign because mpc has left positive
    if (ll_idx == 0) {
      points[i] = -data[30 * i + 16];
      stds[i] = exp(data[30 * (33 + i) + 16]);
    } else {
      points[i] = -data[2 * 33 * ll_idx + 2 * i];
      stds[i] = exp(data[2 * 33 * (4 + ll_idx) + 2 * i]);
    }
  }
  const float std = stds[0];
  poly_fit(points, stds, poly, valid_len_idx);

  path.setPoly(poly);
  path.setProb(prob);
  path.setStd(std);
  path.setValidLen(valid_len);
}