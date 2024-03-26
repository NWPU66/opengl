import random
from turtle import color
import numpy as np
import matplotlib.pyplot as plt
from scipy.stats import norm
import seaborn
from sympy import true
from tqdm.auto import tqdm

seaborn.set_theme()

m = 10000  # 燃烧期样本数目
M = 100000  # 实际保留的有效样本数
sample = [0.0 for i in range(m + M)]
sample[0] = 2.0


def pi(x):
    return (
        0.3 * np.exp(-((x - 0.3) ** 2)) + 0.7 * np.exp(-((x - 2.0) ** 2) / 0.3)
    ) / 1.2113


if __name__ == "__main__":
    for t in tqdm(range(1, m + M)):
        x = sample[t - 1]
        x_star = norm.rvs(loc=x, scale=1, size=1)
        alpha = min(1, pi(x_star) / pi(x))

        u = random.uniform(0, 1)
        if u < alpha:
            sample[t] = x_star
        else:
            sample[t] = x

    x = np.arange(-2, 4, 0.01)
    plt.plot(x, pi(x), color="blue")
    plt.hist(x=np.array(sample[m:]), bins=500, color="red", alpha=0.6)
    plt.show()
