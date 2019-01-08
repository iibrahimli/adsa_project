#ifndef __PIXEL_HPP
#define __PIXEL_HPP

#include <string>
#include <sstream>
#include <iomanip>


#define WHITE pixel(255, 255, 255)
#define BLACK pixel(  0,   0,   0)
#define RED   pixel(255,   0,   0)
#define GREEN pixel(  0, 255,   0)
#define BLUE  pixel(  0,   0, 255)


// similarity/difference between pixels
using sim_t = float;


// RGB pixel

#pragma pack(push, 1)  // 1 byte alignment for i/o
struct pixel3 {
    uint8_t b;    // blue
    uint8_t g;    // green
    uint8_t r;    // red


    pixel3() = default;


    pixel3(uint8_t red, uint8_t green, uint8_t blue)
    : b(blue), g(green), r(red) {}


    inline uint8_t clamp(int x){
        uint8_t ret;

        if(x > 255) ret = 255;
        else if(x < 0) ret = 0;
        else ret = x;

        return ret;
    }


    // sets color
    void set_c(uint8_t red, uint8_t green, uint8_t blue){
        b = blue;
        g = green;
        r = red;
    }


    pixel3& operator+=(const pixel3& rhs){
        this->b = clamp((int) this->b + rhs.b);
        this->g = clamp((int) this->g + rhs.g);
        this->r = clamp((int) this->r + rhs.r);
        return *this;
    }


    friend pixel3 operator+(pixel3 lhs, const pixel3& rhs){
        lhs += rhs;
        return lhs;
    }


    pixel3& operator-=(const pixel3& rhs){
        this->b = clamp((int) this->b - rhs.b);
        this->g = clamp((int) this->g - rhs.g);
        this->r = clamp((int) this->r - rhs.r);
        return *this;
    }


    friend pixel3 operator-(pixel3 lhs, const pixel3& rhs){
        lhs -= rhs;
        return lhs;
    }


    pixel3& operator*=(const float& scl){
        this->b = clamp((int) (this->b * scl));
        this->g = clamp((int) (this->g * scl));
        this->r = clamp((int) (this->r * scl));
        return *this;
    }


    friend pixel3 operator*(pixel3 lhs, const float& scl){
        lhs *= scl;
        return lhs;
    }


    std::string str(){
        std::stringstream ss;

        ss << "("
           << std::setw(3) << std::to_string(r) << ", "
           << std::setw(3) << std::to_string(g) << ", "
           << std::setw(3) << std::to_string(b) << ")";
        
        return ss.str();
    }

};
#pragma pack(pop)



// PIXEL4 - RGBA, works like pixel3 (color manipulation), just stores alpha

#pragma pack(push, 1)  // 1 byte alignment for i/o
struct pixel4 {
    uint8_t b;    // blue
    uint8_t g;    // green
    uint8_t r;    // red
    uint8_t a;    // alpha

    pixel4() = default;


    pixel4(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha=255)
    : b(blue), g(green), r(red), a(alpha) {};


    inline uint8_t clamp(int x){
        uint8_t ret;

        if(x > 255) ret = 255;
        else if(x < 0) ret = 0;
        else ret = x;

        return ret;
    }


    // sets color
    void set_c(uint8_t red, uint8_t green, uint8_t blue){
        b = blue;
        g = green;
        r = red;
    }


    pixel4& operator+=(const pixel4& rhs){
        this->b = clamp((int) this->b + rhs.b);
        this->g = clamp((int) this->g + rhs.g);
        this->r = clamp((int) this->r + rhs.r);
        return *this;
    }


    friend pixel4 operator+(pixel4 lhs, const pixel4& rhs){
        lhs += rhs;
        return lhs;
    }


    pixel4& operator-=(const pixel4& rhs){
        this->b = clamp((int) this->b - rhs.b);
        this->g = clamp((int) this->g - rhs.g);
        this->r = clamp((int) this->r - rhs.r);
        return *this;
    }


    friend pixel4 operator-(pixel4 lhs, const pixel4& rhs){
        lhs -= rhs;
        return lhs;
    }


    pixel4& operator*=(const float& scl){
        this->b = clamp((int) (this->b * scl));
        this->g = clamp((int) (this->g * scl));
        this->r = clamp((int) (this->r * scl));
        return *this;
    }


    friend pixel4 operator*(pixel4 lhs, const float& scl){
        lhs *= scl;
        return lhs;
    }

    std::string str(){
        std::stringstream ss;

        ss << "("
           << std::setw(3) << std::to_string(r) << ", "
           << std::setw(3) << std::to_string(g) << ", "
           << std::setw(3) << std::to_string(b) << ", "
           << std::setw(3) << std::to_string(a) << ")";
        
        return ss.str();
    }

};
#pragma pack(pop)


int abs(const pixel3& p){
    return (p.r + p.g + p.b)/3;
}


int abs(const pixel4& p){
    return (p.r + p.g + p.b)/3;
}


std::ostream& operator << (std::ostream& out, const pixel3& p){
    out << "("
        << std::setw(3) << std::to_string(p.r) << ", "
        << std::setw(3) << std::to_string(p.g) << ", "
        << std::setw(3) << std::to_string(p.b) << ")";
    
    return out;
}


std::ostream& operator << (std::ostream& out, const pixel4& p){
    out << "("
        << std::setw(3) << std::to_string(p.r) << ", "
        << std::setw(3) << std::to_string(p.g) << ", "
        << std::setw(3) << std::to_string(p.b) << ", "
        << std::setw(3) << std::to_string(p.a) << ")";

    return out;
}

// similarity operator : absolute difference
inline sim_t operator * (const pixel3& p1, const pixel3& p2){
    return (float) (abs(p1.r - p2.r) + abs(p1.g - p2.g) + abs(p1.b - p2.b)) / (3*255);
}


inline sim_t operator * (const pixel4& p1, const pixel4& p2){
    return (float) (abs(p1.r - p2.r) + abs(p1.g - p2.g) + abs(p1.b - p2.b)) / (3*255);
}


#endif // __PIXEL_HPP