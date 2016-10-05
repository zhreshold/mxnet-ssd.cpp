/*!
 *  Copyright (c) 2015 by Joshua Zhang
 * \file detect.cpp
 * \brief object detector, based on MXNet and Single Shot Multibox detector
 */

#include "zupply.hpp"
#include "detector.hpp"
#include <iostream>
#include <vector>
#include <string>


int main(int argc, char **argv) {
  std::string out_name;
  std::string model_prefix;
  const char *default_model = "deploy_ssd_300";
  int epoch;
  int width;
  int height;
  float mean_r;
  float mean_g;
  float mean_b;
  float visu_thresh;
  int gpu_id;
  int max_disp_size;
  std::string result_file;
  std::vector<std::string> class_names = {
     "aeroplane", "bicycle", "bird", "boat",
     "bottle", "bus", "car", "cat", "chair",
     "cow", "diningtable", "dog", "horse",
     "motorbike", "person", "pottedplant",
     "sheep", "sofa", "train", "tvmonitor"};

  /* parse arguments */
  zz::cfg::ArgParser parser;
  parser.add_opt_help('h', "help");
  parser.add_opt_version('v', "version", "0.1");
  parser.add_opt_value('o', "out", out_name, std::string(), "output detection result to image", "FILE");
  parser.add_opt_value('m', "model", model_prefix, std::string(default_model), "load model prefix", "FILE");
  parser.add_opt_value('e', "epoch", epoch, 1, "load model epoch", "INT");
  parser.add_opt_value(-1, "width", width, 300, "resize width", "INT");
  parser.add_opt_value(-1, "height", height, 300, "resize height", "INT");
  parser.add_opt_value('r', "red", mean_r, 123.f, "red mean pixel value", "FLOAT");
  parser.add_opt_value('g', "green", mean_g, 117.f, "green mean pixel value", "FLOAT");
  parser.add_opt_value('b', "blue", mean_b, 104.f, "blue mean pixel value", "FLOAT");
  parser.add_opt_value('t', "thresh", visu_thresh, 0.5f, "visualize threshold", "FLOAT");
  parser.add_opt_value(-1, "gpu", gpu_id, -1, "gpu id to detect with, default use cpu", "INT");
  parser.add_opt_value(-1, "disp-size", max_disp_size, 640, "display size, longer side", "INT");
  parser.add_opt_value(-1, "save-result", result_file, std::string(), "save result in text file", "FILE");
  zz::cfg::ArgOption& input = parser.add_opt(-1, "").set_type("FILE")
    .set_help("input image").set_min(1).set_max(1).require();

  parser.parse(argc, argv);
  // check errors
  if (parser.count_error() > 0) {
    std::cout << parser.get_error() << std::endl;
    std::cout << parser.get_help() << std::endl;
    exit(-1);
  }

  // create detector
  int device_type = 1;
  int device_id = 0;
  if (gpu_id > -1) {
    device_type = 2;
    device_id = gpu_id;
  }
  det::Detector detector(model_prefix, epoch, width, height,
    mean_r, mean_g, mean_b, device_type, device_id);
  // detect image
  detector.detect(input.get_value().str(), out_name, class_names, visu_thresh, max_disp_size, result_file);

  return 0;
}
