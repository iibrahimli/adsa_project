// utility functions to put it all together

#ifndef __UTIL_HPP
#define __UTIL_HPP


#include <vector>
#include "bmp.hpp"


// HALP
void print_help(const char* progname){
    std::cout << "Usage:" << std::endl;
    std::cout << progname << " <option> [ <extra1> <extra2> ... ]" << std::endl;
    std::cout << R"(
Options must be provided in the given order.
Options are mutually exclusive.
Possible options:

    --psf <img1> <img2> <psf_file>          :  calculate psfs for two images and store in <out>
                                               * img1 and img2 - images to calculate shift between
                                               * psf_file - output file with psf profile
    
    --scale <img> <psf_file> <out_img>      :  scale image according to its
                                               corresponding psf profile in psf_file and cuts
                                               a strip of given width from their center
    
    --merge <out_img> <img_1> ... <img_N>   :  merge images from left to right in given order
                                               and saves the result with name <out_img>
    
    --help                                  :  display this help text
    )" << std::endl;
}



// correlate two regions considering translation in x axis only
// stores number of pixels shifted (positive direction: left) and activation in res
void x_correlate_region(const BMP_image& a, std::pair<int, int> a_start, std::pair<int, int> a_end,
                        const BMP_image& b, std::pair<int, int> b_start, std::pair<int, int> b_end,
                        int sh_start, int sh_end, int sh_begin, sim_t* act)
{
    // heights are different
    if(abs(a_start.second - a_end.second)+1 != abs(b_start.second - b_end.second)+1){
        throw std::runtime_error("The heights of correlated regions must match (got "
                                + std::to_string(abs(a_start.second-a_end.second)) + " and "
                                + std::to_string(abs(b_start.second-b_end.second)) + ")");
    }

    int height  = a_end.second - a_start.second + 1;
    int width_a = a_end.first - a_start.first + 1;
    int width_b = b_end.first - b_start.first + 1;

    // width of b exceeds width of a
    if(width_a < width_b){
        throw std::runtime_error("Width of b is greater than width of a. Swap the arguments maybe");
    }

    // activations initialized to 0
    for(int i=sh_start; i<sh_end; ++i) act[i - sh_begin] = 0;

    // outer loop with j for cache efficiency

    for(int shift=sh_start; shift<sh_end; ++shift){
        // b entering a
        if(shift <= 0){
            for(int j=a_start.second; j<a_start.second+height; ++j){
                for(int i=0; i<width_b+shift; ++i){
                    act[shift - sh_begin] += a.ploc(i, j) * b.ploc(i-shift, j);
                }
            }
        }
        // b fully inside a
        else if ((width_a - shift) >= width_b){
            for(int j=a_start.second; j<a_start.second+height; ++j){
                for(int i=0; i<width_b; ++i){
                    act[shift - sh_begin] += a.ploc(i+shift, j) * b.ploc(i, j);
                }
            }
        }
        // b leaving a
        else{
            for(int j=a_start.second; j<a_start.second+height; ++j){
                for(int i=0; i<width_a-shift; ++i){
                    act[shift - sh_begin] += a.ploc(i+shift, j) * b.ploc(i, j);
                }
            }
        }
    }
}



// wrapper around x_correlate_region, creates 4 threads (hopefully user has a 4-core CPU)
std::pair<int, float> x_correlate(const BMP_image& a, std::pair<int, int> a_start, std::pair<int, int> a_end,
                                  const BMP_image& b, std::pair<int, int> b_start, std::pair<int, int> b_end)
{
    // each thread correlates 1/4th of total shift space
    int width_a = a_end.first - a_start.first + 1;
    int width_b = b_end.first - b_start.first + 1;

    // shift space
    // int sh_start = -width_b/4+1;
    // TODO restore this
    int sh_start = 0;                  // all images are shifted in one direction
    int sh_end = width_a/4;
    int sh_len = sh_end - sh_start;

    // end result is combination of threads' results, threads write here
    sim_t *res = new sim_t[sh_len];

    // starting threads with their corresponding regions
    // it's ugly, I know
    std::thread t1(&x_correlate_region, std::ref(a), a_start, a_end, std::ref(b), b_start, b_end, sh_start, sh_start+sh_len/4, sh_start, res);
    std::thread t2(&x_correlate_region, std::ref(a), a_start, a_end, std::ref(b), b_start, b_end, sh_start+sh_len/4, sh_start+sh_len/2, sh_start, res);
    std::thread t3(&x_correlate_region, std::ref(a), a_start, a_end, std::ref(b), b_start, b_end, sh_start+sh_len/2, sh_start+sh_len*3/4, sh_start, res);
    std::thread t4(&x_correlate_region, std::ref(a), a_start, a_end, std::ref(b), b_start, b_end, sh_start+sh_len*3/4, sh_start+sh_len, sh_start, res);
    
    // waiting for threads to finish
    t1.join();
    t2.join();
    t3.join();
    t4.join();

    // find minimum activation
    int fs = 0;
    for(int i=0; i<sh_len; ++i) if(res[i] < res[fs]) fs = i;

    // score (confidence) is calculated linearly: 1 at 0 diff, 0 at maximum possible diff
    float score = res[fs] * -1/(width_a*(a_end.second - a_start.second + 1)) + 1;
    
    // -------------------------------------------------------------------------
    // for(int i=0; i<sh_len-1; ++i){
    //     std::cout << res[i] << ",";
    // }
    // std::cout << res[sh_len-1] << std::endl;
    // -------------------------------------------------------------------------

    delete[] res;

    if(score < SCORE_THRESHOLD){
        std::cout << "Warning: correlation between regions of images '" << a.filenm << "' and '" << b.filenm
                  << "' resulted in a match scoring " << score << ", lower than threshold (" << SCORE_THRESHOLD << ")" << std::endl;
    }

    return { fs+sh_start, score };
}



// resizes region of image a (enlarges over x axis with given scale factor)
// (linear interpolation) and writes it starting at the specified location in image b
void x_enlarge_region(const BMP_image& a, std::pair<int, int> a_start, std::pair<int, int> a_end,
                            BMP_image& b, std::pair<int, int> b_start, float scale)
{
    int height = a_end.second - a_start.second;
    int width = (a_end.first - a_start.first) + 1;
    int new_w = floor(width * scale);
    float x, x_0, x_1;  // source pixel and its neighbors

    // outer loop with j for cache efficiency
    for(int j=0; j<height; ++j){
        for(int i=0; i<new_w; ++i){
            x = i / scale;
            x_0 = floor(x);
            x_1 = ceil(x);
            
            if(x_0 == x_1) x_1 += 1.0f;         // x is an integer
            if(x_1 == width) x_1 = width-1;     // out of bounds : replicate edge pixel
            if(x_0 == x_1) x_0 -= 1.0f;
            
            // linear interpolation between neighbor pixels
            b(b_start.first+i, b_start.second+j) = a.ploc(a_start.first+x_0, a_start.second+j)*(x_1 - x) +
                                                   a.ploc(a_start.first+x_1, a_start.second+j)*(x - x_0);
        }
    }
}



// returns a vector of shifts between img1 and img2 using strips of th (thickness) pixels
std::vector<float> calc_psf(const BMP_image& img1, const BMP_image& img2, int* maxshift, unsigned int th /* = 100 */){
    int n_strips = (int) ceil((float) img1.info_h.height / th);
    std::vector<int> sh;
    std::vector<float> psf;
    sh.reserve(n_strips);
    psf.reserve(n_strips);
    unsigned int w = img1.info_h.width;
    unsigned int h = img1.info_h.height;
    int max_shift = 0;

    for(int i=0; i<n_strips; ++i){
        auto corr = x_correlate(img1, {0, i*th}, {w-1, std::min((i+1)*th-1, h-1)}, img2, {0, i*th}, {w-1, std::min((i+1)*th-1, h-1)});
        sh.push_back(corr.first);
        if(corr.first > max_shift) max_shift = corr.first;
        // std::cout << std::setw(4) << i*th << " - " << std::setw(4) << min((i+1)*th-1, h-1) << " : " << corr.first << ", " << corr.second << endl;
    }

    *maxshift = max_shift;

    for(const int& s : sh) psf.push_back((float) max_shift/s);

    // handle +- inf or nan values
    for(int i=0; i<psf.size(); ++i) if(std::isinf(psf[i]) || std::isnan(psf[i])) psf[i] = (i == 0) ? 0 : psf[i-1];

    return psf;
}


// resizes and centers an image
BMP_image psf_resize(const BMP_image img, std::vector<float> psf, unsigned int th){
    // max psf
    float max_psf = 0;
    for(auto& p : psf) if(p > max_psf) max_psf = p;

    unsigned int width = img.info_h.width;
    unsigned int height = img.info_h.height;
    int new_w = width * max_psf;
    
    BMP_image res(new_w, height);

    // resize each strip and write centered
    for(int i=0; i<psf.size(); ++i){
        x_enlarge_region(img, {0, i*th}, {width-1, std::min((i+1)*th, height)}, res, {(int) floor((new_w - floor(width*psf[i]))/2), i*th}, psf[i]);
    }

    return res;
}



// cuts a strip of given width from center of img
BMP_image cut_strip(const BMP_image& img, unsigned int strip_width){
    unsigned int width = img.info_h.width;
    unsigned int height = img.info_h.height;
    unsigned int strip_start = (unsigned int) floor((float) (width - strip_width)/2);

    BMP_image res(strip_width, height);

    for(int j=0; j<height; ++j){
        for(int i=0; i<strip_width; ++i){
            res(i, j) = img.ploc(i+strip_start, j);
        }
    }

    return res;
}



// glues images together
BMP_image merge(std::vector<const BMP_image *> imgs){
    // calculate total width
    unsigned int res_width = 0;
    for(auto i : imgs) res_width += i->info_h.width;

    BMP_image res(res_width, imgs[0]->info_h.height);

    int c_width = 0;

    for(int n=0; n<imgs.size(); ++n){
        for(int j=0; j<res.info_h.height; ++j){
            for(int i=0; i<imgs[n]->info_h.width; ++i){
                res(c_width+i, j) = imgs[n]->ploc(i, j);
            }
        }
        c_width += imgs[n]->info_h.width;
    }

    // interpolate touching regions to smooth out stitches
    c_width = imgs[0]->info_h.width;
    for(int n=1; n<imgs.size(); ++n){
        for(int j=0; j<res.info_h.height; ++j){
            res(c_width, j)   = res(c_width-4, j)*0.5       + res(c_width+4, j)*0.5;
            res(c_width-1, j) = res(c_width-4, j)*(5.0/8.0) + res(c_width+4, j)*(3.0/8.0);
            res(c_width+1, j) = res(c_width-4, j)*(3.0/8.0) + res(c_width+4, j)*(5.0/8.0);
            res(c_width-2, j) = res(c_width-4, j)*(6.0/8.0) + res(c_width+4, j)*(2.0/8.0);
            res(c_width+2, j) = res(c_width-4, j)*(2.0/8.0) + res(c_width+4, j)*(6.0/8.0);
            res(c_width-3, j) = res(c_width-4, j)*(7.0/8.0) + res(c_width+4, j)*(1.0/8.0);
            res(c_width+3, j) = res(c_width-4, j)*(1.0/8.0) + res(c_width+4, j)*(7.0/8.0);
            // res(c_width-4, j) = res(c_width-6, j)*(10.0/12.0) + res(c_width+6, j)*(2.0 /12.0);
            // res(c_width+4, j) = res(c_width-6, j)*(2.0 /12.0) + res(c_width+6, j)*(10.0/12.0);
            // res(c_width-5, j) = res(c_width-6, j)*(11.0/12.0) + res(c_width+6, j)*(1.0 /12.0);
            // res(c_width+5, j) = res(c_width-6, j)*(1.0 /12.0) + res(c_width+6, j)*(11.0/12.0);
        }
        c_width += imgs[n]->info_h.width;
    }

    return res;
}



// psf file I/O

void write_psf(const char* filename, const std::vector<float>& psf, unsigned int resolution, int maxshift){
    std::ofstream out(filename);
    if(!out){
        // error opening file
        std::cerr << "Unable to write to file \'" <<filename << "\'" << std::endl;
        exit(1);
    }
    out << psf.size() << std::endl;
    out << resolution << std::endl;
    out << maxshift << std::endl;
    for(auto& p : psf) out << p << std::endl;
    out.close();
}


std::vector<float> read_psf(const char* filename, unsigned int* resolution, int* maxshift){
    std::ifstream in(filename);
    if(!in){
        // error opening file
        std::cerr << "Unable to read from file \'" <<filename << "\'" << std::endl;
        exit(1);
    }
    std::vector<float> psf;
    int n_strips;
    float p;
    in >> n_strips;
    psf.reserve(n_strips);
    in >> *resolution;
    in >> *maxshift;
    for(int i=0; i<n_strips; ++i){
        in >> p;
        psf.push_back(p);
    }
    return psf;
}



#endif // __UTIL_HPP