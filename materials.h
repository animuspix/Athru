#pragma once

// The generic materials class; 
// can be adapted to create 
// wood, sand, etc

// Consider using a struct
// instead; it's unlikely
// that a materials
// descriptor will be
// process-heavy by itself

// Even though it's /possible/
// to define materials per-box
// and instantiate those boxes
// entirely through the
// x/y/z functions, it's very
// difficult to track
// collectives this way - you
// basically have to select
// all the boxes with the 
// "x" materialType, run your
// transformation, and then
// double-check the 
// materialType every time
// a selection occurs. 

// A more efficient solution
// is to have a bunch of 
// standard classes defined
// at the start of the
// program (probably in a 
// .cpp), named "water", "sand", 
// etc., and to refer to those
// when you do the
// transformation - instead of 
// defining a vector, running
// a transformation, and
// checking the box material at
// every stage of the 
// action, you can specify your 
// transformation to only affect 
// types-of water right at the
// start. Because you're telling
// it to only deal with certain
// boxes before it even begins,
// you don't have to waste cores
// carefully checking every
// single box (remember, there'll
// be thousands of them in a
// meter towards the beginning
// of the space age) 

// Declare vector, point, position
// types!

class Sound;

class Face;

class Material {
	public:

		virtual void transmute(Material * transmuteInto);

		Material();
		
		virtual ~Material() = 0;

	private:

		// String holding the names of different
		// materials

		char * materialType;

		// State-of-matter; gas, liquid, solid
		// Affects the behaviour of the
		// material

		char * state;

		// Sound cue; affects the ambient
		// noise when the player walks over
		// different boxes

		Sound * soundCue;

		// Stores the faces (and thus the 
		// faces) of boxes inheriting from
		// a given material

		Face * boxFaces[6];

		// Light modifiers, e.g refraction,
		// transparency, reflection, 
		// diffraction

		// The above may actually be different
		// types of refraction; we should 
		// double-check that

		float refractiveIndex;
		float percentTransparency;
		float percentReflection;

		// Rigidbody properties 

		short density;
};