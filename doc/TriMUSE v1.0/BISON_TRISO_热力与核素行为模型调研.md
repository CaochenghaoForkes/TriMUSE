# BISON 的 TRISO 热、力与核素行为模型调研

整理日期：2026-11-04。

范围为 **TRISO 颗粒燃料**，未将 BISON 的全部水堆、金属燃料模型混入本表。依据公开论文、技术报告及官方文档整理；表格中的年份是所列文献的发表年份，不一定是模型首次进入 BISON 的年份。期刊文献优先使用正式卷期年份，在线发表年份不同的另作说明。官方网页未标注统一出版年份，因此以“官方文档”列示，并在文献表中注明访问日期。

需要区分三个层次：物理模型描述材料行为；数值方法用于求解模型；Weibull 等属于失效评价方法。论文展示过某种能力，不代表所有 BISON 算例默认启用，也不保证所有能力已在同一算例中统一耦合。

## 热行为

| 模型 | 方法 | 文献 | 年份 |
|---|---|---|---|
| 多层颗粒稳态、瞬态导热 | 求解含裂变热源的能量守恒方程；采用有限元离散，可进行一维球对称、二维轴对称和三维分析，并与力学、核素输运耦合。 | Hales J D, et al.：[Multidimensional multiphysics simulation of TRISO particle fuel][r1] | 2013 |
| 核芯、Buffer、PyC、SiC 的热物性 | 通过经验关系计算导热系数、比热和密度；不同材料模型可考虑温度、孔隙率及辐照等因素。PyC 的方向性由相应材料模型体现。 | Jiang W, et al.：[TRISO particle fuel performance and failure analysis with BISON][r2]<br>Idaho National Laboratory：[BISON TRISO Workshop][d1] | 2021；官方文档 |
| 裂变功率与热源 | 根据裂变率密度及每次裂变释放的能量计算体积热源；功率、通量及辐照历史作为输入或由外部程序提供。 | Hales J D, et al.：[Multidimensional multiphysics simulation of TRISO particle fuel][r1] | 2013 |
| 间隙、脱粘界面的传热 | 采用两侧温差驱动的界面传热关系。BISON 通用框架包含气体导热、辐射及固体接触贡献；间隙宽度可以来自力学变形，气体成分影响导热能力。具体 TRISO 算例按需要选用这些贡献。 | Idaho National Laboratory：[Gap/Plenum Models][d2]<br>Jiang W, Singh G, Hales J D, et al.：[Efficient Failure Probability Calculations and Modeling Interface Debonding in TRISO Particles with Bison][r3] | 2021 报告；官方文档 |

已有的重要反馈路径是：**温度影响变形，变形改变间隙，间隙再影响温度。**

## 力学、裂纹与界面行为

| 模型 | 方法 | 文献 | 年份 |
|---|---|---|---|
| 多层颗粒弹性变形 | 以位移为未知量求解力学平衡方程，根据材料选择弹性本构。PyC 的各向异性模型采用局部材料方向，并将弹性张量转换到计算坐标系。 | Jiang W, et al.：[TRISO particle fuel performance and failure analysis with BISON][r2]<br>Jiang W, Singh G, Hales J D, et al.：[Efficient Failure Probability Calculations and Modeling Interface Debonding in TRISO Particles with Bison][r3] | 2021 |
| 热膨胀 | 将热应变作为本征应变，从总应变中扣除后计算应力；PyC 可以采用径向、切向不同的热膨胀关系。 | Jiang W, et al.：[TRISO particle fuel performance and failure analysis with BISON][r2] | 2021 |
| Buffer 辐照收缩、致密化 | 采用辐照历史相关的尺寸变化关系，形成辐照本征应变；收缩会改变可容纳气体的空间，并可能形成 Buffer–IPyC 间隙。 | Idaho National Laboratory：[BISON TRISO Workshop][d1]<br>Krajewska, et al.：[Analysis of buffer-IPyC separation in TRISO fuel particles][r4] | 2024；官方文档 |
| PyC 辐照收缩与膨胀 | 使用与温度、快中子注量、密度及各向异性有关的经验关系，分别计算径向和切向辐照应变；可描述收缩及后续尺寸变化。 | Jiang W, Singh G, Hales J D, et al.：[Efficient Failure Probability Calculations and Modeling Interface Debonding in TRISO Particles with Bison][r3] | 2021 |
| 核芯辐照肿胀 | 使用燃耗相关的体积肿胀关系，并转换为本征应变；例如 UCO 模型包含固体裂变产物导致的肿胀。 | Idaho National Laboratory：[BISON TRISO Workshop][d1] | 官方文档 |
| Buffer、PyC 蠕变 | 采用应力及辐照条件相关的蠕变本构，随时间更新非弹性应变；用于计算应力松弛及各层载荷重新分配。 | Jiang W, et al.：[TRISO particle fuel performance and failure analysis with BISON][r2] | 2021 |
| 内部气体压力 | 根据裂变气体释放、CO 生成量、气体温度和有效空腔体积计算压力，再将压力作为力学载荷；变形又会改变空腔体积。 | Idaho National Laboratory：[Gap/Plenum Models][d2]<br>Idaho National Laboratory：[BISON TRISO Workshop][d1] | 官方文档 |
| 完好涂层及压力容器失效 | 先求解各层应力，再利用 Weibull 最弱环节理论，对受拉区域进行空间积分，计算失效概率；可考虑颗粒尺寸及材料强度的离散性。 | Jiang W, et al.：[TRISO particle fuel performance and failure analysis with BISON][r2] | 2021 |
| IPyC 裂纹及其引起的 SiC 应力集中 | 用 XFEM 在无需网格沿裂纹重新划分的情况下表示位移不连续；设置裂纹位置、方向及启用条件，计算裂纹形成后的应力重分布。 | Jiang W, et al.：[TRISO particle fuel performance and failure analysis with BISON][r2]<br>Ghezzi, et al.：[TRISO Burnup-Dependent Failure Analysis of a HTGR Design-Basis Accident Using BISON][r5] | 2021、2024 |
| 界面脱粘：有限厚度损伤层 | 在材料界面附近划出薄的可脱粘单元层；达到应力或应变判据后，大幅降低其弹性模量，以近似界面分离和应力释放。 | Schappel D, Pastore G, Terrani K A：[Modeling Interface Debonding in Coated Fuel Particles with BISON][r6] | 2022，2021 年在线发表 |
| 界面脱粘：黏聚区模型 CZM | 采用双线性、混合模式的牵引–分离关系；界面先弹性传力，达到起始判据后损伤软化，通过断裂能控制完全脱粘。 | Jiang W, Singh G, Hales J D, et al.：[Efficient Failure Probability Calculations and Modeling Interface Debonding in TRISO Particles with Bison][r3] | 2021 |
| 界面脱粘：mortar–CZM | 将 mortar 界面离散与黏聚区模型结合，支持两侧不匹配网格，并同时处理机械接触和黏聚断裂；已有二维、三维颗粒示例。 | Jiang W, Recuero A M, Hirschhorn J, Sweet R：[Debonding Modeling of TRISO layers with BISON][r7] | 2025 |
| SiC 的断裂力学失效评价 | 利用相互作用域积分计算应力强度因子，考虑热、辐照本征应变和界面附近渐变物性；据此评价 IPyC 裂纹诱发的 SiC 失效，并与应力型 Weibull 方法比较。 | Recuero A M, Singh G, Jiang W：[Fracture mechanics approach to TRISO fuel particle failure analysis][r8] | 2024 |
| 非球形、局部厚度变化导致的应力集中 | 对平底等理想缺陷建立多维模型，或从 XCT 重建真实三维几何，直接计算局部应力；也可以提取多维应力修正关系供一维统计模型使用。 | Jiang W, et al.：[TRISO particle fuel performance and failure analysis with BISON][r2]<br>Poschmann M, Prudil A, Osmond R：[Fuel performance simulations of TRISO particle geometries derived from XCT][r9] | 2021、2025 |

Weibull、XFEM、CZM 和断裂力学承担不同任务：Weibull 评价失效概率；XFEM 表示材料内部裂纹；CZM 描述预定义材料界面的损伤分离；断裂力学评价裂尖驱动力。这些能力不能直接等同于程序已经能自主预测所有裂纹的起点、路径及跨层扩展。

## 核素、裂变气体与化学侵蚀行为

| 模型 | 方法 | 文献 | 年份 |
|---|---|---|---|
| 多层颗粒核素扩散、产生与衰变 | 求解带源项、衰变项的瞬态核素守恒方程；以有限元计算各层浓度，再通过边界通量或盘存计算释放量、释放份额。 | Hales J D, Jiang W, Toptan A, Gamble K A：[Modeling fission product diffusion in TRISO fuel particles with BISON][r10] | 2021 |
| 经验型核素扩散系数 | 针对核素和材料选用经验扩散关系，通常包含一个或多个 Arrhenius 项；部分关系还包含辐照影响。已有 Ag、Cs、Sr、Kr 等释放分析。 | Hales J D, Toptan A, Jiang W, Spencer B W：[Numerical evaluation of AGR-2 fission product release][r11] | 2022 |
| UCO 核芯裂变气体释放 | 将直接反冲释放与扩散释放相加；扩散部分采用 Booth 等效球模型，还可考虑俘获、再溶解及辐照相关扩散机制。部分 UCO 关系借用 UO₂ 数据，其适用性应结合具体条件判断。 | Idaho National Laboratory：[UCOFGR][d3] | 官方文档 |
| UO₂ 核芯裂变气体释放 | 可采用 Sifgrs，描述晶内气体扩散、晶界气泡及释放等过程；释放气体进入颗粒自由空间，再参与压力计算。 | Idaho National Laboratory：[BISON TRISO Workshop][d1] | 官方文档 |
| CO 生成 | 使用 PROKSCH、GA 等经验模型，根据辐照、燃耗等条件计算 CO 生成量，为颗粒内压模型提供气体来源。 | Idaho National Laboratory：[BISON System Design Description — CarbonMonoxideProduction][d4] | 官方文档 |
| 完整 SiC 中 Ag 的微结构相关输运 | 原子尺度计算晶内、晶界扩散，介观多晶模拟获得有效扩散系数，再将温度、晶粒尺寸相关关系交给 BISON；早期实现使用修正因子。 | Simon P-C A, et al.：[Mechanistic calculation of the effective silver diffusion coefficient in polycrystalline silicon carbide: Application to silver release in AGR-1 TRISO particles][r12] | 2022 |
| Ag 辐照增强扩散 | 在上述多尺度方法中加入辐照增强机制，得到温度、晶粒尺寸和通量相关的有效扩散系数，并用于 AGR-1、AGR-2 释放验证。 | Simon P-C A, Ke J-H, Jiang C, Aagesen L K, Jiang W：[Multiscale, mechanistic modeling of irradiation-enhanced silver diffusion in TRISO particles][r13] | 2025 |
| 涂层失效后的核素释放 | 在部分工程模型中，通过增大失效涂层的有效扩散系数表示屏障能力下降；这是一种失效状态到输运参数的耦合近似。 | Jiang W, et al.：[TRISO particle fuel performance and failure analysis with BISON][r2]<br>Hales J D, Toptan A, Jiang W, Spencer B W：[Numerical evaluation of AGR-2 fission product release][r11] | 2021、2022 |
| 核芯迁移，即 amoeba 效应 | 利用温度梯度和温度相关迁移系数，随时间积分迁移距离；早期工程实现以核芯接触 SiC 作为失效条件之一。 | Novascone S R, et al.：[Summary of BISON milestones and activities - NEAMS FY19 report][r14] | 2019 |
| Pd 侵蚀 SiC：经验模型 | 以温度相关的 Arrhenius 型侵蚀速率累积侵蚀深度；工程失效判据可采用侵蚀深度超过一定 SiC 厚度比例。所列 2026 年文献回顾了原有经验模型，不代表该模型于 2026 年首次实现。 | Bhave C, Dhulipala S L N, Swisher M, et al.：[Bayesian discovery of optimal reduced order models from mechanistic and experimental data: A case study of Pd penetration in TRISO fuels using BISON][r15] | 2026 文献回顾已有模型 |
| Pd 侵蚀 SiC：多尺度机理模型 | 计算 Pd 产生、跨层输运及 SiC 内反应–扩散；用分子动力学提供扩散信息、介观相场模型提取微结构影响，再以机理降阶模型接入 BISON。 | Bhave C, Swisher M, Hirschhorn J, Sweet R, Simon P-C A, Aagesen L K：[A mechanistic, multiscale model for predicting Pd penetration in TRISO fuels using BISON][r16] | 2026 |

**核芯气体释放**是气体从核芯进入颗粒内部空隙，主要影响内压；**颗粒核素释放**是核素穿过涂层进入颗粒外部，直接影响放射性源项，两者应分别建模和评价。

## 统计分析及进一步耦合

| 模型 | 方法 | 文献 | 年份 |
|---|---|---|---|
| 大批量颗粒统计失效 | Monte Carlo 抽样颗粒尺寸及物性，运行快速一维模型，并用多维计算得到的应力关系修正裂纹、非球形影响。 | Jiang W, et al.：[TRISO particle fuel performance and failure analysis with BISON][r2] | 2021 |
| 高效失效概率积分 | 对随机参数空间中的失效概率进行直接数值积分，采用稀疏网格等方法降低计算量，可配合多维颗粒模型。 | Jiang W, et al.：[Efficient high-fidelity TRISO statistical failure analysis using Bison: Applications to AGR-2 irradiation testing][r17] | 2022 |
| 多保真与主动学习可靠性分析 | 联合一维、二维物理模型和代理模型，通过主动学习及子集模拟，将计算集中于失效边界及低概率区域。 | Dhulipala S L N, et al.：[Reliability estimation of an advanced nuclear fuel using coupled active learning, multifidelity modeling, and subset simulation][r18] | 2022 |
| 参数反演与模型不确定性 | 用贝叶斯推断校准 Pd 机理模型，识别重要参数，并量化参数、实验噪声及模型不足带来的不确定性。 | Bhave C, Dhulipala S L N, Swisher M, et al.：[Bayesian discovery of optimal reduced order models from mechanistic and experimental data: A case study of Pd penetration in TRISO fuels using BISON][r15] | 2026 |
| **潜在的与其他行为的耦合：更完整的热—力—输运—化学损伤反馈** | **温度、辐照 → 收缩/肿胀/蠕变 → 间隙、裂纹与脱粘 → 传热和核素输运改变 → 气体压力及 Pd 侵蚀变化 → 再影响应力和失效。**多条局部反馈已有实现；进一步让裂纹开度、侵蚀深度、局部物性和输运通道持续相互更新，是值得关注的拓展方向。**本行是对研究方向的归纳，不能据这些文献认定完整反馈链已统一实现并充分验证。** | Jiang W, Recuero A M, Hirschhorn J, Sweet R：[Debonding Modeling of TRISO layers with BISON][r7]<br>Recuero A M, Singh G, Jiang W：[Fracture mechanics approach to TRISO fuel particle failure analysis][r8]<br>Bhave C, Swisher M, Hirschhorn J, Sweet R, Simon P-C A, Aagesen L K：[A mechanistic, multiscale model for predicting Pd penetration in TRISO fuels using BISON][r16] | 已有局部耦合；完整反馈需逐项核实 |

## 完整文献与文档链接

1. Hales J D, et al. [Multidimensional multiphysics simulation of TRISO particle fuel](https://doi.org/10.1016/j.jnucmat.2013.07.070). Journal of Nuclear Materials, 443(1–3): 531–543, 2013。

2. Jiang W, et al. [TRISO particle fuel performance and failure analysis with BISON](https://doi.org/10.1016/j.jnucmat.2021.152795). Journal of Nuclear Materials, 548: 152795, 2021。

3. Jiang W, Singh G, Hales J D, et al. [Efficient Failure Probability Calculations and Modeling Interface Debonding in TRISO Particles with Bison](https://inldigitallibrary.inl.gov/content/uploads/50/2026/04/Sort_53251.pdf). Idaho National Laboratory, INL/EXT-21-64589, 2021-09-30（技术报告）。

4. Krajewska, et al. [Analysis of buffer-IPyC separation in TRISO fuel particles](https://doi.org/10.1016/j.anucene.2023.110338). Annals of Nuclear Energy, 199: 110338, 2024。

5. Ghezzi, et al. [TRISO Burnup-Dependent Failure Analysis of a HTGR Design-Basis Accident Using BISON](https://doi.org/10.1080/00295639.2023.2230031). Nuclear Science and Engineering, 198(5): 1036–1050, 2024。

6. Schappel D, Pastore G, Terrani K A [Modeling Interface Debonding in Coated Fuel Particles with BISON](https://doi.org/10.1080/00295639.2021.1955590). Nuclear Science and Engineering, 196(3): 276–284, 2022（2021-08-25 在线发表）。

7. Jiang W, Recuero A M, Hirschhorn J, Sweet R [Debonding Modeling of TRISO layers with BISON](https://doi.org/10.13182/TOPFUEL25-48199). TopFuel 2025: Nuclear Reactor Fuel Performance Conference, pp. 145–151, 2025（会议论文）。

8. Recuero A M, Singh G, Jiang W [Fracture mechanics approach to TRISO fuel particle failure analysis](https://doi.org/10.1016/j.jnucmat.2024.155083). Journal of Nuclear Materials, 596: 155083, 2024。

9. Poschmann M, Prudil A, Osmond R [Fuel performance simulations of TRISO particle geometries derived from XCT](https://doi.org/10.1016/j.jnucmat.2025.155714). Journal of Nuclear Materials, 608: 155714, 2025。

10. Hales J D, Jiang W, Toptan A, Gamble K A [Modeling fission product diffusion in TRISO fuel particles with BISON](https://doi.org/10.1016/j.jnucmat.2021.152840). Journal of Nuclear Materials, 548: 152840, 2021。

11. Hales J D, Toptan A, Jiang W, Spencer B W [Numerical evaluation of AGR-2 fission product release](https://doi.org/10.1016/j.jnucmat.2021.153325). Journal of Nuclear Materials, 558: 153325, 2022。

12. Simon P-C A, et al. [Mechanistic calculation of the effective silver diffusion coefficient in polycrystalline silicon carbide: Application to silver release in AGR-1 TRISO particles](https://doi.org/10.1016/j.jnucmat.2022.153669). Journal of Nuclear Materials, 563: 153669, 2022。

13. Simon P-C A, Ke J-H, Jiang C, Aagesen L K, Jiang W [Multiscale, mechanistic modeling of irradiation-enhanced silver diffusion in TRISO particles](https://doi.org/10.1016/j.jnucmat.2024.155464). Journal of Nuclear Materials, 603: 155464, 2025。

14. Novascone S R, et al. [Summary of BISON milestones and activities - NEAMS FY19 report](https://inldigitallibrary.inl.gov/content/uploads/50/2026/04/Sort_20259.pdf). Idaho National Laboratory, INL/EXT-19-55699, 2019；所引 PDF 为 Rev001，第 9 章（技术报告）。

15. Bhave C, Dhulipala S L N, Swisher M, et al. [Bayesian discovery of optimal reduced order models from mechanistic and experimental data: A case study of Pd penetration in TRISO fuels using BISON](https://doi.org/10.1016/j.commatsci.2026.114503). Computational Materials Science, 264: 114503, 2026。

16. Bhave C, Swisher M, Hirschhorn J, Sweet R, Simon P-C A, Aagesen L K [A mechanistic, multiscale model for predicting Pd penetration in TRISO fuels using BISON](https://doi.org/10.1016/j.jnucmat.2026.156536). Journal of Nuclear Materials, 625: 156536, 2026。

17. Jiang W, et al. [Efficient high-fidelity TRISO statistical failure analysis using Bison: Applications to AGR-2 irradiation testing](https://doi.org/10.1016/j.jnucmat.2022.153585). Journal of Nuclear Materials, 562: 153585, 2022。

18. Dhulipala S L N, et al. [Reliability estimation of an advanced nuclear fuel using coupled active learning, multifidelity modeling, and subset simulation](https://doi.org/10.1016/j.ress.2022.108693). Reliability Engineering & System Safety, 226: 108693, 2022。

19. Idaho National Laboratory [BISON TRISO Workshop](https://mooseframework.inl.gov/bison/triso_workshop/index.html). BISON 官方培训文档，访问日期：2026-11-04；页面内容反映其培训版本。

20. Idaho National Laboratory [Gap/Plenum Models](https://mooseframework.inl.gov/bison/theory/gap_plenum_models.html). BISON 官方理论文档，访问日期：2026-11-04。

21. Idaho National Laboratory [UCOFGR](https://mooseframework.inl.gov/bison/source/materials/UCOFGR.html). BISON 官方模型文档，访问日期：2026-11-04。

22. Idaho National Laboratory [BISON System Design Description — CarbonMonoxideProduction](https://mooseframework.inl.gov/bison/sqa/bison_sdd.html). BISON 官方设计说明，见 CarbonMonoxideProduction / PROKSCH / GA 对应条目；访问日期：2026-11-04。

[r1]: https://doi.org/10.1016/j.jnucmat.2013.07.070
[r2]: https://doi.org/10.1016/j.jnucmat.2021.152795
[r3]: https://inldigitallibrary.inl.gov/content/uploads/50/2026/04/Sort_53251.pdf
[r4]: https://doi.org/10.1016/j.anucene.2023.110338
[r5]: https://doi.org/10.1080/00295639.2023.2230031
[r6]: https://doi.org/10.1080/00295639.2021.1955590
[r7]: https://doi.org/10.13182/TOPFUEL25-48199
[r8]: https://doi.org/10.1016/j.jnucmat.2024.155083
[r9]: https://doi.org/10.1016/j.jnucmat.2025.155714
[r10]: https://doi.org/10.1016/j.jnucmat.2021.152840
[r11]: https://doi.org/10.1016/j.jnucmat.2021.153325
[r12]: https://doi.org/10.1016/j.jnucmat.2022.153669
[r13]: https://doi.org/10.1016/j.jnucmat.2024.155464
[r14]: https://inldigitallibrary.inl.gov/content/uploads/50/2026/04/Sort_20259.pdf
[r15]: https://doi.org/10.1016/j.commatsci.2026.114503
[r16]: https://doi.org/10.1016/j.jnucmat.2026.156536
[r17]: https://doi.org/10.1016/j.jnucmat.2022.153585
[r18]: https://doi.org/10.1016/j.ress.2022.108693
[d1]: https://mooseframework.inl.gov/bison/triso_workshop/index.html
[d2]: https://mooseframework.inl.gov/bison/theory/gap_plenum_models.html
[d3]: https://mooseframework.inl.gov/bison/source/materials/UCOFGR.html
[d4]: https://mooseframework.inl.gov/bison/sqa/bison_sdd.html

