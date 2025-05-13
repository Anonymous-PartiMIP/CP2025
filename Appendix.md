# Supplementary Ablation Study  

To more clearly evaluate the effectiveness of the Reward mechanism (including its growth and decay components), we have conducted more detailed ablation experiments.
We compared the performance of **PartiMIP-SCIP** (full version) with the following three variants under 8-core and 16-core configurations:

* **PartiMIP-SCIP-RandomSelect**: Variable selection is completely random (corresponding to PartiMIP-R-SCIP in the original paper).
* **PartiMIP-SCIP-NoReward**: The entire Reward mechanism (including growth and decay) is removed, and decisions are made using only static heuristics (domain size and degree).
* **PartiMIP-SCIP-NoDecaying**: The Reward growth mechanism is retained, but the decaying mechanism is removed. Decisions are based on domain + degree + reward (without decay).

Experimental results are as follows:

| Solver                     | WIN (8 thr.) | W-Imp. (8 thr.) | WIN (16 thr.) | W-Imp. (16 thr.) |
| :------------------------- | :----------- | :-------------- | :------------ | :--------------- |
| PartiMIP-SCIP-RandomSelect | 151          | 0.0%            | 144           | 0.0%             |
| PartiMIP-SCIP-NoReward     | 156          | 3.3%            | 151           | 4.9%             |
| PartiMIP-SCIP-NoDecaying   | 158          | 4.6%            | 155           | 7.6%             |
| PartiMIP-SCIP              | 161          | 6.6%            | 162           | 12.5%            |

From the table above, it can be seen that:

* **Effectiveness of the Reward Mechanism**: The performance improvement from NoReward (static heuristics only) to NoDecaying (static heuristics + basic Reward growth) indicates the positive role of the Reward learning mechanism itself.
* **Necessity of the Decaying Mechanism**: The further performance improvement from NoDecaying to PartiMIP-SCIP (full version, with decay) clearly demonstrates that the Reward decaying mechanism is crucial for balancing exploration and exploitation, avoiding premature convergence, and optimizing overall performance.

The comparisons clearly shows the independent and incremental contributions of the Reward growth mechanism and the Reward decaying mechanism on top of static heuristics.

Due to time constraints, we have currently only completed experiments for a subset of core counts. We will provide the complete ablation study results for all core configurations in the updated version of the paper.