#include <algorithm>
#include <iostream>
#include <iomanip>
#include <thread>
#include <random>
#include <chrono>

#define MAX_ITERATIONS 2e7

const int REND = 550;
const int TITAN = 700;
const int ARTIFICE = 1500;
const int THREADS = std::thread::hardware_concurrency();

class SFC64 {
  public:
  SFC64(uint64_t seed);
  uint64_t next();
  
  private:
  uint64_t a, b, c, counter;
};

class ThreadInfo {
  public:
  SFC64 random;

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
  
  ThreadInfo(int version, int currentQuota, int numberQuota, int shipScrap, int oversell, int average, int targetQuota, int threadNumber, int seed) noexcept;
};

SFC64::SFC64(uint64_t seed) {
  a = seed;
  b = seed ^ 0xdeadbeefcafebabe;
  c = (seed << 32) | (seed >> 32);
  counter = 1;
  
  next();
}

uint64_t SFC64::next() {
  uint64_t result = a + b + counter++;
  a = b ^ (b >> 11);
  b = c + (c << 3);
  c = ((c << 24) | (c >> 40)) + result;
  return result;
}

ThreadInfo::ThreadInfo(int version, int currentQuota, int numberQuota, int shipScrap, int oversell, int average, int targetQuota, int threadNumber, int seed) noexcept : version(version), currentQuota(currentQuota), numberQuota(numberQuota), shipScrap(shipScrap), oversell(oversell), average(average), targetQuota(targetQuota), threadNumber(threadNumber), random(seed){
}

double dist0to1(uint64_t x) noexcept {
  // & to keep only the mantissa
  x &= 0b0'00000000000'1111111111111111111111111111111111111111111111111111;
  // | set the exponent so that the double is now between 1.0 and 1.9999999
  x |= 0b0'01111111111'0000000000000000000000000000000000000000000000000000;
  // reinterpret bits as a double, subtract 1
  return *reinterpret_cast<double *>(&x) - 1.0;
}

//Curve that the game uses to skew the random number generator towards 0
double qCurve(double x) noexcept {
  double f;
  if (x <= 0.1172)
    return ((120.0163409 * x - 50.5378659) * x + 7.4554) * x - 0.503;
  else if (x <= 0.8804)
    return ((0.57326727 * x - 0.8792601) * x + 0.73737564) * x - 0.20546592;
  else
    return ((120.77228959 * x - 313.35391533) * x + 271.4424619) * x - 78.35783615;
}

int incQuota(int num, double r) noexcept {
  return (int)(100*(1 + num*num/16.0)*(1 + qCurve(r)));
}

void threadedPassTest(ThreadInfo* threadData) noexcept {
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
	// + 5 is to make it equal to calling ceil, simplification of (5 * threadData->oversell - 1) / 6 + 1 that compiles better
    int needSell = (5*threadData->oversell + 5)/6;
    int quota = threadData->currentQuota;

    for (int j = threadData->numberQuota; quota < threadData->targetQuota; j++) {
      if (moonPrice - 75 > quota)
        // + 5 is to make it equal to calling ceil, simplification of (5 * threadData->oversell - 1) / 6 + 1 that compiles better
        needSell += (5*moonPrice + 75 + quota + 5)/6;
      else
        needSell += std::max(moonPrice, quota);

      quota += incQuota(j, dist0to1(threadData->random.next()));
      finalJ = j;
    }

    if (threadData->shipScrap + 3*threadData->average*(finalJ - threadData->numberQuota + 1) >= needSell)
      passes++;
  }

  threadData->threadReturn = passes;
}

int main(int argc, char *argv[]) {
  int version;
  int currentQuota;
  int numberQuota;
  int shipScrap;
  int oversell;
  int average;
  int targetQuota;
  
  if(argc == 8){
    version = std::stoi(argv[1]);
    currentQuota = std::stoi(argv[2]);
    numberQuota = std::stoi(argv[3]);
    shipScrap = std::stoi(argv[4]);
    oversell = std::stoi(argv[5]);
    average = std::stoi(argv[6]);
    targetQuota = std::stoi(argv[7]);
  }
  else {
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
  }

  const auto start = std::chrono::high_resolution_clock::now();
  std::thread threaded[THREADS];
  // allocate the memory for an array of ThreadInfo's without initializing any of them(essentially the same as just using malloc)
  alignas(ThreadInfo) ThreadInfo *perThreadInfo = static_cast<ThreadInfo *>(::operator new[](THREADS * sizeof(ThreadInfo))); // this is kind of disgusting
  for (int i = 0; i < THREADS; i++) {
    std::random_device rngSeed;
    // initialize the ThreadInfo's into memory using placement new
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
  const auto end = std::chrono::high_resolution_clock::now();

  std::cout << std::fixed;
  std::cout << std::setprecision(8);
  chance = chance / (((MAX_ITERATIONS - 1) / THREADS + 1) * THREADS);
  std::cout << 100*chance << '%' << std::endl;
  std::cout << "time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << std::endl;

  return 0;
}
