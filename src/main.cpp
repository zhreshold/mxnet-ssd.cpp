/*!
 *  Copyright (c) 2015 by Joshua Zhang
 * \file detect.cpp
 * \brief object detector, based on MXNet and Single Shot Multibox detector
 */

#include "zupply.hpp"
#include <iostream>


int main(int argc, char **argv) {
  /* parse arguments */
  zz::cfg::ArgParser parser;
  parser.add_opt_help('h', "help");
  parser.add_opt_version('v', "version", "0.1");

  parser.parse(argc, argv);
  // check errors
  if (parser.count_error() > 0) {
    std::cout << parser.get_error() << std::endl;
    std::cout << parser.get_help() << std::endl;
    exit(-1);
  }

  return 0;
}
