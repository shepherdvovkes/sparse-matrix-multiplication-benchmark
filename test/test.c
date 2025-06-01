#include <stdio.h>
#include "../dense/dense.h"
#include "../sparse/bcsr.h"

int main() {
    dense_elem_t data[16] = {
        -1., -1.,  0., -1.,
         0., -1.,  0.,  0.,
         0.,  0., -1., -1.,
         0.,  0., -1.,  0.,
    };
    dense_t X = (dense_t) data;
    bcsr_t *X_sparse = bcsr_from_dense(X, 4, 4, 2, 2);

    printf("b_row_start: ");
    for (int i = 0; i < 3; i++) {
        printf("%d ", X_sparse->b_row_start[i]);
    }
    printf("\n");
    printf("b_col_index: ");
    for (int i = 0; i < 3; i++) {
        printf("%d ", X_sparse->b_col_idx[i]);
    }
    printf("\n");

    printf("num blocks: %d\n", X_sparse->k);

    return 0;
}
