/* File: bound.c
 * Author: Sanjay Kannan
 * Bounding Params: latTL lonTL latBR lonBR
 * Usage: bound src dest [bounding rectangle params]
 * -------------------------------------------------
 * Takes the name of a folder of images as its first
 * argument and the name of a destination folder as
 * its second argument. Copies all of the images
 * from folder one to folder two according to the
 * next four GPS values relative to NE, which are
 * the latitudes and longitudes of a top right and
 * bottom left corner defining a bounding rectangle.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include "exif.h"

/* Type: EXIFCoord
 * ---------------
 * Stores the EXIF GPS coordinate
 * of a particular image. Set the
 * error boolean if EXIF read failed.
 */

typedef struct {
    double lat;
    double lon;
    bool error;
} EXIFCoord;

static void err(const char* error);
static void copyFile(const char* src, const char* dest);
static double convertDMS(const int* DMSArray, char direction);
static EXIFCoord getEXIFCoord(const char* path);
static bool fileInBounds(char* path, double latTL,
    double lonTL, double latBR, double lonBR);
static void boundDir(const char* srcPath, const char* destPath,
    double latTL, double lonTL, double latBR, double lonBR);

/* Function: err
 * -------------
 * Prints the provided error
 * to console and then quits.
 */
    
static void err(const char* error) {
    printf("bound: fatal error: %s\n", error);
    exit(1);
}

/* Function: copyFile
 * ------------------
 * Attempts to copy the file at src
 * to the path given by dest. This
 * particular implementation is not
 * error-proof and in particular is
 * prone to injection, but I have it
 * here as a simple system call for
 * easy cross-platform support.
 */

static void copyFile(const char* src, const char* dest) {
    char command[strlen(src) + strlen(dest) + 9];
    sprintf(command, "cp \"%s\" \"%s\"", src, dest);
    system(command); // prone to injection attacks
}

/* Function: convertDMS
 * --------------------
 * Takes an array of six values representing
 * the degree, minute, and second values of
 * a GPS coordinate in rational form, as well
 * as a direction character. Returns the coord
 * in decimal degree form relative to NE such
 * that W and S coordinates are negative.
 */

static double convertDMS(const int* DMSArray, char direction) {
    double minutes = (float) 1 / (float) 60;
    double seconds = (float) 1 / (float) 3600;
    
    double degrees = (float) DMSArray[0] / (float) DMSArray[1];
    degrees += (float) DMSArray[2] / (float) DMSArray[3] * minutes;
    degrees += (float) DMSArray[4] / (float) DMSArray[5] * seconds;
    
    degrees *= (direction == 'N' || direction == 'E') ? 1.0 : -1.0;
    return degrees;
}

/* Function: getEXIFCoord
 * ----------------------
 * Reads and processes the EXIF GPS data
 * from a given image filename. Returns a
 * struct representing the decimal latitude
 * and longitude of image GPS data.
 */

static EXIFCoord getEXIFCoord(const char* path) {
    EXIFCoord coord = {0, 0, true}; // initialize struct with defaults
    int result; void** ifdArray = createIfdTableArray(path, &result);
    
    //default coord error value is true
    if (ifdArray == NULL) return coord;
    
    //see exif.h for documentation on how these calls work
    TagNodeInfo* lat = getTagInfo(ifdArray, IFD_GPS, TAG_GPSLatitude);
    TagNodeInfo* lon = getTagInfo(ifdArray, IFD_GPS, TAG_GPSLongitude);
    TagNodeInfo* latDir = getTagInfo(ifdArray, IFD_GPS, TAG_GPSLatitudeRef);
    TagNodeInfo* lonDir = getTagInfo(ifdArray, IFD_GPS, TAG_GPSLongitudeRef);
    
    //return error coord if any read fails
    if (!lat || lat -> error) return coord;
    if (!lon || lon -> error) return coord;
    if (!latDir || latDir -> error) return coord;
    if (!lonDir || lonDir -> error) return coord;
    
    coord.lat = convertDMS(lat -> numData, latDir -> byteData[0]);
    coord.lon = convertDMS(lon -> numData, lonDir -> byteData[0]);
    coord.error = false;
    return coord;
}

/* Function: fileInBounds
 * ----------------------
 * Checks if the GPS position of a given image
 * falls within the provided bounding rectangle
 * defined by its top left and bottom right corners
 * on a map of the world centered at the Greenwich
 * meridian and oriented upright.
 */

static bool fileInBounds(char* path, double latTL,
    double lonTL, double latBR, double lonBR) {
    EXIFCoord imageGPS = getEXIFCoord(path);
    
    //getEXIFCoords sets false flag if the
    //GPS data could not actually be read
    if (imageGPS.error) return false;
    
    //check bounds [inclusive check with bounds]
    if (imageGPS.lat <= latTL && imageGPS.lon >= lonTL
        && imageGPS.lat >= latBR && imageGPS.lon <= lonBR)
        return true;
        
    //not in bounds
    return false;
}

/* Function: boundDir
 * ------------------
 * Applies fileInBounds to each file in a given
 * directory srcPath, and copies files that pass
 * the test to the provided destination path.
 */

static void boundDir(const char* srcPath, const char* destPath,
    double latTL, double lonTL, double latBR, double lonBR) {
    DIR* src = opendir(srcPath);
    
    //struct representing a file
    struct dirent* fileEnt;
    
    //iterate through all such structs in a dir
    while ((fileEnt = readdir(src)) != NULL) {
        if (fileEnt -> d_type != DT_REG)
            continue; // not a regular file
        
        //compute the image filename from struct and source path
        char fileName[strlen(srcPath) + strlen(fileEnt -> d_name) + 1];
        strcpy(fileName, srcPath); strcat(fileName, "/");
        strcat(fileName, fileEnt -> d_name);
        
        if (fileInBounds(fileName, latTL, lonTL, latBR, lonBR)) {
            //compute the image destination name from struct and source path
            char destName[strlen(destPath) + strlen(fileEnt -> d_name) + 1];
            strcpy(destName, destPath); strcat(destName, "/");
            strcat(destName, fileEnt -> d_name);
            
            //uncomment the following line to enable copying
            copyFile(fileName, destName); // note: see implementation
            printf("copied: %s\n", fileEnt -> d_name); // verbose output
        }
    }
    
    //avoid mem leak
    closedir(src);
}

int main(int argc, char* argv[]) {
    if (argc < 2) // fatal error: no source path provided
        err("no source path provided");
    
    struct stat srcStat;
    char* srcPath;
    
    if (stat(argv[1], &srcStat) == 0 && S_ISDIR(srcStat.st_mode)
        && argv[1][strlen(argv[1]) - 1] != '/') // no trailing slash
        srcPath = argv[1]; // source path exists
    
    else // fatal error: invalid source path
        err("provided source path was invalid");
        
    if (argc < 3) // fatal error: no destination path provided
        err("no destination path provided");
        
    struct stat destStat;
    char* destPath;
    
    if (stat(argv[2], &destStat) == 0 && S_ISDIR(destStat.st_mode)
        && argv[2][strlen(argv[2]) - 1] != '/') // no trailing slash
        destPath = argv[2]; // destination path exists
    
    else // fatal error: invalid destination path
        err("provided destination path was invalid");
    
    if (argc < 7) // missing bounding coords
        err("some bounding coords are missing");
    
    char* remain;
    double coords[4];
    
    //process remaining params as doubles
    //and throw fatal errors as necessary
    //odd params are latitude params
    for (int i = 3; i < 7; i += 1) {
        double param = strtod(argv[i], &remain);
        if (strlen(remain) > 0 || errno == ERANGE)
            err("invalid floating point parameter");
        
        //make sure latitude parameters in range
        if (i % 2 == 1 && (param < -90 || param > 90))
            err("latitude parameter out of range");
        
        //make sure longitude parameters in range
        if (i % 2 == 0 && (param < -180 || param > 180))
            err("longitude parameter out of range");
        
        //set coords array
        coords[i-3] = param;
    }
    
    double latTL = coords[0];
    double lonTL = coords[1];
    double latBR = coords[2];
    double lonBR = coords[3];
    
    //make sure parameters form a bounding rectangle
    if (latTL <= latBR || lonTL >= lonBR)
        err("deformed bounding rectangle defined");
    
    //call bounding function with processed params
    boundDir(srcPath, destPath, latTL, lonTL, latBR, lonBR);
    
    //success
    return 0;
}   
