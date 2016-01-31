# TexGen
A simple tool for procedural texture generation, using various noise techniques.

Copyright (C) 2016 Erik Boström.

This application has the following dependencies:

  - Eigen3
    http://eigen.tuxfamily.org/index.php?title=Main_Page
  - SDL2
    http://www.libsdl.org/
  - Horde3d
    http://www.horde3d.org/
  - Squirrel3
    http://www.squirrel-lang.org/

These libraries are included as code in the project repository.

## Building
Just run make. Currently works only on Linux. Requires gcc 4.8, make and cmake 2.6.

Please excuse the build-system, it's a quickly thrown together mess of makefiles.

## Instructions
Build and run the application (it's created in bin/release). An black screen with a cube should appear. Esc exits. Hit ctrl+tab to bring up the squirrel REPL.

The application is controlled through a simple imperative squirrel API, for a description of it see the manual. At start up the application runs the script in the file 'init.nut'. The application already has some presets that can be viewed by running the command 'loadPreset(preset, seed)' where 'preset' is the number of the preset (currently only 1, 2), and 'seed' is a seed to the random generator (an integer or a string). The model in the viewer can be rotated by clicking the left mouse button and dragging, the light source can be rotated by clicking and dragging the right mouse button. Plus and minus on the number pad zooms in and out.

Have a look through the examples and the manual to get a hang of what the application can do. More docs to be written in the future.
