[toc]

# 4 最速下降法收敛性探讨

## 4.1 误差向量正好是矩阵的一个特征向量

$$
a_i = \frac{{r_i}^Tr_i}{{r_i}^TAr_i} \\
= \frac{{r_i}^Tr_i}{{r_i}^TA^Tr_i} \\
= \frac{{r_i}^Tr_i}{(A{r_i})^Tr_i} \\
= \frac{{r_i}^Tr_i}{(-AAe_i)^Tr_i} \\
= \frac{{r_i}^Tr_i}{\lambda{r_i}^Tr_i} \\
= \frac{1}{\lambda} \tag{4.1.1}
$$

证明, 经过一步后, 误差向量变为0.
$$
x_{i+1} = x_i + a_ir_i \\
x_{i+1} - x = x_i - x + a_i(-Ae_i) \\
e_{i+1} = e_i + a_i(-\lambda e_i) & 由4.1.1 \\ \tag{4.1.2}
e_{i+1} = e_i - e_i \\
= 0
$$

## 4.2 考虑更一般的情况

​	对于更一般的误差向量, 可以将它使用矩阵A的特征向量表达.($\omega_j$是误差向量在特征向量上的分量大小)
$$
e_i =  \sum_{j=1}^n v_j \omega_j
$$
易得以下等式
$$
{e_i}^Te_i = \sum_{j=1}^n {\omega_j}^2 \tag{4.2.1} \\
$$

$$
r_i = -\sum_{j=1}^n \lambda_j v_j \omega_j \tag{4.2.2} \\
$$

$$
{e_i}^TAe_i = \sum_{j=1}^n \lambda_j {\omega_j}^2 \tag{4.2.3}
$$

$$
{r_i}^Tr_i = \sum_{j=1}^n \lambda_j^2 \omega_j^2 \tag{4.2.4} \\
$$

$$
{r_i}^TAr_i = \sum_{j=1}^n \lambda_j^3 \omega_j^2 \tag{4.2.5} \\
$$

则
$$
e_{i+1} = e_i + a_i r_i \\
e_{i+1} = e_i + \frac{{r_i}^Tr_i}{{r_i}^TAr_i} r_i \\
e_{i+1} = e_i + \frac{\sum_{j=1}^n \lambda_j^2 \omega_j^2}{\sum_{j=1}^n \lambda_j^3 \omega_j^2} r_i \\
$$

* 假设各个特征值均相同
  $$
  e_{i+1} = e_i + \frac{\lambda^2\sum_{j=1}^n \omega_j^2}{\lambda^3 \sum_{j=1}^n  \omega_j^2} * - \lambda\sum_{j=1}^n  v_j \omega_j \\
  = e_i - e_i \\
  = 0
  $$
  可见, 在这种情况下, 仍然只需要一次就能收敛.

  那么, 如果$\lambda$不同呢

  

## 4.3 能量范式

* 定义

$$
||e_i||_A = ({e_i}^TAe_i)^{\frac{1}{2}}
$$

由1.5得知, 当能量范式值最小时, 表示误差向量最小(等价).
$$
{||e_{i+1}||_A}^2 = {e_{i+1}}^TAe_{i+1} \\
= {(e_i + a_ir_i)}^TA(e_i + a_ir_i) \\
= {e_i}^TAe_i + a_i{r_i}^TAe_i + a_i{e_i}^TAr_i + {a_i}^2{r_i}^TAr_i \\
$$
由1.4得知${r_i}^TAe_i={e_i}^TAr_i$
$$
{e_i}^TAe_i + 2a_i{r_i}^TAe_i + {a_i}^2{r_i}^TAr_i \\
= {e_i}^TAe_i + 2\frac{{r_i}^Tr_i}{{r_i}^TAr_i}{r_i}^TAe_i + (\frac{{r_i}^Tr_i}{{r_i}^TAr_i})^2{r_i}^TAr_i \\
= {||e_i||_A}^2 - 2\frac{{r_i}^Tr_i}{{r_i}^TAr_i}{r_i}^Tr_i + \frac{({r_i}^Tr_i)^2}{{r_i}^TAr_i} \\
= {||e_i||_A}^2 - \frac{({r_i}^Tr_i)^2}{{r_i}^TAr_i} \\
= {||e_i||_A}^2(1 - \frac{({r_i}^Tr_i)^2}{{r_i}^TAr_i {e_i}^TAe_i}) \\
= {||e_i||_A}^2(1-\frac{(\sum_{j}^n \lambda_j^2 \omega_j^2)^2}{\sum_{j}^n \lambda_j^3 \omega_j^2 \sum_{j}^n \lambda_j {\omega_j}^2})
$$

* 收敛因子的定义,设

$$
\Omega^2=(1-\frac{(\sum_{j} \lambda_j^2 \omega_j^2)^2}{\sum_{j} \lambda_j^3 \omega_j^2 \sum_{j} \lambda_j {\omega_j}^2})
$$

​	由此可知, 一般情况的收敛性分析关键在于找到w的一个上界. 先假设n=2(即有两个特征向量)情况下的结果. 假定$\lambda_1>\lambda_2$误差向量$e_i$的**斜率(相对于由特征向量组成的坐标系而言)取决于起点,记为$u=\frac{w_2}{w_1}$**; 条件范数为$\kappa=\frac{max(\lambda)}{min(\lambda)}$.

由此有:
$$
\Omega^2=(1-\frac{(\lambda_1^2 \omega_1^2 + \lambda_2^2 \omega_2^2)^2}{ (\lambda_1^2 \omega_1 + \lambda_2^2 \omega_2) (\lambda_1^2 \omega_1^3 + \lambda_2^2 \omega_2^3)}) \\
=1-\frac{(\kappa^2 + \mu^2)^2}{(\kappa+\mu^2)(\kappa^3+\mu^2)}
$$
收敛因子越大, 收敛性越差. 下面的结果没有经过严格证明
$$
当\mu和\kappa为大值时, \Omega普遍较大; \\
当\mu和\kappa为小值时, \Omega普遍较小; \\
其它时候要视具体情况而定.
$$
当$\kappa$确定时, 那么$\Omega$的上届通过求驻点得(其中$\kappa$是矩阵性质, 看作常量)
$$
\Omega\prime(u) =
$$
可见$u=\pm\kappa$时, 导数为0. 此时$\Omega$为最大值.

最大值是
$$
max(\Omega) = \frac{(k-1)}{(k+1)}
$$
能量范式的迭代过程
$$
||e_{i+1}||_A \le||e_i||_A (\frac{k-1}{k+1})^i
$$