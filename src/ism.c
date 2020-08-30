#include <stdio.h>
#include <inttypes.h>
#include <ism.h>
#include "fastrng.h"
#include "perlin.h"

int main(int argc, char *argv[]) {
    uint32_t seed = strtoumax(argv[1], NULL, 10);
    rand_seed(seed);
    noise_ptable ptable = create_noise_ptable(rand_int32);
    printf("Ism %d.%d %d\n", ISM_VERSION_MAJOR, ISM_VERSION_MINOR, seed);
    for (float i = 0.0f; i < 5.0f; i += 1.0f) {
        printf("height %.5f\n", noise1(ptable, i * .137f + 0.072));
    }
    return 0;
}
