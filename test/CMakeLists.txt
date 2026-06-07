function(st_add_test name)
    add_executable(${name} ${name}.cpp)
    target_include_directories(${name} PRIVATE
        ${CMAKE_SOURCE_DIR}/dsp
        ${CMAKE_SOURCE_DIR}/io
        ${CMAKE_SOURCE_DIR}/plugin
        ${CMAKE_SOURCE_DIR}/third_party)
    target_compile_features(${name} PRIVATE cxx_std_17)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

# Pure-header DSP / config tests.
st_add_test(test_dsp_math)
st_add_test(test_highpass)
st_add_test(test_resampler)
st_add_test(test_sampler)
st_add_test(test_config)

# Tests that need the AudioFile implementation TU.
foreach(t test_audiofile test_plugin_offline)
    add_executable(${t} ${t}.cpp ${CMAKE_SOURCE_DIR}/io/AudioFile.cpp)
    target_include_directories(${t} PRIVATE
        ${CMAKE_SOURCE_DIR}/dsp
        ${CMAKE_SOURCE_DIR}/io
        ${CMAKE_SOURCE_DIR}/third_party)
    target_compile_features(${t} PRIVATE cxx_std_17)
    add_test(NAME ${t} COMMAND ${t})
endforeach()
