#pragma once

// Purely functional; this is a handler class, not a tangible in-game object

class SegmentHandler {
	public:
		// Remember to populate in the class .cpp

		// Only the space covered by the player's view cone should be drawn (rendered); however, the
		// player's position on the conceptual plane should be used to derive a torus-similar segment;
		// a space 200 meters wide, ten meters deep and 200 meters long surrounding the player and
		// drawn from within the segment should be loaded into memory and used to retrieve the 2D
		// projections shown to the player via the camera

		// Chunk behaviour and smoothing should be performed through the terrain 
		// generation/manipulation techniques described in the [world] header

		void loadSegment();
		void clearSegment();
		void smoothWorld();
		void shrinkSegment(short divideBy = 2);
};