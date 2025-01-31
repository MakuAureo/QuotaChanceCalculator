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
  const int threadNumber;

  const int version;
  const int currentQuota;
  const int numberQuota;
  const int shipScrap;
  const int oversell;
  const int average;
  const int targetQuota;

  int threadReturn;

  ThreadInfo() noexcept : version(0), currentQuota(0), numberQuota(0), shipScrap(0), oversell(0), average(0), targetQuota(0), threadNumber(0), random(std::mt19937(0)) {
  }
  
  ThreadInfo(int version, int currentQuota, int numberQuota, int shipScrap, int oversell, int average, int targetQuota, int threadNumber, int seed) noexcept : version(version), currentQuota(currentQuota), numberQuota(numberQuota), shipScrap(shipScrap), oversell(oversell), average(average), targetQuota(targetQuota), threadNumber(threadNumber), random(std::mt19937(seed)) {
  }
};

//Curve that the game uses to skew the random number generator towards 0
long double qCurve(long double x) noexcept {
  long double f;
  if (x <= 0.1172)
    f = ((120.0163409 * x - 50.5378659) * x + 7.4554) * x - 0.503;
  else if (x <= 0.8804)
    f = ((0.57326727 * x - 0.8792601) * x + 0.73737564) * x - 0.20546592;
  else
    f = ((120.77228959 * x - 313.35391533) * x + 271.4424619) * x - 78.35783615;

  return f;
}  

int incQuota(int num, long double r) noexcept {
  return (int)(100*(1 + num*num/16.0)*(1 + qCurve(r)));
}

void threadedPassTest(ThreadInfo* threadData) noexcept {
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

  int iterations = (MAX_ITERATIONS - 1) / threadData->threadCount + 1;
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
  int version;
  int currentQuota;
  int numberQuota;
  int shipScrap;
  int oversell;
  int average;
  int targetQuota;

  std::cout << "Version: ";
  std::cin >> version;

  std::cout << "Current quota: ";
  std::cin >> currentQuota;

  std::cout << "Current quota number: ";
  std::cin >> numberQuota;

  std::cout << "Ship scrap: ";
  std::cin >> shipScrap;

  std::cout << "Oversell: ";
  std::cin >> oversell;

  std::cout << "Estimated average: ";
  std::cin >> average;

  std::cout << "Target quota: ";
  std::cin >> targetQuota;

  std::thread threaded[THREADS];
  alignas(ThreadInfo) ThreadInfo *perThreadInfo = static_cast<ThreadInfo *>(::operator new[](THREADS * sizeof(ThreadInfo))); // this is kind of disgusting
  for (int i = 0; i < THREADS; i++) {
    std::random_device rngSeed;
    new (&perThreadInfo[i]) ThreadInfo(version, currentQuota, numberQuota, shipScrap, oversell, average, targetQuota, i, rngSeed());
    threaded[i] = std::thread(threadedPassTest, &perThreadInfo[i]);
  }

  double chance = 0;
  for (int i = 0; i < THREADS; i++) {
    threaded[i].join();
    chance += (double)perThreadInfo[i].threadReturn;
	perThreadInfo[i].~ThreadInfo(); // call destructor
  }
  ::operator delete[](perThreadInfo); // free memory

  std::cout << std::fixed;
  std::cout << std::setprecision(8);
  chance = chance / (((MAX_ITERATIONS - 1) / THREADS + 1) * THREADS);
  std::cout << 100*chance << '%' << std::endl;

  return 0;
}
