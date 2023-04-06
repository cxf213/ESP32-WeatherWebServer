#include "SPIFFS.h"

void initSPIFFS();
String readFile(fs::FS &fs, const char * path);
void writeFile(fs::FS &fs, const char * path, const char * message);