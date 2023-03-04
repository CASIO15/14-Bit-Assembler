#include "driver.h"

/*
	TODO:
	1. Write docs.
	2. Write data_img to the file.
	3. Fix encoding of labels.
	4. Add constant to get rid of magic numbers.
	5. Replace final success message with something prettier 
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
	Driver* driver = driver_new_driver();
	return driver_exec(driver, argc, argv);
}