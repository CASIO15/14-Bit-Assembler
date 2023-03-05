#include "driver.h"

/*
	TODO:
	1. Test memory leaks and build on Linux
	2. Replace final success message with something prettier 
*/

/**
* Entry point for the driver. This is the entry point for the driver to be executed.
* 
* @param argc - Number of command line arguments. If - 1 the program will use the standard argument parsing.
* @param argv - Arguments passed to the program.
* 
* @return 0 on success non - zero on failure
*/
int main(int argc, char** argv)
{
	int ret_val;
	Driver* driver = driver_new_driver();
	
	ret_val = driver_exec(driver, argc, argv);
	destroy_driver(&driver);

	return ret_val;
}
