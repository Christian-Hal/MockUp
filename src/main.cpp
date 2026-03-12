
#include "AppController.h"

/*
Entry point for the program.
*/
int main(){

	AppController app;  

	if (!app.init()) {
		return -1;
	}

	app.run();
	app.shutdown();
	return 0;
}