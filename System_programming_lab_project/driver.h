#ifndef DRIVER_H
#define DRIVER_H

/** @file
*/

/** @brief
 * Forward decleration for the programs driver. 
*/
typedef struct driver Driver;

/**
* Create a new driver. This is called by libc's init_driver () to create a new driver.
*
*
* @return Pointer to the new driver or NULL if there was an error allocating memory. The caller must free the returned pointer
*/
Driver* driver_new_driver();

/**
* Execute a driver. This is the entry point for drivers that wish to execute their command line interface.
*
* @param driver
* @param argc - Number of arguments to pass to the driver.
* @param argv - Arguments to pass to the driver. These are passed as - is to the driver's exec () function.
*
* @return 0 on success non - zero on failure. In this case the driver is uninitialized and must be set to NULL
*/
int driver_exec(Driver* driver, int argc, char** argv);


/**
* Executes the assembler. This is the entry point for the execution of the assembly. It will start the pre - assembler and do the first pass of initiating the second pass.
*
* @param driver - The driver to initialize.
* @param argc - Number of arguments to the assembly
* @param argv - Array of argv [ 0 ] to the assembly
*
* @return 0 on success non - zero on failure ( in which case we're exiting the assembly without error
*/
static int exec_impl(Driver* driver, int argc, char** argv);

/**
* Called when the driver is initialized. This is where we initialize the data structures that are used to store debug information.
* 
* @param driver - The driver to initialize.
*/
static void on_initialization(Driver* driver);

/**
* Called when the module is unloaded. Destroys all memory allocated by the module. This is the last function called to clean up the module's data structures.
*
* @param driver - the driver to release
*/
static void on_exit(Driver* driver);

#endif