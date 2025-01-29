#include <algorithm>
#include <iostream>
#include <iomanip>
#include <thread>
#include <random>

#define MAX_ITERATIONS 2e7

const int REND = 550;
const int TITAN = 700;
const int ARTIFICE = 1500;
const int THREADS = std::thread::hardware_concurrency();

class ThreadInfo {
  public:
  std::mt19937 random;

  const int threadCount = THREADS;
  int threadNumber;

  int version;
  int currentQuota;
  int numberQuota;
  int shipScrap;
  int oversell;
  int average;
  int targetQuota;

  int threadReturn;

  ThreadInfo& operator=(const ThreadInfo& ti) {
    if (this != &ti) {
      version = ti.version;
      currentQuota = ti.currentQuota;
      numberQuota = ti.numberQuota;
      shipScrap = ti.shipScrap;
      oversell = ti.oversell;
      average = ti.average;
      targetQuota = ti.targetQuota;
    }

    return *this;
  }
};

//Curve that the game uses to skew the random number generator towards 0
long double qCurve(long double x) {
  long double f;
  if (x <= 0.1172)
    f = ((120.0163409 * x - 50.5378659) * x + 7.4554) * x - 0.503;
  else if (x <= 0.8804)
    f = ((0.57326727 * x - 0.8792601) * x + 0.73737564) * x - 0.20546592;
  else
    f = ((120.77228959 * x - 313.35391533) * x + 271.4424619) * x - 78.35783615;

  return f;
}  

int incQuota(int num, long double r) {
  return (int)(100*(1 + num*num/16.0)*(1 + qCurve(r)));
}

void threadedPassTest(ThreadInfo* threadData) {
  std::uniform_real_distribution<long double> unit(0.0, 1.0);
  int passes = 0;

  int moonPrice = 0;
  switch (threadData->version) {
    case 40: 
      moonPrice = TITAN;
      break;

    case 49:
      moonPrice = REND;
      break;

    case -1:
      moonPrice = 0;
      break;

    default:
      moonPrice = ARTIFICE;
      break;
  }

  int iterations = 1 + MAX_ITERATIONS/(threadData->threadCount+1);
  int finalJ;
  for (int i = threadData->threadNumber*iterations; i < (threadData->threadNumber + 1)*iterations && i < MAX_ITERATIONS; i++) {
    int needSell = (5*threadData->oversell + 5)/6;
    int quota = threadData->currentQuota;

    for (int j = threadData->numberQuota; quota < threadData->targetQuota; j++) {
      if (moonPrice - 75 > quota)
        needSell += (5*moonPrice + 75 + quota + 5)/6;
      else
        needSell += std::max(moonPrice, quota);

      quota += incQuota(j, unit(threadData->random));
      finalJ = j;
    }

    if (threadData->shipScrap + 3*threadData->average*(finalJ - threadData->numberQuota + 1) >= needSell)
      passes++;
  }

  threadData->threadReturn = passes;
}

int main() {
  ThreadInfo runData;

  std::cout << "Version: ";
  std::cin >> runData.version;

  std::cout << "Current quota: ";
  std::cin >> runData.currentQuota;

  std::cout << "Current quota number: ";
  std::cin >> runData.numberQuota;

  std::cout << "Ship scrap: ";
  std::cin >> runData.shipScrap;

  std::cout << "Oversell: ";
  std::cin >> runData.oversell;

  std::cout << "Estimated average: ";
  std::cin >> runData.average;

  std::cout << "Target quota: ";
  std::cin >> runData.targetQuota;

  std::thread threaded[THREADS];
  ThreadInfo perThreadInfo[THREADS];
  for (int i = 0; i < THREADS; i++) {
    std::random_device rngSeed;

    perThreadInfo[i] = runData;
    perThreadInfo[i].threadNumber = i;
    perThreadInfo[i].random = std::mt19937(rngSeed());

    threaded[i] = std::thread(threadedPassTest, &perThreadInfo[i]);
  }

  double chance = 0;
  for (int i = 0; i < THREADS; i++) {
    threaded[i].join();
    chance += (double)perThreadInfo[i].threadReturn;
  }

  std::cout << std::fixed;
  std::cout << std::setprecision(8);
  chance = chance/MAX_ITERATIONS;
  std::cout << 100*chance << '%' << std::endl;

  return 0;
}
