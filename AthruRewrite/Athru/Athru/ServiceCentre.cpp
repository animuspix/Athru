#include "Application.h"
#include "Input.h"
#include "Graphics.h"
#include "ServiceCentre.h"

//Map<char* Service*>* ServiceCentre::Map = nullptr;
ServiceCentre* ServiceCentre::self = nullptr;

ServiceCentre::ServiceCentre()
{
	serviceBox = new Map<char*, Service*>(10);

	// Create and register memory manager + logger
	// here, along with any supporting services
	// accessed by the input service

	// Attempt to create and register the primary input service,
	// then abandon ship if creation fails
	ServiceCentre::Register(new Input());
	if (ServiceCentre::Fetch("Input") == false)
	{
		return;
	}

	// Create and register graphics support services here

	// Attempt to create and register the primary graphics
	// service, then abandon ship if creation fails
	ServiceCentre::Register(new Graphics());
	if (ServiceCentre::Fetch("Graphics") == false)
	{
		return;
	}

	// Create and register additional application-support
	// services here

	// Attempt to create and register the application,
	// then abandon ship if creation fails
	ServiceCentre::Register(new Application());
	if (((Application*)ServiceCentre::Fetch("Application")) == false)
	{
		return;
	}

	// Create and register additional, independent services over here
}

ServiceCentre::~ServiceCentre()
{
	delete serviceBox;
	serviceBox = nullptr;
}

void ServiceCentre::Register(Service* service)
{
	serviceBox->AssignLast(service->GetName(), service);
}

Service* ServiceCentre::Fetch(char* serviceName)
{
	return serviceBox->GetValue(serviceName);
}
