hex-to-wesnoth
==============

Converts simple graphical hexagonal tile maps (such as those from [Wizardawn](http://www.wizardawn.com) into [Battle For Wesnoth](http://www.wesnoth.org) compatible maps.

Compiling
---------
This program uses [SFML](http://http://www.sfml-dev.org) 2.1 to handle the images (not 1.6, which is likely to be in your (if using Linux) distribution repos). You will also need a C++11 compatible compiler. To compile in place on Linux using g++ run

  g++ main.cpp -o hex-to-wesnoth -O2 -lsfml-system -lsfml-graphics -std=c++11

I may provide a makefile later.
