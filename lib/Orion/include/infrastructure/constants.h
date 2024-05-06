#pragma once
const int max_fri_depth = 30;
const int ldt_repeat_num = 33;
//2^log_slice_number slices, each slice contains N/2^{log_slice_number+1} elements, each element is a pair of 
//quad residue
const int log_slice_number = 6;
const int slice_number = 1 << log_slice_number;
const int rs_code_rate = 5;
const int max_bit_length = 30;

const int packed_size = 4; //4 unsigned long long packed
