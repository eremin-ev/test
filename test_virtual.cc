/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */


#include <iostream>

struct Shape {
    Shape(int w = 0, int h = 0)
    {
        width = w;
        height = h;
    }

    /* remove `virtual` to disable derived Rect and Tri override `area()` */
    virtual int area() const
    {
        int result = 0;
        std::cout << "Shape area: " << result << std::endl;
        return result;
    }

protected:
    int width, height;
};

struct Rect: public Shape {
    Rect(int w = 0, int h = 0)
        : Shape(w, h)
    { }

    int area() const override
    {
        int result = width*height;
        std::cout << "Rectangle area: " << result << std::endl;
        return result;
    }
};

struct Tri: public Shape {
    Tri(int w = 0, int h = 0)
        : Shape(w, h)
    { }

    int area() const override
    {
        int result = (width*height)/2;
        std::cout << "Triangle area: " << result << std::endl;
        return result;
    }
};

int main()
{
    const Shape *shp;
    Shape shape(10, 3);
    Rect rect(10, 5);
    Tri tri(10, 7);

    shp = &shape;
    shp->area();

    shp = &rect;
    shp->area();

    shp = &tri;
    shp->area();

    shape.area();

    const Shape &spr = tri;
    spr.area();

    return 0;
}
