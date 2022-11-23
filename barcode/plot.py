import matplotlib.pyplot as graph
import numpy as np
import os

path = "C:/Users/brand/Desktop"

file = open(os.path.join(path, "n.log"))
lines = file.readlines()

x = []
y = []

for i, line in enumerate(lines):
    if i >= 11000 and i <= 12000:
        x.append(i - 11000)
        y.append(int(line))

graph.plot(np.array(x), np.array(y))

graph.title("No Filtering")
#graph.title("Filtering with Average of 50 samples")
#graph.title("Filtering with Average of 50 samples & Range of ± 10")
graph.xlabel("Read Count")
graph.ylabel("ADC")

graph.savefig(os.path.join(path, "barcode.png"))