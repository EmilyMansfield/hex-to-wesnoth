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

class Tile
{
	public:

	sf::Color color;
	std::string name;

	Tile(sf::Color color, std::string name)
	{
		this->color = color;
		this->name = name;
	}

	bool equal(sf::Color color)
	{
		unsigned int t = 1;

		return (this->color.r - t <= color.r && this->color.r + t >= color.r) &&
			(this->color.g - t <= color.g && this->color.g + t >= color.g) &&
			(this->color.b - t <= color.b && this->color.b + t >= color.b) &&
			(this->color.a - t <= color.a && this->color.a + t >= color.a);
	}
};

// Linearly blur the image with the specified radius in one dimension,
// then return its transpose. When called twice this efficiently blurs
// the image in two dimensions
sf::Image blur_1d(sf::Image& img, unsigned int r)
{
	sf::Image result;
	result.create(img.getSize().y, img.getSize().x);

	// Iterate over each pixel and blur it horizontally
	for(unsigned int y = 0; y < img.getSize().y; ++y)
	{
		for(unsigned int x = 0; x < img.getSize().x; ++x)
		{
			// Running sum of pixel values (RGB)
			// Don't use sf::Color as we will easily exceed char bounds
			unsigned int pixelSum[3] = { 0, 0, 0 };
			// Number of pixels tested (Used to compute average)
			unsigned int n = 0;

			for(int x2 = x-r; x2 <= x+r; ++x2)
			{
				// Skip if out of bounds
				if(x2 < 0 || x2 >= img.getSize().x)
					continue;

				// Add pixel to running sum
				sf::Color pixelCol = img.getPixel(x2, y);
				pixelSum[0] += pixelCol.r;
				pixelSum[1] += pixelCol.g;
				pixelSum[2] += pixelCol.b;
				++n;
			}

			// Save blurred pixel value to transposed position in the
			// result image
			if(n == 0) n = 1;
			sf::Color pixelCol(pixelSum[0] / n, pixelSum[1] / n, pixelSum[2] / n);
			result.setPixel(y, x, pixelCol);
		}
	}

	return result;
}

// Linearly blur the image with the specified radius. Slow, but more
// useful than gaussian in this case as we can limit the radius exactly.
sf::Image blur(sf::Image& img, unsigned int r)
{
	// Blur horizontally
	sf::Image tmp = blur_1d(img, r);

	// Blur vertically
	return blur_1d(tmp, r);
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
	std::cerr << "  -b\tspecify blur radius. Default prefiltered\n";
	std::cerr << "  -h\tshow this help and exit\n\n";
	std::cerr << "Examples:\n";
	std::cerr << "  hex-to-wesnoth map.png -x 16 17 -o output.map\t";
	std::cerr << "Convert map.png to output.map with tile width 16 and height 17\n";

	return;
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

	unsigned int initialOffsetX = 26;
	unsigned int initialOffsetY = 23;

	// Radius of the blur to apply. 0 => pre-filtered image
	unsigned int blurRadius = 0;

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
			// Blur radius
			else if((std::string)argv[i] == "-b")
			{
				if(i+1 >= argc || argv[i+1][0] == '-')
				{
					std::cerr << "-b requires an argument\n";
					return 1;
				}
				blurRadius = std::stoi(argv[++i]);
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

	// Blur the input file
	sf::Image result = blur(inFile, blurRadius);
	result.saveToFile("OutputImage.png");
	return 0;

	// Open the data file
	std::ifstream dataFile(dataFileName, std::ifstream::in);

	// Map of colours to their respective tile names
	// Not using actual map or unordered_map because then we have to
	// go to the trouble of implementing hash functions, which is
	// completely pointless due to the small size of the data set
	// (which will always be small)
	std::vector<Tile> tileMap;

	// Data file consists of 3-byte hex number followed by a string
	// (space separated). Number equates to centre pixel RGB value
	// of the blurred tile blurred with a radius equal to the minimum
	// distance from the centre to an edge
	while(dataFile.good())
	{
		unsigned int pixel = 0x000000;
		std::string tileString = "Gg";

		dataFile >> std::hex >> pixel >> tileString;

		// Convert RGB value to an SFML colour
		sf::Color pixelCol;
		pixelCol.r = (pixel & 0xFF0000) >> 16;
		pixelCol.g = (pixel & 0x00FF00) >> 8;
		pixelCol.b =  pixel & 0x0000FF;

		// Create the tile and add it to the tile map
		tileMap.push_back(Tile(pixelCol, tileString));
	}
	//~ Tile tileMap[] =
	//~ {
		//~ Tile(sf::Color(0x71, 0x93, 0xbf), "Wog"),	// Ocean
		//~ Tile(sf::Color(0xf2, 0xf2, 0xf2), "Aa"),	// Snowy plains
		//~ Tile(sf::Color(0xbe, 0xbe, 0xbe), "Aa^Fpa"),// Snowy deciduous forest
		//~ Tile(sf::Color(0xea, 0xea, 0xea), "Ha"),	// Snowy hills
		//~ Tile(sf::Color(0xc6, 0xc6, 0xc6), "Ms"),	// Snowy mountains
		//~ Tile(sf::Color(0x79, 0x79, 0x79), "Ms^Xm"),	// Snowy tall mountains
		//~ Tile(sf::Color(0xb9, 0xe0, 0x7e), "Gll"),	// Green plains
		//~ Tile(sf::Color(0xa7, 0xce, 0x6c), "Gll^Gvs"), // Grass
		//~ Tile(sf::Color(0x96, 0xb8, 0x8f), "Gll^Fmw"), // Grassy forest
		//~ Tile(sf::Color(0xb7, 0xd6, 0x52), "Hh"), // Grassy hills
		//~ Tile(sf::Color(0xdb, 0x90, 0x72), "Mm"), // Mountain
		//~ Tile(sf::Color(0x98, 0xbb, 0xaa), "Ss"), // Swamp
		//~ Tile(sf::Color(0x93, 0xb8, 0xaf), "Sm"), // Marsh
		//~ Tile(sf::Color(0xcd, 0xcd, 0xcd), "Rb"), // Dirt
		//~ Tile(sf::Color(0x9f, 0x9f, 0x9f), "Aa^Fetd"), // Dead tree
		//~ Tile(sf::Color(0x88, 0x88, 0x88), "Cha"), // Snowy castle
		//~ Tile(sf::Color(0x83, 0xaa, 0x48), "Ch"), // Castle
		//~ Tile(sf::Color(0x76, 0x9d, 0x3c), "Kv"), // Temple
		//~ Tile(sf::Color(0x6f, 0x6f, 0x6f), "Kv"), // Snowy temple
		//~ Tile(sf::Color(0x84, 0x84, 0x84), "Kea"), // Snowy mine
		//~ Tile(sf::Color(0x6e, 0x6e, 0x63), "Kha"), // Snowy tower
		//~ Tile(sf::Color(0x81, 0xa8, 0x46), "Ke"), // Mine
		//~ Tile(sf::Color(0x76, 0x9d, 0x3b), "Kh"), // Tower
		//~ Tile(sf::Color(0x96, 0xbd, 0x5b), "Qxu"), // Cave
		//~ Tile(sf::Color(0xaa, 0xaa, 0xaa), "Khr"), // Dungeon
		//~ Tile(sf::Color(0x73, 0x73, 0x73), "Aa^Vha"), // Snowy village
		//~ Tile(sf::Color(0xa4, 0xa4, 0xa4), "Aa^Vhca") // Snowy city
	//~ };

	// Default tile if there is no match in the tile map
	std::string defaultTile = "Gg";

	// Output the map header to the map file
	outFile << "border_size=1\n" << "usage=map\n" << std::endl;

	// Assume the image has been filtered correctly already

	// Iterate through the centre points of each tile and use the colour
	// value at the point to determine which tile should be added, before
	// outputting the tile to the map file
	for(unsigned int y = initialOffsetY; y < inFile.getSize().y; y += tileOffsetY)
	{
		for(unsigned int x = initialOffsetX; x < inFile.getSize().x; x += tileOffsetX)
		{
			unsigned int yTmp = y;
			if(((x - initialOffsetX) / tileOffsetX) % 2 == 0)
			{
				yTmp += tileOffsetY / 2;
			}
			if(x != initialOffsetX)
			{
				outFile << ", ";
				//~ std::cout << ", ";
			}
	
			// If the tileMap contains a tile for the colour of the pixel,
			// then add that tile to the map, otherwise add a default tile
			bool found = false;
			sf::Color col = inFile.getPixel(x, yTmp);
			//~ std::cout << std::hex << (int)col.r << " " << (int)col.g << " " << (int)col.b;
			for(auto tile : tileMap)
			{
				if(tile.equal(col))
				{
					outFile << tile.name;
					found = true;
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
