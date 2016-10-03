/*!
 *  Copyright (c) 2015 by Joshua Zhang
 * \file detector.hpp
 * \brief ssd detection module header
 */

#include "c_predict_api.h"
#include <string>
#include <vector>

namespace det {
class Detector {
public:
  Detector(std::string model_prefix, int epoch, int width, int height,
           float mean_r, float mean_g, float mean_b,
           int device_type=1, int device_id=1);

  void detect(std::string in_img, std::string out_img);
private:
  PredictorHandle predictor_;
  std::vector<char> buffer_;
  const char *input_nodes_[] = {"data"};
  int width_;
  int height_;
  const mx_uint input_shape_indptr_[] = {0, 4};
  const mx_uint input_shape_data_[4];
  float mean_r_;
  float mean_g_;
  float mean_b_;
}
}
