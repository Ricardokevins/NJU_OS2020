#include <stdio.h>
#include "utils.h"
#include "data.h"
#include "func.h"

int main(int argc, char *argv[]) {
    char driver[NAME_LENGTH];
    char srcFilePath[NAME_LENGTH];
    char destFilePath[NAME_LENGTH];

    stringCpy("fs.bin", driver, NAME_LENGTH - 1);
    format(driver, SECTOR_NUM, SECTORS_PER_BLOCK);

    stringCpy("/boot", destFilePath, NAME_LENGTH - 1);
    mkdir(driver, destFilePath);

    stringCpy(argv[1], srcFilePath, NAME_LENGTH - 1);
    stringCpy("/boot/initrd", destFilePath, NAME_LENGTH - 1);
    cp(driver, srcFilePath, destFilePath);

    stringCpy("/dev", destFilePath, NAME_LENGTH - 1);
    mkdir(driver, destFilePath);

    stringCpy("/dev/stdin", destFilePath, NAME_LENGTH - 1);
    touch(driver, destFilePath);

    stringCpy("/dev/stdout", destFilePath, NAME_LENGTH - 1);
    touch(driver, destFilePath);

    stringCpy("/usr", destFilePath, NAME_LENGTH - 1);
    mkdir(driver, destFilePath);

    stringCpy(argv[2], srcFilePath, NAME_LENGTH - 1);
    stringCpy("/usr/print", destFilePath, NAME_LENGTH - 1);
    cp(driver, srcFilePath, destFilePath);

    stringCpy(argv[3], srcFilePath, NAME_LENGTH - 1);
    stringCpy("/usr/bounded_buffer", destFilePath, NAME_LENGTH - 1);
    cp(driver, srcFilePath, destFilePath);

    stringCpy(argv[4], srcFilePath, NAME_LENGTH - 1);
    stringCpy("/usr/philosopher", destFilePath, NAME_LENGTH - 1);
    cp(driver, srcFilePath, destFilePath);

    stringCpy(argv[5], srcFilePath, NAME_LENGTH - 1);
    stringCpy("/usr/reader_writer", destFilePath, NAME_LENGTH - 1);
    cp(driver, srcFilePath, destFilePath);

    stringCpy(argv[6], srcFilePath, NAME_LENGTH - 1);
    stringCpy("/usr/Shell", destFilePath, NAME_LENGTH - 1);
    cp(driver, srcFilePath, destFilePath);

    stringCpy(argv[7], srcFilePath, NAME_LENGTH - 1);
    stringCpy("/usr/testcase", destFilePath, NAME_LENGTH - 1);
    cp(driver, srcFilePath, destFilePath);

    stringCpy("/", destFilePath, NAME_LENGTH - 1);
    ls(driver, destFilePath);

    stringCpy("/boot", destFilePath, NAME_LENGTH - 1);
    ls(driver, destFilePath);

    stringCpy("/usr", destFilePath, NAME_LENGTH - 1);
    ls(driver, destFilePath);

    stringCpy("/usr/temp", destFilePath, NAME_LENGTH - 1);
    touch(driver, destFilePath);

    stringCpy("/usr", destFilePath, NAME_LENGTH - 1);
    ls(driver, destFilePath);
    
    //stringCpy("/", destFilePath, NAME_LENGTH - 1);
    //ls(driver, destFilePath);

    //stringCpy("/boot", destFilePath, NAME_LENGTH - 1);
    //ls(driver, destFilePath);

   // stringCpy("/usr", destFilePath, NAME_LENGTH - 1);
  //  ls(driver, destFilePath);

    stringCpy("/usr/temp", destFilePath, NAME_LENGTH - 1);
    rm(driver, destFilePath);
    
    stringCpy("/usr", destFilePath, NAME_LENGTH - 1);
    ls(driver, destFilePath);

    //stringCpy("/usr/bounded_buffer", destFilePath, NAME_LENGTH - 1);
    //rm(driver, destFilePath);

    //stringCpy("/usr", destFilePath, NAME_LENGTH - 1);
    //ls(driver, destFilePath);

    stringCpy("/test", destFilePath, NAME_LENGTH - 1);
    mkdir(driver, destFilePath);

    stringCpy("/test", destFilePath, NAME_LENGTH - 1);
    ls(driver, destFilePath);

    stringCpy("/", destFilePath, NAME_LENGTH - 1);
    ls(driver, destFilePath);

    stringCpy("/test", destFilePath, NAME_LENGTH - 1);
    rmdir(driver, destFilePath);

    //stringCpy("/usr", destFilePath, NAME_LENGTH - 1);
   // rmdir(driver, destFilePath);

    stringCpy("/", destFilePath, NAME_LENGTH - 1);
    ls(driver, destFilePath);
    

    return 0;
}
