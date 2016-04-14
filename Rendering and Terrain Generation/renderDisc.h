#pragma once

#include "renderShell.h"

class RenderDisc : RenderShell { 
	public:
		// Render disc properties go here
		// Wtf is there a render disc? Well, 
		// for optimisation purposes each
		// planet is divided like this:
		//
		// Shells (each ten standard meters
		// deep and having a diameter equal
		// to the planet's diameter 
		// overall)
		//
		// Discs (a subset of each shell,
		// more toroidal than true planar
		// spheres; each one contains a
		// particular "band" from a
		// given shell) (ten meters deep,
		// still having planetary diameter
		// but with a total x-width of 200
		// meters)
		//
		// Segments (small boxes taken
		// from each disc; these are what
		// the camera references when it
		// calculates a two-dimensional
		// projection of the information
		// within the player's view cone,
		// and unlike the previous 
		// divisions they aren't circular;
		// each one has an "offset" assumed
		// from the index of their disc)
		// (segments are also what actually
		// gets loaded each time the player
		// covers the 200 metre span
		// between them; they're essentially
		// a chunk analogue for spherical
		// space)
		//
		// So apart from recreating the
		// concept of "chunks" for spherical
		// space, what are all these divisions
		// good for?
		//
		// Well, they make a really good
		// coordinate system. Because each shell,
		// disc and segment has an index (and 
		// because all indices are read in a 
		// positive direction) it's possible to
		// describe position within the sphere
		// in terms of which shell, which disc,
		// and which segment; this can then be
		// used to translate to and from each
		// displacement fractal into the world
		// itself, giving the system an easy
		// way to draw forms from different
		// portions of the planet
		//
		// They are also super-frigging-awesome
		// for rendering; instead of rendering
		// the whole planet at once, you can 
		// draw a subset of each fractal into
		// fractalPlane objects (with
		// area equal to the current segment),
		// combine the fractals into another
		// fractalPlane object that actually
		// holds the terrain to be rendered,
		// then copy the hypothetical fractal
		// into the worldspace through an
		// array of voxels
		//
		// The whole process takes much, much
		// less time than it would to render
		// the whole planet - hundreds of
		// of thousands of times less. 
		//
		// Note: use the shell/disc/segment
		// system to translate between clouds
		// of 3D data and clouds of 2D data when
		// reading between the hypothetical
		// primitive sphere and the different
		// fractal planes (note: the fractal
		// planes are not the whole fractal,
		// because capturing an entire fractal
		// at once is impossible; they are 
		// subsets of the whole fractal with
		// dimension equal to the x/y scale of
		// the currently loaded segment)
	private:
		short discIndex;
};