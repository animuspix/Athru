========================================================================
    STATIC LIBRARY : Athru Project Overview
========================================================================

AppWizard has created this Athru library project for you.

This file contains a summary of what you will find in each of the files that
make up your Athru application.


Athru.vcxproj
    This is the main project file for VC++ projects generated using an Application Wizard.
    It contains information about the version of Visual C++ that generated the file, and
    information about the platforms, configurations, and project features selected with the
    Application Wizard.

Athru.vcxproj.filters
    This is the filters file for VC++ projects generated using an Application Wizard. 
    It contains information about the association between the files in your project 
    and the filters. This association is used in the IDE to show grouping of files with
    similar extensions under a specific node (for e.g. ".cpp" files are associated with the
    "Source Files" filter).


/////////////////////////////////////////////////////////////////////////////

StdAfx.h, StdAfx.cpp
    These files are used to build a precompiled header (PCH) file
    named Athru.pch and a precompiled types file named StdAfx.obj.

/////////////////////////////////////////////////////////////////////////////
Other notes:

AppWizard uses "TODO:" comments to indicate parts of the source code you
should add to or customize.

/////////////////////////////////////////////////////////////////////////////

Just something to notate before I forget: there should be 10 major steps between the start of the game and the final outcome: 10 halves gives a final voxel size of just under one millimetre (1/(2^10) metres) (assuming a starting size of one meter), which is more than enough for reasonably-realistic presentation without breaking the user's system. It also feels like the maximum number of significant moments in human history that you can tie the game into before the whole thing begins to feel contrived.

Development is confirmed for DirectX/Win7+ Desktop; there's some excellent DirectX 11 tutorials over here (there's a DirectX 9 tute over there as well)

http://www.rastertek.com/tutindex.html

I should really clean this up and swap it out for a [readme.md] instead - there's a weird mixture of automade library creation comments and development notes here, and they should probably be split up and differentiated.
