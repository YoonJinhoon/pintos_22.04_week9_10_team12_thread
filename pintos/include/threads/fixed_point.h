#define F (1 << 14) 
#define INT_MAX ((1 << 31) -1)
#define INT_MIN (-1 << 31) 
#include <stdint.h>
static inline int int_to_fp(int n); /* integer를 fixed point로 전환 */
static inline int fp_to_int_round(int x); /* FP를 int로 전환(반올림) */
static inline int fp_to_int(int x); /* FP를 int로 전환(버림) */
static inline int add_fp(int x, int y); /* FP의 덧셈 */
static inline int add_mixed(int x, int n); /* FP와 int의 덧셈 */
static inline int sub_fp(int x, int y); /* FP의 뺄셈(x-y) */
static inline int sub_mixed(int x, int n); /* FP와 int의 뺄셈(x-n) */
static inline int mult_fp(int x, int y); /* FP의 곱셈 */
static inline int mult_mixed(int x, int y); /* FP와 int의 곱셈 */
static inline int div_fp(int x, int y); /* FP의 나눗셈(x/y) */
static inline int div_mixed(int x, int n); /* FP와 int 나눗셈(x/n) */

static inline int int_to_fp(int n){
return n * F;
}
static inline int fp_to_int_round(int x){
        return x / F;
}
static inline int fp_to_int(int x){
        if (x >= 0)
         return (x + F / 2) / F; 
        else
        return (x - F / 2) / F;
}
static inline int add_fp(int x, int y){
        return x + y;
}
static inline int add_mixed(int x, int n){
        return x + n * F;
}
static inline int sub_fp(int x, int y){
        return x - y ;
}
static inline int sub_mixed(int x, int n){
        return x - n * F;
}
static inline int mult_mixed(int x, int y){
        return (int)((((long long) x) * y));
}
static inline int div_fp(int x, int y){
        return (int)((((long long) x) * F) / y);
}
static inline int div_mixed(int x, int n){
        return x / n ;
}
static inline int mult_fp (int x, int y) {
	return (int)(((int64_t) x) * y / F);
}