#include <mpfr.h>
#include <iostream>
#include <string>
#include <cmath>

int main() {
    const int decimal_digits = 10000; // 要输出的小数位
    const int guard = 50;             // 多算几位防止误差
    const int total_digits = decimal_digits + guard;

    // 十进制位数转二进制位数，再多留 64 bit
    mpfr_prec_t prec = (mpfr_prec_t)std::ceil(total_digits * std::log2(10.0)) + 64;

    mpfr_t pi;
    mpfr_init2(pi, prec);
    mpfr_const_pi(pi, MPFR_RNDN);     // 直接算 π

    mpfr_exp_t exp;
    char* str = mpfr_get_str(nullptr, &exp, 10, total_digits, pi, MPFR_RNDN);

    std::string s(str);
    mpfr_free_str(str);
    mpfr_clear(pi);

    std::cout << s[0] << '.' << s.substr(1, decimal_digits) << std::endl;
    return 0;
}
