#include <random>
#include <math.h>
#include <iostream>
#include <iomanip>

#define MAX_ITERATIONS 2e6

long rend = 550;
long titan = 700;
long artifice = 1500;

//Curve that the game uses to skew the random number generator towards 0
long double qCurve(long double x) {
  long double f;
  if (x <= 0.1172)
    f = 120.0163409*x*x*x-50.5378659*x*x+7.4554*x-0.503;
  else if (x <= 0.8804)
    f = 0.57326727*x*x*x-0.8792601*x*x+0.73737564*x-0.20546592;
  else
    f = 120.77228959*x*x*x-313.35391533*x*x+271.4424619*x-78.35783615;

  return f;
}

int main(int argc, char** argv) {
  std::random_device rng;
  std::mt19937 gen(rng());
  std::uniform_real_distribution<long double> unit(0.0, 1.0);

  auto incQuota = [](long int num, long double r) {
    return floor(100*(1 + num*num/16.0)*(1+qCurve(r)));
  };

  std::cout << "Version: ";
  long vers;
  std::cin >> vers;

  std::cout << "Current quota: ";
  long currQ;
  std::cin >> currQ;

  std::cout << "Current quota number: ";
  long numbQ;
  std::cin >> numbQ;

  std::cout << "Ship scrap: ";
  long shipS;
  std::cin >> shipS;

  std::cout << "Oversell: ";
  long overS;
  std::cin >> overS;

  std::cout << "Estimated average: ";
  long currA;
  std::cin >> currA;

  std::cout << "Target quota: ";
  long targQ;
  std::cin >> targQ;

  double chance = 0;
  for (int i = 0; i < MAX_ITERATIONS; i++) {
    long needS = ceil(5*overS/6.0);
    long quota = currQ;

    for (int j = numbQ; quota < targQ; j++) {
      switch (vers) {
        case 40: 
          if (titan - 75 > quota)
            needS += ceil((5*titan+75+quota)/6.0);
          else
            needS += fmax(titan, quota);
          break;

        case 49:
          if (rend - 75 > quota)
            needS += ceil((5*rend+75+quota)/6.0);
          else
            needS += fmax(rend, quota);
          break;

        case -1:
          needS += quota;
          break;

        default:
          if (artifice - 75 > quota)
            needS += ceil((5*artifice+75+quota)/6.0);
          else
            needS += fmax(artifice, quota);
          break;
      }
      quota += incQuota(j, unit(rng));

      if ((quota >= targQ) && (shipS + 3*currA*(j-numbQ+1) >= needS))
        chance++;
    }
  }

  std::cout << std::fixed;
  std::cout << std::setprecision(8);
  chance = chance/MAX_ITERATIONS;
  std::cout << 100*chance << '%' << std::endl;

  return 0;
}
