================================================================================
GTSAM - Georgia Tech Smoothing and Mapping Library

MATLAB wrapper

http://borg.cc.gatech.edu/projects/gtsam
================================================================================

This is the GTSAM MATLAB toolbox, a MATLAB wrapper around the GTSAM C++
library. To build it, enable GTSAM_INSTALL_MATLAB_TOOLBOX in CMake.

The interface to the toolbox is generated automatically by the wrap
tool which directly parses C++ header files. The tool generates matlab
proxy objects together with all the native functions for wrapping and
unwrapping scalar and non scalar types and objects. The interface
generated by the wrap tool also redirects the standard output stream
(cout) to matlab's console.

----------------------------------------
Note about newer Ubuntu versions unsupported by MATLAB (later than 10.04)
----------------------------------------

If you have a newer Ubuntu system, you must make a small modification to your
MATLAB installation, due to MATLAB being distributed with an old version of
the C++ standard library.  Delete or rename all files starting with
'libstdc++' in your MATLAB installation directory, in paths:

	/usr/local/MATLAB/[version]/sys/os/[system]/ 
	/usr/local/MATLAB/[version]/bin/[system]/


----------------------------------------
Adding the toolbox to your MATLAB path
----------------------------------------

To get started, first add the 'toolbox' folder to your MATLAB path - in the
MATLAB file browser, right-click on the folder and click 'Add to path -> This
folder' (do not add the subfolders to your path).


----------------------------------------
Trying out the examples
----------------------------------------

The examples are located in the 'gtsam_examples' subfolder.  You may either
run them individually at the MATLAB command line, or open the GTSAM example
GUI by running 'gtsamExamples'.  Example:

>> cd /Users/yourname/toolbox  % Change to wherever you installed the toolbox
>> cd gtsam_examples           % Change to the examples directory
>> gtsamExamples               % Run the GTSAM examples GUI


----------------------------------------
Running the unit tests
----------------------------------------

The GTSAM MATLAB toolbox also has a small set of unit tests located in the
gtsam_tests directory.  Example:

>> cd /Users/yourname/toolbox  % Change to wherever you installed the toolbox
>> cd gtsam_tests              % Change to the examples directory
>> test_gtsam                  % Run the unit tests
Starting: testJacobianFactor
Starting: testKalmanFilter
Starting: testLocalizationExample
Starting: testOdometryExample
Starting: testPlanarSLAMExample
Starting: testPose2SLAMExample
Starting: testPose3SLAMExample
Starting: testSFMExample
Starting: testStereoVOExample
Starting: testVisualISAMExample
Tests complete!


----------------------------------------
Writing your own code
----------------------------------------

Coding for the GTSAM MATLAB toolbox is straightforward and very fast once you
understand a few basic concepts!  Please see the manual to get started.
