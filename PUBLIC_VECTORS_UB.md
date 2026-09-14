# Public Test Vectors Containing Undefined Behavior

This can be updated with the script found [here](./tools/test_gen_utils/find_ub.sh).
**Note**: running this requires [ripgrep](https://github.com/burntsushi/ripgrep).

```
./Public-Tests/B01_synthetic/011_uninit_char_ptr_lib/test_vectors/bad.json:2:  "has_ub": "Program is killed after segmentation fault",
./Public-Tests/B01_synthetic/015_return_stack_buffer_lib/test_vectors/bad.json:2:  "has_ub": "Returns buffer allocated on the stack",
./Public-Tests/B01_synthetic/010_integer_overflow/test_vectors/overflow.json:4:  "has_ub": "Integer overflow"
./Public-Tests/B01_organic/update_md5_lib/test_vectors/1.json:22831:  "has_ub": "yes"
./Public-Tests/B02_synthetic/char_to_bool/test_vectors/67.json:11:  "has_ub": 1
./Public-Tests/B01_organic/pow43_lib/test_vectors/83.json:2:  "has_ub": "index -984 out of bounds",
./Public-Tests/B01_organic/pow43_lib/test_vectors/15.json:2:  "has_ub": "index -84 out of bounds",
./Public-Tests/B01_synthetic/012_uninit_int_ptr_lib/test_vectors/bad.json:2:  "has_ub": "Uses an uninitialized pointer",
./Public-Tests/B02_synthetic/char_to_bool/test_vectors/71.json:11:  "has_ub": 1
./Public-Tests/B01_organic/pow43_lib/test_vectors/14.json:2:  "has_ub": "index -48 out of bounds",
./Public-Tests/B01_synthetic/019_integer_overflow_char_max_multiply_lib/test_vectors/bad.json:2:  "has_ub": "This test exhibits integer overflow in C",
./Public-Tests/B01_organic/dequantize_granule_lib/test_vectors/1.json:2:  "has_ub": "Applying zero offset to NULL pointer",
./Public-Tests/B02_synthetic/char_to_bool/test_vectors/60.json:11:  "has_ub": 1
./Public-Tests/B01_organic/pow43_lib/test_vectors/84.json:2:  "has_ub": "index 1579 out of bounds",
./Public-Tests/B02_synthetic/char_to_bool/test_vectors/65.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/char_to_bool/test_vectors/58.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/char_to_bool/test_vectors/57.json:11:  "has_ub": 1
./Public-Tests/B01_organic/pow43_lib/test_vectors/30.json:2:  "has_ub": "index 528 out of bounds",
./Public-Tests/B02_synthetic/char_to_bool/test_vectors/61.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/char_to_bool/test_vectors/70.json:11:  "has_ub": 1
./Public-Tests/B01_organic/pow43_lib/test_vectors/81.json:2:  "has_ub": "index -113 out of bounds",
./Public-Tests/B02_synthetic/char_to_bool/test_vectors/62.json:11:  "has_ub": 1
./Public-Tests/B01_synthetic/015_return_stack_buffer/test_vectors/bad.json:4:  "has_ub": "Returns buffer allocated on the stack"
./Public-Tests/B02_synthetic/char_to_bool/test_vectors/64.json:11:  "has_ub": 1
./Public-Tests/B01_synthetic/022_stdlib_div_lib/test_vectors/test06.json:3:  "has_ub": "Divides by 0",
./Public-Tests/B02_synthetic/char_to_bool/test_vectors/59.json:11:  "has_ub": 1
./Public-Tests/B01_organic/pow43_lib/test_vectors/29.json:2:  "has_ub": "index 272 out of bounds",
./Public-Tests/B02_synthetic/char_to_bool/test_vectors/68.json:11:  "has_ub": 1
./Public-Tests/B01_organic/pow43_lib/test_vectors/82.json:2:  "has_ub": "index -184 out of bounds",
./Public-Tests/B02_synthetic/char_to_bool/test_vectors/63.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/char_to_bool/test_vectors/66.json:11:  "has_ub": 1
./Public-Tests/B01_synthetic/016_divide_by_zero_float/test_vectors/test04.json:4:  "has_ub": "Divides by zero: floating point"
./Public-Tests/B01_synthetic/019_integer_overflow_char_max_multiply/test_vectors/bad.json:4:  "has_ub": "This test exhibits integer overflow in C"
./Public-Tests/B02_synthetic/memcpy_fun_buffers/test_vectors/22.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/memcpy_fun_buffers/test_vectors/57.json:11:  "has_ub": 1
./Public-Tests/B01_synthetic/030_mutable_buffer_overlap_extrahard_lib/test_vectors/test04.json:2:  "has_ub": "Overflow",
./Public-Tests/B01_organic/bitwriter_add_lib/test_vectors/2.json:2:  "has_ub": "shift exponent 100 is too large for 64-bit type 'tflac_uint'",
./Public-Tests/B01_organic/bitwriter_add_lib/test_vectors/4.json:2:  "has_ub": "shift exponent 18446744073709551580 is too large for 64-bit type 'tflac_uint'",
./Public-Tests/B01_organic/ldexp_q2_lib/test_vectors/52.json:2:  "has_ub": "shift exponent -536870912 is negative",
./Public-Tests/B01_synthetic/009_stack_buffer_overflow/test_vectors/test09_overflow_int2.json:5:  "has_ub": "This test should segfault"
./Public-Tests/B02_synthetic/task_manager_lib/test_vectors/test07.json:2:  "has_ub": "MAX_TASKS = -1 which eventually gets passed to malloc",
./Public-Tests/B01_organic/ldexp_q2_lib/test_vectors/4.json:2:  "has_ub": "shift exponent -25 is negative",
./Public-Tests/B02_synthetic/task_manager_lib/test_vectors/test01.json:2:  "has_ub": "Derefernece NULL ptr",
./Public-Tests/B01_synthetic/009_stack_buffer_overflow/test_vectors/test07_overflow_buf2.json:4:  "has_ub": "Buffer overflow"
./Public-Tests/B01_organic/ldexp_q2_lib/test_vectors/3.json:2:  "has_ub": "shift exponent -3 is negative",
./Public-Tests/B01_organic/ldexp_q2_lib/test_vectors/2.json:2:  "has_ub": "shift exponent -1 is negative",
./Public-Tests/B01_organic/ldexp_q2_lib/test_vectors/5.json:2:  "has_ub": "shift exponent -13 is negative",
./Public-Tests/B01_synthetic/012_uninit_int_ptr/test_vectors/bad.json:4:  "has_ub": "Uses an uninitialized pointer"
./Public-Tests/B01_organic/ldexp_q2_lib/test_vectors/51.json:2:  "has_ub": "shift exponent -250 is negative",
./Public-Tests/B01_organic/read_side_info_lib/test_vectors/c1.json:2:  "has_ub": "Applying zero offset to NULL pointer",
./Public-Tests/B01_synthetic/009_stack_buffer_overflow_lib/test_vectors/test09_overflow_int2.json:4:  "has_ub": "This test should segfault",
./Public-Tests/B01_synthetic/009_stack_buffer_overflow_lib/test_vectors/test07_overflow_buf2.json:2:  "has_ub": "Buffer overflow",
./Public-Tests/B02_synthetic/doubleneg_lib/test_vectors/3.json:2: "has_ub": "Assignment is outside range of int representable values",
./Public-Tests/B02_synthetic/doubleneg_lib/test_vectors/2.json:2: "has_ub": "Assignment is outside range of int representable values",
./Public-Tests/B02_synthetic/doubleneg_lib/test_vectors/4.json:2: "has_ub": "Assignment is outside range of int representable values",
./Public-Tests/B02_synthetic/doubleneg_lib/test_vectors/1.json:2: "has_ub": "Assignment is outside range of int representable values",
./Public-Tests/B01_synthetic/010_integer_overflow_lib/test_vectors/overflow.json:2:  "has_ub": "Integer overflow",
./Public-Tests/B01_organic/tritanopia_lib/test_vectors/3.json:2:  "has_ub": "-55.5838 is outside the range of representable values of type 'unsigned char'",
./Public-Tests/B02_synthetic/mutable_duplication_dag/test_vectors/multiple_deletes.json:10:  "has_ub": "heap uaf per asan",
./Public-Tests/B02_synthetic/strcmp/test_vectors/test_0003_empty_input.json:10:  "has_ub": "output depends on unitialized stack memory with empty input",
./Public-Tests/B02_synthetic/mutable_duplication_dag/test_vectors/delete_node.json:11:  "has_ub": 1
./Public-Tests/B01_organic/hdr_bitrate_lib/test_vectors/101.json:2:  "has_ub": "index 15 out of bounds",
./Public-Tests/B01_synthetic/022_stdlib_div/test_vectors/test06.json:5:  "has_ub": "Divides by 0"  
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/stress_maximum_complexity.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/mixed_valid_invalid.json:11:  "has_ub": 1
./Public-Tests/B01_organic/hdr_bitrate_lib/test_vectors/112.json:2:  "has_ub": "index -1 out of bounds",
./Public-Tests/B01_synthetic/016_divide_by_zero_float_lib/test_vectors/test04.json:2:  "has_ub": "Divides by zero: floating point",
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/add_multiple_shapes.json:11:  "has_ub": 1
./Public-Tests/B01_organic/hdr_bitrate_lib/test_vectors/117.json:2:  "has_ub": "index -1 out of bounds",
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/use_after_delete_view.json:11:  "has_ub": 1
./Public-Tests/B01_synthetic/030_mutable_buffer_overlap_extrahard/test_vectors/test04.json:4:  "has_ub": "Overflow"
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/stress_multiple_operations.json:11:  "has_ub": 1
./Public-Tests/B01_synthetic/030_mutable_buffer_overlap_extrahard/test_vectors/test06.json:4:  "has_ub": "Overflow"
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/all_menu_options.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/singleton_reuse_50_times.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/complex_use_after_delete.json:11:  "has_ub": 1
./Public-Tests/B01_organic/hdr_bitrate_lib/test_vectors/96.json:2:  "has_ub": "index 15 out of bounds",
./Public-Tests/B01_synthetic/011_uninit_char_ptr/test_vectors/bad.json:4:  "has_ub": "Program is killed after segmentation fault"
./Public-Tests/B02_synthetic/strcmp/test_vectors/test_0084_set_name_overflow.json:10:  "has_ub": "buffer overflow",
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/exceed_max_shapes_in_scene.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/compare_same_shape.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/compare_scenes_different_counts.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/compare_different_scenes.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/strcmp/test_vectors/test_0078_adduser_name_overflow.json:10:  "has_ub": "buffer overflow",
./Public-Tests/B01_organic/hdr_bitrate_lib/test_vectors/64.json:2:  "has_ub": "index 15 out of bounds",
./Public-Tests/B01_synthetic/017_signed_length_confusion/test_vectors/test08.json:2:  "has_ub": "Possible buffer overflow",
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/complex_scene_comparison.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/strcmp/test_vectors/test_0080_adduser_both_overflow.json:10:  "has_ub": "buffer overflow",
./Public-Tests/B01_synthetic/017_signed_length_confusion/test_vectors/test01.json:4:  "has_ub": "buffer overflow or fail in call to fgets",
./Public-Tests/B02_synthetic/strcmp/test_vectors/test_0079_adduser_pass_overflow.json:10:  "has_ub": "buffer overflow",
./Public-Tests/B02_synthetic/strcpy/test_vectors/41.json:11:  "has_ub": 1
./Public-Tests/B01_organic/hdr_bitrate_lib/test_vectors/80.json:2:  "has_ub": "index 15 out of bounds",
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/compare_different_shapes.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/remove_all_shapes.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/strcpy/test_vectors/19.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/view_scene_basic.json:11:  "has_ub": 1
./Public-Tests/B01_organic/hdr_bitrate_lib/test_vectors/48.json:2:  "has_ub": "index 15 out of bounds",
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/compare_loaded_original.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/compare_identical_scenes.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/strcpy/test_vectors/58.json:10:  "has_ub": "asan reports stack buffer overflow",
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/interleaved_operations.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/strcpy/test_vectors/61.json:10:  "has_ub": "stack buffer overflow per asan",
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/save_load_preservation.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/strcpy/test_vectors/20.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/add_shape_basic.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/strcpy/test_vectors/42.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/save_then_load.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/strcpy/test_vectors/59.json:10:  "has_ub": "different output with asan vs normal compilation but no error",
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/delete_then_all_ops.json:11:  "has_ub": 1
./Public-Tests/B01_organic/hdr_bitrate_lib/test_vectors/110.json:2:  "has_ub": "index -1 out of bounds",
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/exactly_50_shapes.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/remove_shape_basic.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/strcpy/test_vectors/7.json:11:  "has_ub": 1
./Public-Tests/B01_organic/hdr_bitrate_lib/test_vectors/16.json:2:  "has_ub": "index 15 out of bounds",
./Public-Tests/B02_synthetic/strcpy/test_vectors/56.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/remove_shape_invalid_index.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/strcpy/test_vectors/31.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/add_max_shapes_to_scene.json:11:  "has_ub": 1
./Public-Tests/B01_organic/hdr_bitrate_lib/test_vectors/113.json:2:  "has_ub": "index -1 out of bounds",
./Public-Tests/B02_synthetic/strcpy/test_vectors/32.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/save_scene_basic.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/add_all_shape_types.json:11:  "has_ub": 1
./Public-Tests/B01_organic/hdr_bitrate_lib/test_vectors/32.json:2:  "has_ub": "index 15 out of bounds",
./Public-Tests/B02_synthetic/strcpy/test_vectors/55.json:11:  "has_ub": "1"
./Public-Tests/B02_synthetic/strcpy/test_vectors/33.json:10:  "has_ub": "different output when compiled with Asan, even though no reported error",
./Public-Tests/B02_organic/rdg_genstdout_lib/test_vectors/test09.json:2:  "has_ub": "arithmetic overflow",
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/fill_remove_fill.json:11:  "has_ub": 1
./Public-Tests/B02_synthetic/memmove/test_vectors/72.json:11:  "has_ub": 1
./Public-Tests/B01_organic/hdr_bitrate_lib/test_vectors/111.json:2:  "has_ub": "index -1 out of bounds",
./Public-Tests/B02_synthetic/pointer_comparison_ascii_art/test_vectors/compare_all_shape_combinations.json:11:  "has_ub": 1
./Public-Tests/B02_organic/parse_uname_lib/test_vectors/test01.json:2:  "has_ub": "Calls strstr(uname, ...) where uanme is NULL",
./Public-Tests/B01_organic/hdr_bitrate_lib/test_vectors/115.json:2:  "has_ub": "index -1 out of bounds",
./Public-Tests/B01_organic/hdr_bitrate_lib/test_vectors/119.json:2:  "has_ub": "index 15 out of bounds",
./Public-Tests/B02_synthetic/arity_lib/test_vectors/3.json:20:"has_ub": "yes"}
./Public-Tests/B02_synthetic/arity_lib/test_vectors/5.json:20:"has_ub": "yes"}
./Public-Tests/B02_synthetic/arity_lib/test_vectors/2.json:20:"has_ub": "yes"}
./Public-Tests/B02_synthetic/arity_lib/test_vectors/4.json:20:"has_ub": "yes"}
./Public-Tests/B01_organic/merge_sort_lib/test_vectors/2.json:2:  "has_ub": "Dereferences NULL pointer",
./Public-Tests/B01_organic/gaussian_kernel_lib/test_vectors/3.json:2:  "has_ub": "heap-buffer-overflow",
./Public-Tests/B02_synthetic/matrix_mult_lib/test_vectors/test05.json:2:  "has_ub": "Causes integer overflow",
./Public-Tests/B02_synthetic/matrix_mult_lib/test_vectors/test01.json:2:  "has_ub": "Calls strdup(NULL)",
./Public-Tests/B02_synthetic/goto_lib/test_vectors/test01.json:2:  "has_ub": "Calls fopen with NULL",
./Public-Tests/B02_organic/encode_base64_lib/test_vectors/test10.json:2:  "has_ub": "Triggers call of strlen with string without null terminator",
./Public-Tests/B02_synthetic/container_of/test_vectors/test0.json:2:  "has_ub": "Member access within null pointer",
./Public-Tests/B02_synthetic/modeselect_lib/test_vectors/3.json:2: "has_ub": "signed integer overflow",
./Public-Tests/B02_synthetic/modeselect_lib/test_vectors/5.json:2: "has_ub": "signed integer overflow",
./Public-Tests/B02_synthetic/modeselect_lib/test_vectors/2.json:2: "has_ub": "signed integer overflow",
./Public-Tests/B02_synthetic/modeselect_lib/test_vectors/4.json:2: "has_ub": "signed integer overflow",
./Public-Tests/B02_organic/decode_base64_lib/test_vectors/test10.json:2:  "has_ub": "Triggers call of strlen with string without null terminator",
./Public-Tests/B02_organic/search_and_replace_lib/test_vectors/test12.json:2:  "has_ub": "Calls strlen(orig) where orig is NULL",
./Public-Tests/B02_synthetic/modeselect_lib/test_vectors/1.json:2: "has_ub": "signed integer overflow",
./Public-Tests/B02_organic/utf8_lib/test_vectors/test07.json:2:  "has_ub": "Not UB but assert(string != NULL) triggers IOT instruction core dump",
./Public-Tests/B02_organic/search_and_replace_lib/test_vectors/test04.json:2:  "has_ub": "search being empty causes infinite loop",
./Public-Tests/B02_organic/search_and_replace_lib/test_vectors/test14.json:2:  "has_ub": "Calls strlen(orig) where orig isn't NULL terminated",
./Public-Tests/B02_organic/unfilter_lib/test_vectors/0.json:2:  "has_ub": "Applying zero offset to NULL pointer",
./Public-Tests/B02_organic/search_and_replace_lib/test_vectors/test10.json:2:  "has_ub": "Calls strlen(search) where search is NULL",
./Public-Tests/B02_organic/utf8_lib/test_vectors/test06.json:2:  "has_ub": "Calls strlen(string) where string doesn't have NULL termination",
./Public-Tests/B02_organic/search_and_replace_lib/test_vectors/test11.json:2:  "has_ub": "Calls strlen(value) where value is NULL",
```
