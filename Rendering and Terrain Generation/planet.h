#pragma once

class Planet {
	public:
	
		// Collection of terrain-generation
		// functions; each one takes a
		// coordinate
	
		//short xFunction(float xCoord);
		//short yFunction();
		//short zFunction();
	
		//short getBox(float xCoord, float yCoord, float ZCoord);
	
		// The world map has infinite size, so traditional absolute
		// rendering won't work; the alternative is to take the 
		// spherical volume around the player character (out to 
		// the depth of their view cone), then rendering a slice 
		// of that according to the angular width and angular 
		// height of the view (stored inside [camera])
	
		// Terrain generation itself should occur independantly
		// of player position - although the terrain is only
		// rendered when the player discovers it, the function
		// controlling that render should be set at game start
	
		// Do more research into terrain generation mechanics,
		// publish results here
	
		// Terrain generation is strictly algorithmic, and can
		// be done with just a seed and the player coordinates;
		// the details of this are on paper, but they'll be put
		// up here ASAP
	
	private:
		short terrainZLimit;
		short averageWaterLevel;
		short averageMass;
		short averageRadius;
};