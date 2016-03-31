#pragma once

class Vector {
	public:
		short getMagnitude();
		
		// Perform an operator
		// overload for 
		// calculating the
		// dot/cross product

		short vectorForm();

		// Add supporting
		// functions + vars
		// for handling
		// differential/integral
		// calculus

	private:
		short displacementX;
		short displacementY;
		short displacementZ;
};