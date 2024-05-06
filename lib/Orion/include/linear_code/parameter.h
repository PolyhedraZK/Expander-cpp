#pragma once

//use the same distance parameter from brakedown
const double target_distance = 0.07;
const int distance_threshold = (int) (1.0 / target_distance) - 1;
const int rs_rate = 2;
const double alpha = 0.238, beta = 0.1205, r = 1.72;
const int cn = 10, dn = 20;
const int column_size = 128;