#ifndef COLOR_H
#define COLOR_H

#include <iostream>

class rgb{
    public:
        int r, g, b;
        rgb(){r = 0; g = 0; b = 0;}
        rgb(double _r, double _g, double _b) {
            r = static_cast<int>(255.999 * _r);
            g = static_cast<int>(255.999 * _g);
            b = static_cast<int>(255.999 * _b);
        }

        void draw(std::ostream &out){
            out << r << ' ' << g << ' ' << b << '\n';
        }
};

class hsv{
    public:
        double h, s, v;

        hsv() {  h = 0; s = 0; v = 0; }
        hsv(double _h, double _s, double _v){
            h = _h;
            s = _s;
            v = _v;
        }
        
        rgb to_rgb() {
            double hh, p, q, t, ff;
            long i;
            if (s <= 0.0)
            { // < is bogus, just shuts up warnings
                return rgb(v, v, v);
            }
 
            hh = h;
            if (hh >= 360.0) {
                hh = 0.0;
            }
            hh /= 60.0;
            i = (long)hh;
            ff = hh - i;
            p = v * (1.0 - s);
            q = v * (1.0 - (s * ff));
            t = v * (1.0 - (s * (1.0 - ff)));

            switch (i)
            {
                case 0:
                    return rgb(v, t, p);
                case 1:
                    return rgb(q, v, p);
                case 2:
                    return rgb(p, v, t);
                case 3:
                    return rgb(p, q, v);
                case 4:
                    return rgb(t, q, v);
                case 5:
                default:
                    return rgb(v, p, q);
            }
        }
};
#endif
