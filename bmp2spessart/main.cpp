#define _CRT_SECURE_NO_WARNINGS

#include <algorithm>
#include <iostream>
#include <string>
#include <format>
#include <map>
#include <vector>

void convert(const std::string& path) {
	auto* const file{ fopen(path.c_str(), "rb") };
	if (!file) {
		const auto message{ "File not found: " + path };
		throw std::exception{message.c_str()};
	}

	unsigned char header[54];
	if (fread(header, 1, 54, file) != 54) {
		const auto message{ "File is invalid or corrupted: " + path };
		throw std::exception{message.c_str()};
	}
	if (header[0] != 'B' || header[1] != 'M') {
		const auto message{ "File is invalid or corrupted: " + path };
		throw std::exception{message.c_str()};
	}

	const auto bits{ *reinterpret_cast<int*>(&header[0x1C]) };
	if (bits != 24) {
		const auto message{ "File is not a 24bit BMP: " + path
			+ " is " + std::to_string(bits) + "bit" };
		throw std::exception{message.c_str()};
	}

	const auto width{ *reinterpret_cast<int*>(&header[0x12]) };
	const auto height{ *reinterpret_cast<int*>(&header[0x16]) };
	const auto size{ *reinterpret_cast<int*>(&header[0x22]) };

	auto* const rawdata{ new unsigned char[size] };
	fread(rawdata, 1, size, file);
	fclose(file);

	std::vector<std::string> pixel_data;

	for (int y = height; y-->0;) {
		for (int x = 0; x < width; x++) {
			int offset = y * width + x;
			offset *= 3; //3 byte per pixel in 24bit, index into correct pixel and subindex by byte below

			std::string dat;
			dat += std::format("{:0>2x}", rawdata[offset + 2]); //BMP stores in order BGR
			dat += std::format("{:0>2x}", rawdata[offset + 1]);
			dat += std::format("{:0>2x}", rawdata[offset + 0]);

			pixel_data.push_back(dat);
		}
	}

	//todo: option to blend similar colours so you don't have to change clown pencil 50 times
	std::vector <std::string> unique_colours;
	for (const auto& pixel : pixel_data) {
		if (std::ranges::none_of(unique_colours, [&pixel](const std::string& col) {
			return pixel == col;
			})) {
			unique_colours.push_back(pixel);
		}
	}

	std::map<std::string, unsigned> index_map;
	unsigned index = 0;
	for (const auto& col : unique_colours) {
		index_map.emplace(col, index++);
	}

	std::string output;
	//header info
	output += R"({"w":)" + std::to_string(width) + R"(,"h":)" + std::to_string(height) + R"(,"rgn":)";
	//colour list
	output += R"([)";
	auto cols = unique_colours.size();
	for (int i = 0; i < cols; ++i) {
		output += R"({"clr":"#)";
		output += unique_colours[i];
		output += R"(","txt":""})";
		if (i < cols - 1) {
			output += ',';
		}
	}
	//pixel index data block
	output += R"(],"bmp":[)";
	const auto pixels = width * height;
	for (int i = 0; i < pixels; ++i) {
		output += std::to_string(index_map.find(pixel_data[i])->second);
		if (i < pixels - 1) {
			output += ',';
		}
	}
	output += "]}";

	std::cout << output;

	delete[] rawdata;
}

int main(int argc, char** argv) {

	std::string path;

	for (int i = 0; i < argc; i++) {
		std::cout << argv[i] << std::endl;
	}

	//let user name the target in-program if they started it from explorer
	if (argc == 1) {
		std::cout << "File (24bit BMP): ";
		std::cin >> path;
	}
	//launch from command line as: bmp2spessart <filepath>
	else if (argc == 2) {
		path = argv[1];
	}

	if (path.empty()) {
		std::cerr << "Unsupported launch: try bmp2spessart <file>" << std::endl;
	} else 
	try {
		convert(path);
	}
	catch (const std::exception& e) {
		std::cerr << e.what();
	}

	std::cout << std::endl << std::endl;

	std::cout << "Done, press any key to exit";
	system("pause");
	return 0;

}