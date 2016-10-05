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
           int device_type=1, int device_id=0);


  void detect(std::string in_img, std::string out_img,
    std::vector<std::string> class_names, float visu_thresh,
    int max_disp_size, std::string save_result);
 private:
  PredictorHandle predictor_;
  std::vector<char> buffer_;
  unsigned int width_;
  unsigned int height_;
  float mean_r_;
  float mean_g_;
  float mean_b_;
};  // class Detector
}  //namespace det
