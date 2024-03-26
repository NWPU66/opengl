import numpy as np
import matplotlib.pyplot as plt

R = 1.0
resolution = 25

if __name__ == "__main__":
    # # 在 r 和 \theta 上采样
    # r = np.arange(0, 1, 1 / resolution) * R
    # theta = np.arange(0, 1, 1 / resolution) * 2.0 * np.pi

    # 均匀采样
    uni = np.arange(0, 1, 1 / resolution)
    r = R * np.sqrt(uni)
    theta = 2.0 * np.pi * uni

    # 计算xy坐标
    points = list()
    points_std = list()
    for i in r:
        for j in theta:
            points += [(i * np.cos(j), i * np.sin(j))]
            points_std += [(i, j)]

    # 画出点的分布
    plt.scatter(*zip(*points), s=1)
    plt.show()

    # 画出r和\theta在标准空间的分布
    plt.scatter(*(zip(*points_std)), s=1)
    plt.show()
