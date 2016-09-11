#pragma once

#include "Typedefs.h"

class Service
{
	public:
		Service(char* name);
		~Service();

		char* GetName();
		void SetName(char* newName);

	private:
		char* serviceName;
};

