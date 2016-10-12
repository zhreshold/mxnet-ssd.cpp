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

  std::vector<float> detect(std::string in_img);
  std::vector<float> detect(const char *in_img) {
    return detect(std::string(in_img));
  }

 private:
  PredictorHandle predictor_;
  std::vector<char> buffer_;
  unsigned int width_;
  unsigned int height_;
  float mean_r_;
  float mean_g_;
  float mean_b_;
};  // class Detector

void visualize_detection(std::string img_path,
               std::vector<float> &detections,
               float visu_thresh,
               int max_disp_size,
               std::vector<std::string> class_names = {},
               std::string out_file = "");

void save_detection_results(std::string out_file,
                     std::vector<float> &detections,
                     std::vector<std::string> class_names = {},
                     float thresh = 0);

std::vector<std::string> load_class_map(std::string map_file);
}  //namespace det
