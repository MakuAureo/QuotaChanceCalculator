# Quota Chance Calculator
Simulates quota rolls to estimate the chance of getting to a desired quota.

## Compiling
`g++ quotaChance.cpp`

## Usage
The program takes 7 inputs:
- Version:
  Acceptable values:
  - 40 (Titan q2+)
  - 49 (Rend q2+)
  - 50/56/62/69 (Artifice q2+)
  - -1 (No paid moons)

- Current quota:
  What the current quota is (Ex: 130, 237, ...)

- Current quota number:
  What number is the current quota (Ex: 1, 2, ...)

- Ship scrap:
  How much scrap the ship has at the start of the current quota (after selling last quota)

- Oversell:
  How much is planned to be bought over the quota by overselling (Ex: 700 for jet)

- Estimated average:
  Estimate of how much scrap is collected from that point onwards

- Target quota:
  Desired quota to be reached
