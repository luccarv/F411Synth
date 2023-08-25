import numpy as np
import matplotlib.pyplot as plot

x = []
y = []

i = 0;
for line in open('wave.txt', 'r'):
    x.append(i)
    i = i + 1

    y.append(float(line))

plot.plot(x, y)
plot.show()