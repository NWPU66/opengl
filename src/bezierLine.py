import numpy as np
import matplotlib.pyplot as plt
from scipy.special import comb


def getInterpolationPoints(controlPoints, tList):
    n = len(controlPoints) - 1
    interPoints = list()
    for t in tList:
        Bt = np.zeros(2, np.float64)
        for i in range(len(controlPoints)):
            Bt = Bt + comb(n, i) * np.power(1 - t, n - i) * np.power(t, i) * np.array(
                controlPoints[i]
            )
        interPoints.append(list(Bt))
    return interPoints


if __name__ == "__main__":
    points = [[1, 1], [3, 4], [5, 5], [7, 2]]
    tList = np.linspace(0, 1, 10)
    interPointsList = getInterpolationPoints(points, tList)
    x = np.array(interPointsList)[:, 0]
    y = np.array(interPointsList)[:, 1]

    plt.plot(x, y, color="b")
    plt.scatter(np.array(points)[:, 0], np.array(points)[:, 1], color="r")
    plt.show()
