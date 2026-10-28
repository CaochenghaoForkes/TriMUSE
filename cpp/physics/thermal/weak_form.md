## 瞬态导热方程

\[
\rho c_p\frac{\partial T}{\partial t}
-\nabla\cdot(k\nabla T)=Q
\qquad \text{在 }\Omega.
\]

傅立叶导热定律
\[
\boldsymbol q=-k\nabla T.
\]

定义向外热流方向
\[
q_{\mathrm{out}}
=\boldsymbol q\cdot\boldsymbol n
=-k\nabla T\cdot\boldsymbol n.
\]

三类边界
\[
\begin{aligned}
T&=\bar T &&\text{在 }\Gamma_D,\\
q_{\mathrm{out}}&=\bar q &&\text{在 }\Gamma_N,\\
q_{\mathrm{out}}&=h(T-T_\infty) &&\text{在 }\Gamma_R.
\end{aligned}
\]

初值条件
\[
T(\boldsymbol x,0)=T_0(\boldsymbol x).
\]

选择试函数
\[
v=0\qquad\text{在 }\Gamma_D.
\]
\[
T\in V_{\bar T}
=\{u\in H^1(\Omega):u=\bar T\text{ 在 }\Gamma_D\},
\]
\[
v\in V_0
=\{v\in H^1(\Omega):v=0\text{ 在 }\Gamma_D\}.
\]

乘试函数 对空间积分

\[
\int_\Omega
\rho c_p\frac{\partial T}{\partial t}v\,\mathrm d\Omega
-
\int_\Omega
v\,\nabla\cdot(k\nabla T)\,\mathrm d\Omega
=
\int_\Omega Qv\,\mathrm d\Omega.
\]

导热项部分使用分部积分

\[
\nabla\cdot(vk\nabla T)
=
\nabla v\cdot k\nabla T
+
v\,\nabla\cdot(k\nabla T),
\]

散度定理

\[
-\int_\Omega v\,\nabla\cdot(k\nabla T)\,\mathrm d\Omega
=
\int_\Omega k\nabla T\cdot\nabla v\,\mathrm d\Omega
-
\int_{\partial\Omega}v\,k\nabla T\cdot\boldsymbol n\,\mathrm d\Gamma.
\]

带入热流方向的约定得到

\[
\int_\Omega \rho c_p\frac{\partial T}{\partial t}v\,\mathrm d\Omega
+
\int_\Omega k\nabla T\cdot\nabla v\,\mathrm d\Omega
+
\int_{\partial\Omega}q_{\mathrm{out}}v\,\mathrm d\Gamma
=
\int_\Omega Qv\,\mathrm d\Omega.
\]

带入边界得到最终弱形式

\[
\boxed{
\begin{aligned}
&\int_\Omega \rho c_p\frac{\partial T}{\partial t}v\,\mathrm d\Omega
+\int_\Omega k\nabla T\cdot\nabla v\,\mathrm d\Omega
+\int_{\Gamma_R}hTv\,\mathrm d\Gamma\\
&\qquad=
\int_\Omega Qv\,\mathrm d\Omega
-\int_{\Gamma_N}\bar qv\,\mathrm d\Gamma
+\int_{\Gamma_R}hT_\infty v\,\mathrm d\Gamma,
\qquad \forall v\in V_0.
\end{aligned}
}
\]

离散: 用形函数插值温度

\[
T_h(\boldsymbol x,t)=\sum_jN_j(\boldsymbol x)t_j(t).
\]

Galerkin 让试函数依次取形函数

\[
v_h=N_i,\qquad i=1,2,\ldots
\]

得到 $i$ 个线性方程 $i$ $j$ 遍历当前标量有限元空间的全部自由度；对于我们使用的节点型 H1 元，可以理解为全部场插值节点

\[
\sum_jM_{ij}\dot t_j+\sum_jK_{ij}t_j=F_i,
\]

\[
M_{ij}=\int_\Omega \rho c_pN_iN_j\,\mathrm d\Omega,
\]

\[
K_{ij}
=
\int_\Omega k\nabla N_i\cdot\nabla N_j\,\mathrm d\Omega
+
\int_{\Gamma_R}hN_iN_j\,\mathrm d\Gamma,
\]

\[
F_i
=
\int_\Omega QN_i\,\mathrm d\Omega
-\int_{\Gamma_N}\bar qN_i\,\mathrm d\Gamma
+\int_{\Gamma_R}hT_\infty N_i\,\mathrm d\Gamma.
\]

可以整理成总体矩阵的形式

\[
\boxed{M\dot{\boldsymbol t}+K\boldsymbol t=F.}
\]

实际单元积分中: 把物理单元映射回参考单元，在参考坐标下计算形函数并求积
设几何映射为

\[
\boldsymbol x=F_e(\boldsymbol\xi),\qquad
J=\frac{\partial\boldsymbol x}{\partial\boldsymbol\xi}.
\]

物理形函数与参考形函数满足: \(\hat N_i=\hat N_i(\boldsymbol\xi)\) 表示定义在参考单元上以参考坐标为自变量的形函数

\[
N_i(\boldsymbol x)=\hat N_i(F_e^{-1}(\boldsymbol x)).
\]
\[
N_i(F_e(\boldsymbol\xi))=\hat N_i(\boldsymbol\xi)
\]

因此体积分转换为:

\[
\int_{\Omega_e} f(\boldsymbol x)\,\mathrm d\Omega
=
\int_{\hat\Omega}
f(F_e(\boldsymbol\xi))\,|\det J|\,\mathrm d\boldsymbol\xi.
\]

例如热容矩阵

\[
M^e_{ij}
=
\int_{\hat\Omega}
\rho c_p\,
\hat N_i\hat N_j\,
|\det J|\,\mathrm d\boldsymbol\xi.
\]

导热项还需要变换梯度

\[
\nabla_{\boldsymbol x}N_i
=J^{-\mathsf T}\nabla_{\boldsymbol\xi}\hat N_i.
\]

\[
K^e_{ij}
=
\int_{\hat\Omega}
k\,
(J^{-\mathsf T}\nabla_{\boldsymbol\xi}\hat N_i)
\cdot
(J^{-\mathsf T}\nabla_{\boldsymbol\xi}\hat N_j)
|\det J|\,\mathrm d\boldsymbol\xi.
\]

## 界面热阻

首先,对于分域场,同一个界面位置有两套独立自由度，因此整体形函数也分成了两套，如两个一维单元相邻：
区域 a                    区域 b
a0 ───────── a1   │   b0 ───────── b1
x=0          x=1      x=1          x=2
a1 和 b0 坐标相同，但在分域场中是不同的自由度。整体温度向量有四项：
\[
\boldsymbol t=
\begin{bmatrix}
t_{a0}&t_{a1}&t_{b0}&t_{b1}
\end{bmatrix}^{\mathsf T}.
\]
每个自由度都有自己的形函数
以 a1 对应的形函数为例，它在整个计算域上的定义是：因为它只插值区域 a 的自由度，不参与区域 b 的温度插值
\[
\Phi_{a1}(x)=
\begin{cases}
N_{a1}(x),&x\in\Omega_a,\\
0,&x\in\Omega_b.
\end{cases}
\]

所以，用“两侧分别是什么”来表示这个整体形函数，就是：
\[
\Phi_{a1}=(N_{a1},0).
\]

同样，b0 的整体形函数是：
\[
\Phi_{b0}=(0,N_{b0}).
\]
以前 $v_h=N_i$ 分域后仍然是这件事，只是现在整体基函数变成：
\[
(N_{a0},0),\quad
(N_{a1},0),\quad
(0,N_{b0}),\quad
(0,N_{b1}).
\]

依次选择它们就得到了：
v = (Na0, 0) → a0 自由度对应的方程
v = (Na1, 0) → a1 自由度对应的方程
v = (0, Nb0) → b0 自由度对应的方程
v = (0, Nb1) → b1 自由度对应的方程

因此，建立 a 侧第 \(i\) 行时选择：
\[
v_a=N_i^a,\qquad v_b=0,
\]
实际上就是选择了第 \(i\) 个 a 侧整体基函数作为测试函数，得到了 a 侧第 ai 自由度对应的方程


内部热阻界面: 设区域 $\Omega_a$ 和 $\Omega_b$ 共用界面 $\Gamma_I$,
两侧的温度自由度独立。定义从 $a$ 侧流向 $b$ 侧的热流密度为正,
单位面积热阻为 $R_I$, 传热系数为 $h_I=1/R_I$。

\[
q_I=h_I(T_a-T_b).
\]

两侧的法向均取各自区域的外法向, 所以同一股界面热流满足

\[
q_{\mathrm{out},a}
=-k_a\nabla T_a\cdot\boldsymbol n_a=q_I,
\qquad
q_{\mathrm{out},b}
=-k_b\nabla T_b\cdot\boldsymbol n_b=-q_I.
\]

两侧分别乘测试函数并分部积分。沿用上文的左端外向热流项,
两侧界面贡献相加得到 (热阻界面的热流依赖两侧未知温度，所以整理到左端)

\[
\begin{aligned}
I_{\Gamma_I}
&=\int_{\Gamma_I}q_{\mathrm{out},a}v_a\,\mathrm d\Gamma
+\int_{\Gamma_I}q_{\mathrm{out},b}v_b\,\mathrm d\Gamma\\
&=\int_{\Gamma_I}q_I(v_a-v_b)\,\mathrm d\Gamma\\
&=\int_{\Gamma_I}h_I(T_a-T_b)(v_a-v_b)\,\mathrm d\Gamma.
\end{aligned}
\]

这个界面项加入两侧热方程弱形式的左端, 表示区域之间的热量交换,
不增加外部热源。

离散: 分别用两侧单元的形函数插值温度 (\(n_a,n_b\) 是该面片两侧相邻单元的自由度数；)

\[
T_{a,h}=\sum_{j=1}^{n_a}N_j^a t_j^a,
\qquad
T_{b,h}=\sum_{j=1}^{n_b}N_j^b t_j^b.
\]

Galerkin 方法: 分别取两侧形函数作为测试函数。 (\(i,j\) 遍历这两个单元的局部自由度；)
对 $a$ 侧的第 $i$ 行, 取 $v_a=N_i^a$, $v_b=0$, 界面项为

\[
\sum_{j=1}^{n_a}
\left(\int_{\Gamma_I}h_I N_i^aN_j^a\,\mathrm d\Gamma\right)t_j^a
-
\sum_{j=1}^{n_b}
\left(\int_{\Gamma_I}h_I N_i^aN_j^b\,\mathrm d\Gamma\right)t_j^b.
\]

对 $b$ 侧的第 $i$ 行, 取 $v_a=0$, $v_b=N_i^b$, 界面项为

\[
-
\sum_{j=1}^{n_a}
\left(\int_{\Gamma_I}h_I N_i^bN_j^a\,\mathrm d\Gamma\right)t_j^a
+
\sum_{j=1}^{n_b}
\left(\int_{\Gamma_I}h_I N_i^bN_j^b\,\mathrm d\Gamma\right)t_j^b.
\]

其中 $i$ 对应当前测试函数和矩阵行, $j$ 对应温度展开中的自由度和矩阵列。
由此得到四个矩阵块

\[
\begin{aligned}
(K_{aa})_{ij}&=\int_{\Gamma_I}h_I N_i^aN_j^a\,\mathrm d\Gamma,
&
(K_{ab})_{ij}&=-\int_{\Gamma_I}h_I N_i^aN_j^b\,\mathrm d\Gamma,\\
(K_{ba})_{ij}&=-\int_{\Gamma_I}h_I N_i^bN_j^a\,\mathrm d\Gamma,
&
(K_{bb})_{ij}&=\int_{\Gamma_I}h_I N_i^bN_j^b\,\mathrm d\Gamma.
\end{aligned}
\]

把两侧形函数值分别写成列向量 $\boldsymbol N_a$、$\boldsymbol N_b$,
并拼接自由度温度列向量

\[
\boldsymbol t=
\begin{bmatrix}\boldsymbol t_a\\\boldsymbol t_b\end{bmatrix},
\qquad
\boldsymbol B=
\begin{bmatrix}\boldsymbol N_a\\-\boldsymbol N_b\end{bmatrix}.
\]

负号只放在 $\boldsymbol B$ 的第二侧形函数中, 因而

\[
T_{a,h}-T_{b,h}=\boldsymbol B^{\mathsf T}\boldsymbol t.
\]

四个矩阵块可以合并成

\[
\boxed{
K_{\Gamma_I}
=\begin{bmatrix}K_{aa}&K_{ab}\\K_{ba}&K_{bb}\end{bmatrix}
=\int_{\Gamma_I}h_I\boldsymbol B\boldsymbol B^{\mathsf T}\,\mathrm d\Gamma.
}
\]

实际界面片积分: 将参考面点 $\hat{\boldsymbol s}$ 映射到两侧参考单元,
再在对应参考坐标上计算两侧形函数

\[
\boldsymbol\xi_a=\mathrm{Loc1}(\hat{\boldsymbol s}),
\qquad
\boldsymbol\xi_b=\mathrm{Loc2}(\hat{\boldsymbol s}),
\]

\[
\hat{\boldsymbol B}(\hat{\boldsymbol s})=
\begin{bmatrix}
\hat{\boldsymbol N}_a(\boldsymbol\xi_a)\\
-\hat{\boldsymbol N}_b(\boldsymbol\xi_b)
\end{bmatrix}.
\]

对一个界面片 $\Gamma_{I,e}$, 积分转换为

\[
K_{\Gamma_{I,e}}
=\int_{\hat\Gamma_e}
h_I\,\hat{\boldsymbol B}\hat{\boldsymbol B}^{\mathsf T}
J_\Gamma\,g\,\mathrm d\hat{\boldsymbol s}.
\]

这里 $J_\Gamma$ 为面映射的雅可比权重, 一维点界面取 $1$。
$g$ 为降维几何权重: 一维球对称取 $4\pi r^2$, 二维轴对称取 $2\pi r$,
完整三维取 $1$。前面的物理界面积分隐含相应的实际面积测度。

采用参考面求积规则后

\[
\boxed{
K_{\Gamma_{I,e}}\approx
\sum_q
\underbrace{h_{I,q}w_qJ_{\Gamma,q}g_q}_{\texttt{weight}}
\underbrace{\hat{\boldsymbol B}_q}_{\texttt{jump}}
\hat{\boldsymbol B}_q^{\mathsf T}.
}
\]

因此代码逐个积分点累加

```cpp
mfem::AddMult_a_VVt(weight, jump, local);
```

再按两侧拼接的完整自由度编号, 将局部矩阵加入整个界面模块的矩阵

```cpp
matrix.AddSubMatrix(joined, joined, local, 0);
```
