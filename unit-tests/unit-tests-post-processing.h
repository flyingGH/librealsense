// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

// Definitions and utility functions to load and parse prerecorded frames data
// utilized in post-processing filters validation
// The file is intended to be used by both librealsense and 3rd party tools
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <numeric>

struct ppf_test_config
{
    std::string name = "";
    bool        spatial_filter = false;
    float       spatial_alpha = 0.f;
    uint8_t     spatial_delta = 0;
    int         spatial_iterations;
    bool        temporal_filter = false;
    float       temporal_alpha = 0.f;
    uint8_t     temporal_delta = 0;
    uint8_t     temporal_persistence = 0;
    bool        holes_filter = false;
    int         holes_filling_mode = 0;
    int         downsample_scale = 1;
    float       depth_units = 0.001f;
    float       stereo_baseline = 0.f;
    float       focal_length = 0.f;
    uint32_t    input_res_x = 0;
    uint32_t    input_res_y = 0;
    uint32_t    output_res_x = 0;
    uint32_t    output_res_y = 0;

    size_t      frames_sequence_size = 1;

    std::vector<std::string> input_frame_names;
    std::vector<std::string> output_frame_names;

    std::vector<std::vector<uint8_t>> _input_frames; // stores the actul pixel values for the frames sequence
    std::vector<std::vector<uint8_t>> _output_frames;

    void reset()
    {
        name.clear();
        spatial_filter = temporal_filter = holes_filter = false;
        spatial_alpha = temporal_alpha = 0;
        spatial_iterations = temporal_persistence = holes_filling_mode = spatial_delta = temporal_delta = 0;
        input_res_x = input_res_y = output_res_x = output_res_y = 0;
        downsample_scale = 1;
        _input_frames.clear();
        _output_frames.clear();
    }
};


std::vector<uint8_t> load_from_binary(const std::string& str)
{
    std::ifstream file(str.c_str(), std::ios::binary);

    // Determine the file length
    file.seekg(0, std::ios_base::end);
    std::size_t size = file.tellg();
    file.seekg(0, std::ios_base::beg);

    // Create a vector to store the data
    std::vector<uint8_t> v(size);

    // Load the data
    file.read((char*)&v[0], size);
    // Close the file
    file.close();

    return v;
}

enum metadata_attrib : uint8_t
{
    res_x,
    res_y,
    focal_length,
    depth_units,
    stereo_baseline,
    downscale,
    spat_filter,
    spat_alpha,
    spat_delta,
    spat_iter,
    temp_filter,
    temp_alpha,
    temp_delta,
    temp_persist,
    holes_filter,
    holes_fill,
    frames_sequence_size
};

// The table establishes the mapping of attributes name to be found in the input files
// generated by the reference viewer (realsense2ex_test.cpp)
const std::map<metadata_attrib, std::string> metadata_attributes = {
    { res_x,        "Resolution_x" },
    { res_y,        "Resolution_y" },
    { focal_length,  "Focal Length" },
    { depth_units,   "Depth Units" },
    { stereo_baseline,"Stereo Baseline" },
    { downscale,    "Scale" },
    { spat_filter,   "Spatial Filter Params:" },
    { spat_alpha,   "SpatialAlpha" },
    { spat_delta,   "SpatialDelta" },
    { spat_iter,    "SpatialIterations" },
    { temp_filter,  "Temporal Filter Params:" },
    { temp_alpha,   "TemporalAlpha" },
    { temp_delta,   "TemporalDelta" },
    { temp_persist, "TemporalPersistency" },
    { holes_filter, "Holes Filling Mode:" },
    { holes_fill,   "HolesFilling" },
    { frames_sequence_size, "Frames sequence length" }
};

// Parse frame's metadata files and partially fill the configuration struct
inline ppf_test_config attrib_from_csv(const std::string& str)
{
    // First, fill all key/value pair into and appropriate container
    std::map<std::string, std::string> dict;

    std::ifstream  data(str.c_str());
    std::string line;
    int invalid_line = 0;

    std::string key, value;
    while (true)
    {
        if (!std::getline(data, key, ','))
            invalid_line++;

        std::getline(data, value, '\n');
        std::stringstream ss(value); // trim EOL discrepancies
        ss >> value;
        dict[key] = value;

        if (invalid_line > 1)  // Two or more non-kvp lines designate eof. Note this when creating the attributes
            break;
    }

    // Then parse the data according to the predefined set of attribute names
    ppf_test_config cfg;

    // Note that the function does not differentiate between input and output frames metadata csv.
    // The user must assign the parameters appropriately
    cfg.input_res_x = dict.count(metadata_attributes.at(res_x)) ? std::stoi(dict.at(metadata_attributes.at(res_x))) : 0;
    cfg.input_res_y = dict.count(metadata_attributes.at(res_y)) ? std::stoi(dict.at(metadata_attributes.at(res_y))) : 0;
    cfg.stereo_baseline = dict.count(metadata_attributes.at(stereo_baseline)) ? std::stof(dict.at(metadata_attributes.at(stereo_baseline))) : 0.f;
    cfg.depth_units = dict.count(metadata_attributes.at(depth_units)) ? std::stof(dict.at(metadata_attributes.at(depth_units))) : 0.f;
    cfg.focal_length = dict.count(metadata_attributes.at(focal_length)) ? std::stof(dict.at(metadata_attributes.at(focal_length))) : 0.f;

    cfg.downsample_scale = dict.count(metadata_attributes.at(downscale)) ? std::stoi(dict.at(metadata_attributes.at(downscale))) : 1;
    cfg.spatial_filter = dict.count(metadata_attributes.at(spat_filter)) > 0;
    cfg.spatial_alpha = dict.count(metadata_attributes.at(spat_alpha)) ? std::stof(dict.at(metadata_attributes.at(spat_alpha))) : 0.f;
    cfg.spatial_delta = dict.count(metadata_attributes.at(spat_delta)) ? std::stoi(dict.at(metadata_attributes.at(spat_delta))) : 0;
    cfg.spatial_iterations = dict.count(metadata_attributes.at(spat_iter)) ? std::stoi(dict.at(metadata_attributes.at(spat_iter))) : 0;
    cfg.temporal_filter = dict.count(metadata_attributes.at(temp_filter)) > 0;
    cfg.temporal_alpha = dict.count(metadata_attributes.at(temp_alpha)) ? std::stof(dict.at(metadata_attributes.at(temp_alpha))) : 0.f;
    cfg.temporal_delta = dict.count(metadata_attributes.at(temp_delta)) ? std::stoi(dict.at(metadata_attributes.at(temp_delta))) : 0;
    cfg.temporal_persistence = dict.count(metadata_attributes.at(temp_persist)) ? std::stoi(dict.at(metadata_attributes.at(temp_persist))) : 0;

    cfg.holes_filter = dict.count(metadata_attributes.at(holes_filter)) > 0;
    cfg.holes_filling_mode = dict.count(metadata_attributes.at(holes_fill)) ? std::stoi(dict.at(metadata_attributes.at(holes_fill))) : 0;

    // Parse the name of the files sequence
    cfg.frames_sequence_size = dict.count(metadata_attributes.at(frames_sequence_size)) ? std::stoi(dict.at(metadata_attributes.at(frames_sequence_size))) : 0;
    REQUIRE(cfg.frames_sequence_size);
    // Populate the input_frame_names member with the file names
    for (size_t i = 0; i < cfg.frames_sequence_size; i++)
        cfg.input_frame_names.emplace_back(dict.at(std::to_string(i)) + ".raw");
    return cfg;
}

inline bool load_test_configuration(const std::string test_name, ppf_test_config& test_config)
{
    std::string folder_name = get_folder_path(special_folder::temp_folder);
    std::string base_name = folder_name + test_name;
    base_name += ".0";  // Frame sequence will always be zero-indexed
    static const std::map<std::string, std::string> test_file_names = {
        { "Input_pixels",    ".Input.raw" },
        { "Input_metadata",  ".Input.csv" },
        { "Output_pixels",   ".Output.raw"},
        { "Output_metadata", ".Output.csv"}
    };

    std::vector<bool> fe;
    // Verify that all the required test files are present
    for (auto& filename : test_file_names)
    {
        CAPTURE(base_name);
        CAPTURE(filename.second);
        fe.push_back(file_exists(base_name + filename.second));
        if (!fe.back())
        {
            WARN("A required test file is not present: " << base_name + filename.second << " .Test will be skipped");
        }
    }

    if (std::find(fe.begin(), fe.end(), false) != fe.end())
        return false;

    // Prepare the configuration data set
    test_config.reset();

    test_config.name = test_name;
    ppf_test_config input_meta_params = attrib_from_csv(base_name + test_file_names.at("Input_metadata"));
    ppf_test_config output_meta_params = attrib_from_csv(base_name + test_file_names.at("Output_metadata"));
    test_config.frames_sequence_size = input_meta_params.frames_sequence_size;

    if (test_config.frames_sequence_size > 50)
    {
        WARN("The input sequence is too long - " << test_config.frames_sequence_size << " frames. Performance may be affected");
    }
    test_config.input_frame_names = input_meta_params.input_frame_names;
    test_config.output_frame_names = output_meta_params.input_frame_names;
    // Prefetch all frames data. Assumes that the frames sequences less than a hundred frames
    for (size_t i = 0; i < test_config.frames_sequence_size; i++)
    {
        test_config._input_frames.push_back(std::move(load_from_binary(folder_name + test_config.input_frame_names[i])));
        test_config._output_frames.push_back(std::move(load_from_binary(folder_name + test_config.output_frame_names[i])));
    }

    test_config.input_res_x = input_meta_params.input_res_x;
    test_config.input_res_y = input_meta_params.input_res_y;
    test_config.output_res_x = output_meta_params.input_res_x;
    test_config.output_res_y = output_meta_params.input_res_y;
    test_config.depth_units = input_meta_params.depth_units;
    test_config.focal_length = input_meta_params.focal_length;
    // In existing XML files stereo baseline is in meters, but software expects it to be in mm:
    test_config.stereo_baseline = input_meta_params.stereo_baseline * 1000.f;
    test_config.downsample_scale = output_meta_params.downsample_scale;
    test_config.spatial_filter = output_meta_params.spatial_filter;
    test_config.spatial_alpha = output_meta_params.spatial_alpha;
    test_config.spatial_delta = output_meta_params.spatial_delta;
    test_config.spatial_iterations = output_meta_params.spatial_iterations;
    test_config.holes_filter = output_meta_params.holes_filter;
    test_config.holes_filling_mode = output_meta_params.holes_filling_mode;
    test_config.temporal_filter = output_meta_params.temporal_filter;
    test_config.temporal_alpha = output_meta_params.temporal_alpha;
    test_config.temporal_delta = output_meta_params.temporal_delta;
    test_config.temporal_persistence = output_meta_params.temporal_persistence;

    // Perform sanity tests on the input data
    CAPTURE(test_config.name);
    CAPTURE(test_config.input_res_x);
    CAPTURE(test_config.input_res_y);

    for (auto i = 0; i < test_config.frames_sequence_size; i++)
    {
        CAPTURE(test_config._input_frames[i].size());
        CAPTURE(test_config._output_frames[i].size())
    }

    CAPTURE(test_config.output_res_x);
    CAPTURE(test_config.output_res_y);
    CAPTURE(test_config.spatial_filter);
    CAPTURE(test_config.spatial_alpha);
    CAPTURE(test_config.spatial_delta);
    CAPTURE(test_config.spatial_iterations);
    CAPTURE(test_config.temporal_filter);
    CAPTURE(test_config.temporal_alpha);
    CAPTURE(test_config.temporal_delta);
    CAPTURE(test_config.temporal_persistence);
    CAPTURE(test_config.holes_filter);
    CAPTURE(test_config.holes_filling_mode);
    CAPTURE(test_config.downsample_scale);

    // Input data sanity
    // The resulted frame dimension will be dividible by 4;
    auto _padded_width = uint16_t(test_config.input_res_x / test_config.downsample_scale) + 3;
    _padded_width /= 4;
    _padded_width *= 4;

    auto _padded_height = uint16_t(test_config.input_res_y / test_config.downsample_scale) + 3;
    _padded_height /= 4;
    _padded_height *= 4;
    REQUIRE(test_config.output_res_x == _padded_width);
    REQUIRE(test_config.output_res_y == _padded_height);
    REQUIRE(test_config.input_res_x > 0);
    REQUIRE(test_config.input_res_y > 0);
    REQUIRE(test_config.output_res_x > 0);
    REQUIRE(test_config.output_res_y > 0);
    REQUIRE(std::fabs(test_config.stereo_baseline) > 0.f);
    REQUIRE(test_config.depth_units > 0);
    REQUIRE(test_config.focal_length > 0);
    REQUIRE(test_config.frames_sequence_size > 0);
    for (auto i = 0; i < test_config.frames_sequence_size; i++)
    {
        REQUIRE((test_config.input_res_x * test_config.input_res_y * 2) == test_config._input_frames[i].size()); // Assuming uint16_t type
        REQUIRE((test_config.output_res_x * test_config.output_res_y * 2) == test_config._output_frames[i].size());
    }

    // The following tests use assumption about the filters intrinsic
    // Note that the specific parameter threshold are verified as of April 2018.
    if (test_config.spatial_filter)
    {
        REQUIRE(test_config.spatial_alpha >= 0.25f);
        REQUIRE(test_config.spatial_alpha <= 1.f);
        REQUIRE(test_config.spatial_delta >= 1);
        REQUIRE(test_config.spatial_delta <= 50);
        REQUIRE(test_config.spatial_iterations >= 1);
        REQUIRE(test_config.spatial_iterations <= 5);
    }
    if (test_config.temporal_filter)
    {
        REQUIRE(test_config.temporal_alpha >= 0.f);
        REQUIRE(test_config.temporal_alpha <= 1.f);
        REQUIRE(test_config.temporal_delta >= 1);
        REQUIRE(test_config.temporal_delta <= 100);
        REQUIRE(test_config.temporal_persistence >= 0);
        REQUIRE(test_config.temporal_persistence <= 8);
    }

    if (test_config.holes_filter)
    {
        REQUIRE(test_config.holes_filling_mode >= 0);
        REQUIRE(test_config.holes_filling_mode <= 2);
    }

    return true;
}

template <typename T>
inline bool profile_diffs(const std::string& plot_name, std::vector<T>& distances, const float max_allowed_std, const float outlier, size_t frame_idx)
{
    static_assert((std::is_arithmetic<T>::value), "Profiling is defined for built-in arithmetic types");

    std::ofstream output_file(plot_name);
    for (const auto &val : distances) output_file << val << "\n";
    output_file.close();

    REQUIRE(!distances.empty());

    float mean = std::accumulate(distances.begin(),
        distances.end(), 0.0f) / distances.size();

    auto pixels = distances.size();
    auto first_non_identical_index = -1;
    auto first_difference = 0.f;
    auto non_identical_count = distances.size() - std::count(distances.begin(), distances.end(), static_cast<T>(0));
    auto first_non_identical_iter = std::find_if(distances.begin(), distances.end(), [](const float& val) { return val != 0; });
    if (first_non_identical_iter != distances.end())
    {
        first_non_identical_index = first_non_identical_iter - distances.begin();
        first_difference = *first_non_identical_iter;
    }
    auto max = std::max_element(distances.begin(), distances.end());
    auto max_val_index = max - distances.begin();
    auto max_val = (max != distances.end()) ? *max : 0;
    float e = 0;
    float inverse = 1.f / distances.size();
    for (auto elem : distances)
    {
        e += pow(elem - mean, 2);
    }

    float standard_deviation = static_cast<float>(sqrt(inverse * e));

    if (max_val != 0)
        WARN("Frame" << frame_idx
            << ":  Non-identical pixels = " << non_identical_count
            << " \nFirst non-identical diff = " << first_difference << " at index " << first_non_identical_index
            << "\nmax_diff=" << max_val << ", index=" << max_val_index);

    CAPTURE(pixels);
    CAPTURE(mean);
    CAPTURE(max_val);
    CAPTURE(max_val_index);
    CAPTURE(non_identical_count);
    CAPTURE(first_non_identical_index);
    CAPTURE(first_difference);
    CAPTURE(outlier);
    CAPTURE(standard_deviation);
    CAPTURE(max_allowed_std);
    CAPTURE(frame_idx);

    INTERNAL_CATCH_TEST((standard_deviation <= max_allowed_std), Catch::ResultDisposition::ContinueOnFailure, "CHECK");
    INTERNAL_CATCH_TEST((fabs((max_val)) <= outlier), Catch::ResultDisposition::ContinueOnFailure, "CHECK");
    //REQUIRE(standard_deviation <= max_allowed_std);
    //REQUIRE(fabs((max_val)) <= outlier);

    return ((fabs(max_val) <= outlier) && (standard_deviation <= max_allowed_std));
}
