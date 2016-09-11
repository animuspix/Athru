#include "Service.h"

Service::Service(char* name)
{
	serviceName = name;
}

Service::~Service()
{
}

char* Service::GetName()
{
	return serviceName;
}

void Service::SetName(char* newName)
{
	serviceName = newName;
}
