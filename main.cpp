#include <iostream>
#include <complex>
#include <cmath>

#include <atomic>
#include <chrono>
#include <thread>

#include "color.h"
#include "rbuffer.h"
#include "line.h"

using std::log;
typedef std::complex<double> cmplxd;

uint64_t height = 16000, width = 16000;     //  image height and width
uint64_t max_iter = 1024;
double radius = 10000;
double zoom = 0.75;
cmplxd origin = cmplxd(-0.5, 0);

int threads = 4;

rbuffer<line<cmplxd>> * lines;   //  Lines of the image to be passed into the processing
rbuffer<line<rgb>> *    processed_lines;

/* Count the numeber of itertions it takes until z_n > radius where z_n = z_(n-1)^2 + pt and z_0 = 0 */
uint64_t mandelbrot_iter(cmplxd * pt) {
    uint64_t i;
    cmplxd r = 0;
    for(i = 0; i < max_iter; i++) {
        r = (r * r) + *pt;
        if(abs(r) >= radius){
            break;
        }
    }
    *pt = r;
    return i;
}

void print_lines() {
    std::cout << "P3\n" << width << ' ' << height <<  "\n255\n";
    while(!processed_lines->is_empty() || processed_lines->is_open()){
        line <rgb> r;
        processed_lines->remove(&r);
        for(uint64_t i = 0; i < width; i++){
            r.row[i].draw(std::cout);
        }
        delete r.row;
    }
}

std::atomic_size_t rw_ord = height - 1;
void render_line() {
    while(!lines->is_empty() || lines->is_open()) {
        line<cmplxd> l;
        lines->remove(&l);     

        line<rgb> lc;
        lc.row_indx = l.row_indx;
        lc.row = (rgb *) malloc(width * sizeof(rgb));

        for(uint64_t i = 0; i < width; i++) {
            lc.row_indx = l.row_indx;

            uint64_t n = mandelbrot_iter(&(l.row[i]));
            double sn = (double)n - log(log( abs((l.row[i])))/log(radius))/log(2);
            
            double h = sn * (360/max_iter) + 235;
            if(h > 360){
                h = 720 - h;
            }
            double s = 1 - (double)sn/(double)max_iter;
            double v = (n < max_iter) ? sn/((double) max_iter) : 0;

            rgb r = hsv(h, s ,v).to_rgb();
            lc.row[i] = r;
        }

        while(lc.row_indx != rw_ord) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        processed_lines->add(&lc);
        rw_ord--;
        delete l.row;
    }
}

void make_lines() {
    std::thread wrkrs [threads];
    for(int i = 0; i < threads; i++){
        wrkrs[i] = std::thread(render_line);
    }
    double scale = 0.5 * (height < width ? height : width);
    double h_height = 0.5 * height;
    double h_width = 0.5 * width;
    
    for(uint64_t i = height - 1; i >= 0; i--) {
        line<cmplxd> l;
        l.row_indx = i;
        l.row = (cmplxd *) malloc(width * sizeof(cmplxd));
        for(uint64_t j = 0; j < width; j++){
           
            double r = ((j - h_width)/scale) * 1/zoom;
            double im = ((i - h_height)/scale) * 1/zoom;

            cmplxd pt = cmplxd(r, im);
            pt += origin;

            l.row[j] = pt; 
        }
        //blocks when full as to not fill memory
        lines->add(&l);
        if(i == 0){
            break;
        }
    }

    //Signal done adding lines
    lines->finish();
    for(int i = 0; i < threads; i++){
        wrkrs[i].join();
    }
    processed_lines->finish();
}

void make_image() {
    std::thread maker (make_lines);
    std::thread prntr (print_lines);


    maker.join();
    prntr.join();

    return;
}

int main(int argc, char ** argv) {
    lines = new rbuffer<line<cmplxd>>(400);    
    processed_lines = new rbuffer<line<rgb>>(100);

    make_image();
}
