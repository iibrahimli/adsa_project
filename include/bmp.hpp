#ifndef __BMP_HPP
#define __BMP_HPP


#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <thread>
#include <cmath>
#include "pixel.hpp"


// if a correlation result has a score lower than this, a warning will be shown
#define SCORE_THRESHOLD 0.8


// TODO fix 24/32 bits per pixel support

using pixel = pixel4;  // bmps we work with use 32 bits per pixel


#pragma pack(push, 1)  // 1 byte alignment for file i/o
struct BMP_file_header {
    uint16_t   file_type    { 0x4D42 };      // magic number, always "BM" which is 0x4D42
    uint32_t   file_size    { 0 };           // size of the file (in bytes)
    uint16_t   reserved1    { 0 };           // reserved, always 0
    uint16_t   reserved2    { 0 };           // reserved, always 0
    uint32_t   pxl_offset   { 0 };           // start position of pixel data (bytes from the beginning of the file)
};


// BITMAPINFOHEADER - 40 bytes
struct BMP_info_header {
    uint32_t   size         { 0 };           // size of this header (in bytes)
    int32_t    width        { 0 };           // width of bitmap in pixels
    int32_t    height       { 0 };           // width of bitmap in pixels
    uint16_t   planes       { 1 };           // # of planes for the target device, this is always 1
    uint16_t   bit_count    { 0 };           // # of bits per pixel
    uint32_t   compression  { 0 };           // 0 or 3 - uncompressed. THIS PROGRAM CONSIDERS ONLY UNCOMPRESSED BMPS
    uint32_t   size_image   { 0 };           // 0 - for uncompressed images
    int32_t    x_pxl_per_m  { 0 };           // x pixels per meter
    int32_t    y_pxl_per_m  { 0 };           // y pixels per meter
    uint32_t   colors_used  { 0 };           // # of color indexes in the color table. Use 0 for the max number of colors
    uint32_t   colors_imp   { 0 };           // # of of colors used for displaying the bitmap. If 0 all colors are required
};
#pragma pack(pop)



struct BMP_image {

    BMP_file_header   file_h;
    BMP_info_header   info_h;
    pixel *           data{nullptr};        // stores pixels in column-major order WITHOUT padding


    // no default constructor
    BMP_image() = delete;


    // copy constructor
    BMP_image(const BMP_image& i)
    : file_h(i.file_h), info_h(i.info_h), filenm(i.filenm), padding(i.padding), neg_height(i.neg_height)
    {
        data = new pixel[i.info_h.width * i.info_h.height];
        for(int d=0; d<i.info_h.width * i.info_h.height; ++d) data[d] = i.data[d];
    }


    // move constructor
    BMP_image(BMP_image&& i)
    : file_h(i.file_h), info_h(i.info_h), filenm(i.filenm), padding(i.padding), neg_height(i.neg_height)
    {
        pixel* dcp = data;
        data = i.data;
        i.data = dcp;
    }


    // reads a BMP image from file
    BMP_image(const char *filename){
        read(filename);
    }


    // creates a blank image of given size
    BMP_image(unsigned int width, unsigned int height, pixel fill_color = WHITE, bool has_alpha = true){

        if (width <= 0 || height <= 0) {
            throw std::runtime_error("The image width and height must be positive numbers");
        }

        info_h.width = width;
        info_h.height = height;

        neg_height = true;

        // don't hold padding bytes
        data = new pixel[width * height];

        // fill pixels array
        for(int i=0; i<width*height; ++i) data[i] = fill_color;

        info_h.size = sizeof(BMP_info_header);
        file_h.pxl_offset = sizeof(BMP_file_header) + sizeof(BMP_info_header);

        if (has_alpha) {
            info_h.bit_count = 32;
            file_h.file_size = file_h.pxl_offset + width * height * info_h.bit_count/8;
        }
        else {
            info_h.bit_count = 24;
            padding = 4 - ((info_h.width * info_h.bit_count/8) % 4);
            file_h.file_size = file_h.pxl_offset + (width * info_h.bit_count/8 + padding) * height;
        }

    }


    // free the memory allocated on heap
    ~BMP_image(){
        delete[] data;
    }


    // pixel access is similar to Fortran array indexing, but is 0-indexed
    // for example: a(2, 5) is 3rd pixel of 6th row

    // read-only access to pixel at location (i, j)
    inline const pixel& ploc(unsigned int i, unsigned int j) const {
        if(!(0 <= i && i < info_h.width && 0 <= j && j < info_h.height)){
            throw std::runtime_error("Coordinates out of range: (" + std::to_string(i) + ", " + std::to_string(j) + ")");
        }
        return data[j*info_h.width + i];
    }


    // read-write access to pixel at location (i, j)
    inline pixel& operator()(unsigned int i, unsigned int j){
        if(!(0 <= i && i < info_h.width && 0 <= j && j < info_h.height)){
            throw std::runtime_error("Coordinates out of range: (" + std::to_string(i) + ", " + std::to_string(j) + ")");
        }
        return data[j*info_h.width + i];
    }

    void read(const char *filename){

        filenm = filename;

        std::ifstream in(filename, std::ios::binary);

        if(!in){
            // error opening file
            std::cerr << "Unable to read file \'" << filename << "\'" << std::endl;
            exit(EXIT_FAILURE);
        }

        // read file header
        in.read((char*) &file_h, sizeof file_h);

        // if first 2 bytes don't match BM
        if(file_h.file_type != 0x4D42){
            std::cerr << "File \'" << filename << "\' is not a BMP file" << std::endl;
            exit(EXIT_FAILURE);
        }

        // read DIB header
        in.read((char*) &info_h, sizeof info_h);

        // image is stored top to bottom
        if(info_h.height < 0){
            info_h.height = abs(info_h.height);
            neg_height = true;
        }

        // seek to pixels
        in.seekg(file_h.pxl_offset, in.beg);

        // adjust the header fields for output, only save the headers and the data.
        if(info_h.bit_count == 32) {
            info_h.size = sizeof(BMP_info_header);
            file_h.pxl_offset = sizeof(BMP_file_header) + sizeof(BMP_info_header);
        } else {
            info_h.size = sizeof(BMP_info_header);
            file_h.pxl_offset = sizeof(BMP_file_header) + sizeof(BMP_info_header);
        }

        // initialize data array
        data = new pixel[info_h.width * info_h.height];

        // if bytes per pixel is not divisible by 4
        if(info_h.bit_count/8 % 4 != 0){

            // check whether we need padding
            if(info_h.width % 4 == 0){

                // width is divisible by 4 = no padding needed
                in.read((char*) data, info_h.width * info_h.height * info_h.bit_count/8);

            }
            else{
                padding = 4 - ((info_h.width * info_h.bit_count/8) % 4);

                for(int r=0; r<info_h.height; ++r){
                    // read a row
                    in.read((char*) &data[r * info_h.width], info_h.width * info_h.bit_count/8);

                    // discard padding
                    in.ignore(padding);
                }
            }

        }
        else{
            in.read((char*) data, info_h.width * info_h.height * info_h.bit_count/8);
        }

        // file size is size of headers + pixel data with padding bytes
        file_h.file_size = sizeof file_h + sizeof info_h + info_h.height * (info_h.width * info_h.bit_count/8 + padding);

    }


    void save_as(const char *filename){
        filenm = filename;

        std::ofstream out(filename, std::ios::binary);

        if(!out){
            // error opening file
            std::cerr << "Unable to write to file \'" << filename << "\'" << std::endl;
            exit(EXIT_FAILURE);
        }

        // can't work with other bits per pixel values
        if(info_h.bit_count != 24 && info_h.bit_count != 32){
            std::cerr << "Can't work with bpp values other than 32 or 24 (bpp is " << info_h.bit_count << ")" << std::endl;
            exit(EXIT_FAILURE);
        }

        write_h_p(out);
        out.close();
    }


    void print_summary(){
        std::cout << std::left << std::setw(16) << "file name: " << filenm << std::endl;
        std::cout << std::left << std::setw(16) << "file size: " << std::setprecision(5) << (float) file_h.file_size/(0x1<<20) << " MiB" << std::endl;
        std::cout << std::left << std::setw(16) << "info_h size: " << info_h.size << std::endl;
        std::cout << std::left << std::setw(16) << "pixel offset: " << file_h.pxl_offset << std::endl;
        std::cout << std::left << std::setw(16) << "width: " << info_h.width << std::endl;
        std::cout << std::left << std::setw(16) << "height: " << info_h.height << std::endl;
        std::cout << std::left << std::setw(16) << "bits/pixel: " << info_h.bit_count << std::endl;
        std::cout << std::left << std::setw(16) << "padding bytes: " << padding << std::endl;
        std::cout << std::left << std::setw(16) << "compression: " << info_h.compression << std::endl;
    }


    friend std::pair<int, float> x_correlate(const BMP_image& a, std::pair<int, int> a_start, std::pair<int, int> a_end,
                                             const BMP_image& b, std::pair<int, int> b_start, std::pair<int, int> b_end);

private:

    std::string    filenm      {"<unknown>"};     // file name
    unsigned int   padding     = 0;               // # of padding bytes at the end of the row
    bool           neg_height  = false;           // true if h is negative (top to bottom)


    // write headers and pixel data to of
    void write_h_p(std::ofstream& of){
        if(neg_height) info_h.height *= -1;
        of.write((const char*) &file_h, sizeof file_h);
        of.write((const char*) &info_h, sizeof info_h);
        if(neg_height) info_h.height *= -1;

        for(int r=0; r<info_h.height; ++r){
            of.write((const char *) &data[r * info_h.width], info_h.width * info_h.bit_count/8);
            of.write("\x00\x00\x00", padding);  // pad with null bytes
        }
    }

};



#endif // __BMP_HPP