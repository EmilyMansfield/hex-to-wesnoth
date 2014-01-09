/*
	hex-to-wesnoth
    Copyright (C) 2014 Daniel Mansfield

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <utility>
#include <cstdlib>

std::string randString(unsigned int len)
{
	static std::string alphanum =
		"abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"1234567890";

	std::string str;

	while(len-- > 0)
	{
		str += alphanum[rand() % alphanum.length()];
	}

	return str;
}

std::string incrementalString()
{
	static unsigned int i = 0;

	return std::to_string(i++);
}

void printHelp()
{
	std::cerr << "Usage: hex-to-wesnoth [OPTION]... [INPUT FILE]\n";
	std::cerr << "Convert a graphic hex based map into a Battle For Wesnoth compatible map\n\n";
	std::cerr << "  -o\tspecify the output file. Default \"map\"\n";
	std::cerr << "  -d\tspecify a data file. Default \"tiles.dat\"\n";
	std::cerr << "  -t\tspecify tile width and height. Default 32 34\n";
	std::cerr << "  -i\tspecify initial tile offset. Default 26 23\n";
	std::cerr << "  -m\ttop-left tile is a major (upper) tile not a minor\n";
	std::cerr << "  -h\tshow this help and exit\n\n";
	std::cerr << "Examples:\n";
	std::cerr << "  hex-to-wesnoth map.png -x 16 17 -o output.map\t";
	std::cerr << "Convert map.png to output.map with tile width 16 and height 17\n";

	return;
}

// Perform an RGB colour comparison with a threshold value
bool color_compare(sf::Color a, sf::Color b, int t)
{
	return (abs(a.r-b.r) < t && abs(a.g-b.g) < t && abs(a.b-b.b) < t);
}

// Return true if the images are equal, only comparing non-transparent
// pixels in a to those in b
bool image_compare(sf::Image& a, sf::Image& b)
{
	// Colour comparison threshold
	int threshold = 3;

	// If the images are not the same size then return false, they
	// cannot be equal to each other
	if(a.getSize() != b.getSize()) return false;

	for(unsigned int y = 0; y < a.getSize().y; ++y)
	{
		for(unsigned int x = 0; x < a.getSize().x; ++x)
		{
			// Ignore non-opaque alpha values
			if(a.getPixel(x, y).a != 0xff) continue;
			// Compare rgb, return false if discrepancy
			if(!color_compare(a.getPixel(x,y), b.getPixel(x,y), threshold))
			{
				return false;
			}
		}
	}

	// No discrepancies so images are equal
	return true;
}

int main(int argc, char *argv[])
{
	// Filenames
	std::string outFileName = "map";
	std::string inFileName = "image.png";
	std::string dataFileName = "tiles.dat";

	// Tile centre pixel offsets
	unsigned int tileOffsetX = 32;
	unsigned int tileOffsetY = 34;

	unsigned int initialOffsetX = 5;
	unsigned int initialOffsetY = 6;

	// Tile dimensions
	unsigned int tileWidth = 43;
	unsigned int tileHeight = 35;

	// First tile is an upper one and not a lower (Wesnoth requires lower)
	bool majorTileStart = false;

	// Read the command line arguments
	if(argc > 1)
	{
		for(int i = 1; i < argc; ++i)
		{
			// Output file
			if((std::string)argv[i] == "-o")
			{
				// Requires an argument
				if(i+1 >= argc || argv[i+1][0] == '-')
				{
					std::cerr << "-o requires an argument\n";
					return 1;
				}
				outFileName = argv[++i];
			}
			// Data file
			else if((std::string)argv[i] == "-d")
			{
				// Requires an argument
				if(i+1 >= argc || argv[i+1][0] == '-')
				{
					std::cerr << "-d requires an argument\n";
					return 1;
				}
				dataFileName = argv[++i];
			}
			// Tile offset x and y
			else if((std::string)argv[i] == "-t")
			{
				if(i+2 >= argc || argv[i+1][0] == '-' || argv[i+2][0] == '-')
				{
					std::cerr << "-t requires two arguments\n";
					return 1;
				}
				tileOffsetX = std::stoi(argv[i+1]);
				tileOffsetY = std::stoi(argv[i+2]);
				i += 2;
			}
			// Initial tile offset x and y
			else if((std::string)argv[i] == "-i")
			{
				if(i+2 >= argc || argv[i+1][0] == '-' || argv[i+2][0] == '-')
				{
					std::cerr << "-i requires two arguments\n";
					return 1;
				}
				initialOffsetX = std::stoi(argv[i+1]);
				initialOffsetY = std::stoi(argv[i+2]);
				i += 2;
			}
			// Top left tile is a major one (Wesnoth requires minor)
			else if((std::string)argv[i] == "-m")
			{
				majorTileStart = true;
			}
			// Help
			else if((std::string)argv[i] == "-h")
			{
				printHelp();
			}
			// An unknown argument
			else if(argv[i][0] == '-')
			{
				std::cerr << argv[i] << " is not a valid argument\n";
				printHelp();
				return 1;
			}
			// Default to the input file name
			else
			{
				inFileName = argv[i];
			}
		}
	}
	else
	{
		printHelp();
		return 1;
	}
	// Open the output file
	std::ofstream outFile(outFileName, std::ofstream::out);

	// Open the input file
	sf::Image inFile;
	inFile.loadFromFile(inFileName);

	// Open the data file
	std::ifstream dataFile(dataFileName, std::ifstream::in);

	// Vector of tile images and their respective tile names
	std::vector<std::pair<sf::Image, std::string>> tileMap;

	// Data file consists of tile filename and tile name for use in
	// the final map file, separated by a space. Filenames should be
	// (though not a necessity) local to the executable directory
	while(dataFile.good())
	{
		std::string filename = "data/placeholder.png";
		std::string tileString = "Gg";

		dataFile >> filename >> tileString;

		// Load the image and add it to the tile map
		sf::Image tileImage;
		tileImage.loadFromFile(filename);
		tileMap.push_back(std::make_pair(tileImage, tileString));
	}

	// Default tile if there is no match in the tile map
	std::string defaultTile = "Gg";

	// Output the map header to the map file
	outFile << "border_size=1\n" << "usage=map\n" << std::endl;

	srand(1);

	// Iterate over the file in step sizes equal to the tile dimensions
	// and compare each pixel that is not transparent (alpha != 255) in that
	// square to each tile in turn. If all pixels are equal, then the tiles
	// match and the terrain code is outputted to the file. Otherwise move
	// on to the next tile
	for(unsigned int y = initialOffsetY; y < inFile.getSize().y; y += tileOffsetY)
	{
		for(unsigned int x = initialOffsetX; x < inFile.getSize().x; x += tileOffsetX)
		{
			// Adjust for hexagonal minor/major tiling
			unsigned int yTmp = y;
			if(((x - initialOffsetX) / tileOffsetX) % 2 == (int)majorTileStart)
			{
				yTmp += tileOffsetY / 2;
			}
			if(yTmp + tileOffsetY > inFile.getSize().y) continue;
			if(x != initialOffsetX)
			{
				outFile << ", ";
			}

			// Create the small subimage to compare to
			sf::Image subimage;
			subimage.create(tileWidth, tileHeight);
			subimage.copy(inFile, 0, 0, sf::IntRect(x, yTmp, tileWidth, tileHeight));
			//~ subimage.saveToFile("tiles/" + incrementalString() + ".png");

			// Iterate over every tile in the tile map and compare it to the
			// subimage
			bool found = false;
			for(auto tile : tileMap)
			{
				// If the subimage matches one of the tiles then output
				// the corresponding name
				if(found = image_compare(tile.first, subimage))
				{
					outFile << tile.second;
					break;
				}
			}
			if(!found) outFile << defaultTile;
		}
		outFile << std::endl;
	}

	// Close the map file
	outFile.close();

	return 0;
}
