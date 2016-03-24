#pragma once

// Purely functional; this is a handler class, not a tangible in-game object

class World {
	public:
		// Add related visual functions (load chunks, clear chunks, smoothWorld, shrink render sphere)
		// Remember to populate in the class .cpp

		void loadChunk();
		void clearChunk();
		void smoothWorld();
		void shrinkRenderSphere();

		void clearScreen();
		void fillScreen();
};