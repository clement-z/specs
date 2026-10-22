#include <complex>
#include <cmath>
#include <limits>
#include <numbers>
#include <iostream>
using std::is_floating_point_v; using std::is_arithmetic_v;
template<typename FloatT, typename T>
requires is_floating_point_v<FloatT> && is_arithmetic_v<T>
bool angleIsEqual(std::complex<FloatT> v, T angle)
{
    return std::fmod(std::arg(v) - angle, 2*std::numbers::pi) < std::numeric_limits<FloatT>::epsilon();
}
int main(){
  std::complex<double> v = std::polar(1.0, 0.1);
  std::cout << "actual 0.1 vs expected 2.39 -> " << angleIsEqual(v, 2.39) << " (should be false)\n";
  std::cout << "actual 0.1 vs expected -3.0 -> " << angleIsEqual(v, -3.0) << " (should be false)\n";
  std::complex<double> w = std::polar(1.0, 2.0);
  std::cout << "actual 2.0000000000001 vs 2.0 -> " << angleIsEqual(std::polar(1.0,2.0000000000001), 2.0) << " (should be true)\n";
  std::cout << "actual 2.0 vs 2.0+2pi -> " << angleIsEqual(w, 2.0+2*std::numbers::pi) << " (should be true; modulo 2pi)\n";
  std::cout << "actual -3.14159 vs +3.14159 (same angle) -> " << angleIsEqual(std::polar(1.0,-3.14159), 3.14159) << " (should be true)\n";
}
