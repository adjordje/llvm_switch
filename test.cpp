#include <iostream>

void super_foo() {
    int a = 5000;
    int b = 2000;

    switch(a) {
        case 1000: b++; break;
        case 2000: b--; break;
        case 3000: b-=2; break;
        case 4000: b-=3; break;
        default: b*=100; break; 
    }

    switch(b) {
        case 3000: b++; break;
        case 4000: b--; break;
        default: b+=200; break; 
    }

    switch(b) {
        case 5000: b++; break;
        case 6000: b--; break;
        default: b-=300; break; 
    }
}
