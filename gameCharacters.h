#pragma once

// Super-generic game character; 
// things common to
// AICharacters and 
// PlayerCharacters go here

class GameCharacter {
	public:
		void move();
		void attack();

		// other generic behaviours here...
	private:
		char * voxelPatternFile;
		
		short health;
		short strength;
		short meanSpeed;
		
		// other generic properties here...
};