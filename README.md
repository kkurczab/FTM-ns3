Master Thesis Title: Simulation Analysis of the IEEE 802.11mc Fine-Time Measurement Protocol
Author: mgr. inż. Krzysztof Kurczab
Date of Defense: 27.10.2023
University: AGH University in Kraków
Thesis supervisor: prof. dr hab. inż. Katarzyna Kosek-Szott

# FTM-ns3
The FTM protocol implementation in the NS3 simulator, provided by https://github.com/tkn-tub/wifi-ftm-ns3 and edited by me for the purpose of my Master Thesis.

In order to enable convenient  simulations, I added different simulation parameters, in the form of command line arguments to the https://github.com/kkurczab/FTM-ns3/blob/main/scratch/ftm-example.cc script. Additionally, the default setting is listed after running the **./waf --run "ftm-example --PrintHelp** command.

For specific parameter combinations, the simulation did not start due to a pronounced correlation between the _burstDuration_ and _minDeltaFtm_ parameters. Consequently, the code was adjusted to adhere to the rules listed as in the algorithm below:

If burstDuration = 5 and minDeltaFtm in {320, 640} **or**
burstDuration = 5 and ftmsPerBurst in {2, 3} and minDeltaFtm = 10 **or**
burstDuration = 11 and minDeltaFtm in {5, 10} **or**
burstDuration = 11 and ftmsPerBurst in {2, 3} and minDeltaFtm = 640
  **skip the combination**
EndIf
