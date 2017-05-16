
int loadPng(const char *strFilename, 
						unsigned char*& data, 
						int& width, int& height, 
						const int bands, const bool swaprgb);

int getPngInfo(const char *strFilename, 
							 int *ipWidth, int *ipHeight, 
							 int *ipBands=NULL);
