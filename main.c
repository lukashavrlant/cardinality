#include <stdio.h>
#include "declarations.h"
#include "hll_file_consumer.h"
#include "tests/alltests.h"

int main(int argc, const char * argv[])
{
    printf("asdasd");
    if (argc > 1) {
        if (strcmp(argv[1], "--test") == 0) {
            
            run_all_tests();
            // return 0;
            hyperloglog(14, "/Users/lukashavrlant/Desktop/test");
        }
        uint b = 14;
        if (argc > 2) {
            b = (uint) atoi(argv[2]);
        }
        hyperloglog(b, argv[1]);
    } else {
        hyperloglog(14, "/Users/lukashavrlant/Desktop/test");
    }
    
    return 0;
}
