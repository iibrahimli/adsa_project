#include <iostream>
#include <iomanip>
#include <vector>
#include <cstring>
#include "../include/bmp.hpp"
#include "../include/util.hpp"

using namespace std;

int main(int argc, char *argv[]){

    // main just parses arguments and calls needed functions

    unsigned int resolution = 10;  // thickness of horizontal strips

    if(argc < 5){
        print_help(argv[0]);
        return 1;
    }

    if(!strcmp(argv[1], "--psf")){
        if(argc != 1+4) print_help(argv[0]);

        BMP_image a(argv[2]);
        BMP_image b(argv[3]);
        int maxshift = 0;

        auto psf = calc_psf(a, b, &maxshift, resolution);

        // psf file format: n_strips \n thickness \n maxshift \n psf1 \n ... psfN \n
        write_psf(argv[4], psf, resolution, maxshift);
    }
    else if(!strcmp(argv[1], "--scale")){
        if(argc != 1+4) print_help(argv[0]);
        
        BMP_image a(argv[2]);
        int maxshift = 0;

        auto psf = read_psf(argv[3], &resolution, &maxshift);

        cut_strip(psf_resize(a, psf, resolution), maxshift).save_as(argv[4]);
        
    }
    else if(!strcmp(argv[1], "--merge")){
        vector<const BMP_image*> images;
        images.reserve(argc-3);

        for(int arg=3; arg<argc; ++arg){
            images.push_back(new BMP_image(argv[arg]));
        }
        merge(images).save_as(argv[2]);
        
        // free memory
        for(int i=0; i<images.size(); ++i) delete images[i];
    }
    else{
        print_help(argv[0]);
        return 1;
    }

    return 0;
}