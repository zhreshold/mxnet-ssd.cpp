/*!
 *  Copyright (c) 2015 by Joshua Zhang
 * \file detector.hpp
 * \brief ssd detection module impl
 */

#include "zupply.hpp"
#include "CImg.h"
#include "detector.hpp"
#include <iostream>

namespace det {
Detector::Detector(std::string model_prefix, int epoch, int width, int height,
                   float mean_r, float mean_g, float mean_b,
                   int device_type, int device_id) {
  if (epoch < 0 || epoch > 9999) {
    std::cerr << "Invalid epoch number: " << epoch << std::endl;
    exit(-1);
  }
  std::string model_file = model_prefix + "-" + zz::fmt::int_to_zero_pad_str(epoch, 4);
  if (!zz::os::is_file(model_file)) {
    std::cerr << "Model file: " << model_file << " does not exist" << std::endl;
    exit(-1);
  }
  std::string json_file = model_prefix + "-symbol.json";
  if (!zz::os::is_file(json_file)) {
    std::cerr << "JSON file: " << model_file << " does not exist" << std::endl;
    exit(-1);
  }
  if (width < 1 || height < 1) {
    std::cerr << "Invalid width or height: " << width << "," << height << std::endl;
    exit(-1);
  }
  width_ = width;
  height_ = height;
  input_shape_data_[0] = 1;
  input_shape_data_[1] = 3;
  input_shape_data_[2] = width;
  input_shape_data_[3] = height;
  mean_r_ = mean_r;
  mean_g_ = mean_g;
  mean_b_ = mean_b;

  // load parameters
  std::ifstream param_file("model_file", std::ios::binary | std::ios::ate);
  if (!param_file.is_open()) {
    std::cerr << "Unable to open model file: " << model_file << std::endl;
    exit(-1);
  }
  std::streamsize size = param_file.tellg();
  param_file.seekg(0, std::ios::beg);
  buffer_.resize(size);
  if (param_file.read(buffer.data(), size)) {
    MXPredCreate(json_file.c_str(), buffer_.data(), static_cast<int>(size),
      device_type, device_id, 1, input_nodes_, input_shape_indptr_, input_shape_data_
      &predictor_);
  } else {
    std::cerr << "Unable to read model file: " << model_file << std::endl;
    exit(-1);
  }
}

void Detector::detect(std::string in_img, std::string out_img) {
  if (!zz::os::is_file(in_img)) {
    std::cerr << "Image file: " << in_img << " does not exist" << std::endl;
    exit(-1);
  }
  zz::Image image(in_img);
  if (image.empty()) {
    std::cerr << "Unable to load image file: " << in_img << std::endl;
    exit(-1);
  }
  if (image.channels() != 3) {
    std::cerr << "RGB image required" << std::endl;
    exit(-1);
  }

  // resize image
  image.resize(width_, height_);
  int size = image.channels() * image.cols() * image.rows();
  std::vector<float> in_data(size);

  // de-interleave and minus means
  unsigned char *ptr = image.ptr();
  float *data_ptr = in_data.data();
  for (int i = 0; i < size; i +=3) {
    *(data_ptr++) = static_cast<float>(ptr[i]) - mean_r_;
  }
  for (int i = 1; i < size; i +=3) {
    *(data_ptr++) = static_cast<float>(ptr[i]) - mean_g_;
  }
  for (int i = 2; i < size; i +=3) {
    *(data_ptr++) = static_cast<float>(ptr[i]) - mean_b_;
  }

  mx_uint *shape = NULL;
  mx_uint shape_len = 0;
  MXPredSetInput(predictor_, "data", in_data.data(), static_cast<mx_uint>(size));
  MXPredForward(predictor_);
  MXPredGetOutputShape(predictor_, 0, &shape, &shape_len);
}

}
